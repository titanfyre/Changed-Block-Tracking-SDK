#include <ntddk.h>
#include <ntdddisk.h>
#include <ntddstor.h>
#include <wdf.h>
#include <wdmsec.h>
#include "CbtProtocol.h"
#include "../Runtime/DemoKernel.h"
#define CB_TAG 'tbcT'
typedef struct FILTER {
 WDFDEVICE device;CB_U64 id,session,generation,capacity,mapBytes,writes,inflight,frozenCount;
 ULONG current;BOOLEAN registered,counted,invalid,frozen,expired;
 PUCHAR maps[2];WDFMEMORY memory[2];CB_U16 pdo[CB_NAME];
} FILTER;
typedef struct TRACK {BOOLEAN observed,write;CB_U64 offset,length;} TRACK;
typedef struct OWNER {BOOLEAN accepted;} OWNER;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILTER,FilterContext)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(TRACK,TrackContext)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(OWNER,OwnerContext)
static WDFSPINLOCK StateLock;static WDFWAITLOCK LifecycleLock;
static WDFDEVICE Control;static FILTER* Filters[CB_MAX_DEVICES];static FILTER* volatile Active;
static ULONG DeviceCount;static BOOLEAN OwnerOpen;static CB_U64 NextId,NextSession,NextGeneration;
static TS_RUNTIME_CLIENT Runtime;
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD AddDevice;
EVT_WDF_OBJECT_CONTEXT_CLEANUP FilterCleanup;
EVT_WDF_OBJECT_CONTEXT_DESTROY DriverCleanup;
EVT_WDF_IO_QUEUE_IO_DEFAULT Forward;
EVT_WDF_REQUEST_COMPLETION_ROUTINE Completed;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL Command;
EVT_WDF_DEVICE_FILE_CREATE Open;
EVT_WDF_FILE_CLEANUP Close;
static FILTER* Current(void){return (FILTER*)InterlockedCompareExchangePointer((PVOID volatile*)&Active,NULL,NULL);}
static FILTER* Find(CB_U64 id){ULONG i;for(i=0;i<CB_MAX_DEVICES;++i)if(Filters[i]&&Filters[i]->id==id)return Filters[i];return NULL;}
static void State(FILTER* f,CB_STATE* output){
 RtlZeroMemory(output,sizeof(*output));output->size=sizeof(*output);output->version=CB_VERSION;output->block_bytes=CB_BLOCK;
 output->device=f->id;output->session=f->session;output->generation=f->generation;output->capacity=f->capacity;output->bitmap_bytes=f->mapBytes;
 output->write_requests=f->writes;output->inflight=f->inflight;output->changed_blocks=f->frozen?f->frozenCount:0;
 if(Current()==f)output->flags|=CB_FLAG_ACTIVE;if(f->invalid)output->flags|=CB_FLAG_INVALID;if(f->frozen)output->flags|=CB_FLAG_FROZEN;if(f->expired)output->flags|=CB_FLAG_ACCESS_EXPIRED;
}
static void Mark(FILTER* f,CB_U64 offset,CB_U64 length){
 CB_U64 first,last,bits;RTL_BITMAP bitmap;
 if(!length)return;
 if(offset>=f->capacity||length>f->capacity-offset){f->invalid=TRUE;return;}
 first=offset/CB_BLOCK;last=(offset+length-1)/CB_BLOCK;bits=(f->capacity+CB_BLOCK-1)/CB_BLOCK;
 RtlInitializeBitMap(&bitmap,(PULONG)f->maps[f->current],(ULONG)bits);RtlSetBits(&bitmap,(ULONG)first,(ULONG)(last-first+1));
}
static BOOLEAN ReadOnlyControl(ULONG code){
 switch(code){
 case IOCTL_DISK_GET_DRIVE_GEOMETRY:case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:case IOCTL_DISK_GET_LENGTH_INFO:
 case IOCTL_DISK_GET_PARTITION_INFO:case IOCTL_DISK_GET_PARTITION_INFO_EX:case IOCTL_DISK_GET_DRIVE_LAYOUT:case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
 case IOCTL_STORAGE_QUERY_PROPERTY:case IOCTL_STORAGE_GET_DEVICE_NUMBER:case IOCTL_STORAGE_GET_HOTPLUG_INFO:return TRUE;
 default:return FALSE;
 }
}
// Pending mutations belong to the device, not a tracking session. A request
// submitted before Start can still change the newly tracked disk on completion.
static void Record(WDFDEVICE device,WDFREQUEST request){
 TRACK* t=TrackContext(request);FILTER* f=FilterContext(device);
 if(t->observed){WdfSpinLockAcquire(StateLock);NT_ASSERT(f->inflight>0);if(f->inflight)--f->inflight;
  if(Current()==f&&f->maps[f->current]){
   if(!TsRuntimeAllowed(&Runtime,TITAN_SDK_CHANGED_BLOCK_TRACKING)){f->invalid=TRUE;f->expired=TRUE;}
   else if(t->write)Mark(f,t->offset,t->length);else f->invalid=TRUE;
  }
  t->observed=FALSE;WdfSpinLockRelease(StateLock);
 }
}
VOID Completed(WDFREQUEST request,WDFIOTARGET target,PWDF_REQUEST_COMPLETION_PARAMS result,WDFCONTEXT context){
 WDFDEVICE device=(WDFDEVICE)context;UNREFERENCED_PARAMETER(target);Record(device,request);WdfRequestCompleteWithInformation(request,result->IoStatus.Status,result->IoStatus.Information);WdfObjectDereference(device);
}
VOID Forward(WDFQUEUE queue,WDFREQUEST request){
 WDFDEVICE device=WdfIoQueueGetDevice(queue);FILTER* f=FilterContext(device);TRACK* t=TrackContext(request);WDF_REQUEST_PARAMETERS p;WDF_REQUEST_SEND_OPTIONS options;
 t->observed=FALSE;WDF_REQUEST_PARAMETERS_INIT(&p);WdfRequestGetParameters(request,&p);
 t->write=(BOOLEAN)(p.Type==WdfRequestTypeWrite&&p.Parameters.Write.Length!=0);
 if(t->write||(p.Type==WdfRequestTypeDeviceControl&&!ReadOnlyControl(p.Parameters.DeviceIoControl.IoControlCode))||p.Type==WdfRequestTypeDeviceControlInternal){
  t->observed=TRUE;if(t->write){t->offset=(CB_U64)p.Parameters.Write.DeviceOffset;t->length=(CB_U64)p.Parameters.Write.Length;}
  // Keep FILTER context alive through asynchronous completion/removal. No lock
  // is held while forwarding and no per-request allocation is performed.
  WdfObjectReference(device);WdfSpinLockAcquire(StateLock);++f->inflight;
  if(Current()==f){
   if(!TsRuntimeAllowed(&Runtime,TITAN_SDK_CHANGED_BLOCK_TRACKING)){f->invalid=TRUE;f->expired=TRUE;}
   else if(t->write){++f->writes;Mark(f,t->offset,t->length);}else f->invalid=TRUE;
  }
  WdfSpinLockRelease(StateLock);
 }else if(Current()==f){WdfSpinLockAcquire(StateLock);if(Current()==f&&!TsRuntimeAllowed(&Runtime,TITAN_SDK_CHANGED_BLOCK_TRACKING)){f->invalid=TRUE;f->expired=TRUE;}WdfSpinLockRelease(StateLock);}
 WdfRequestFormatRequestUsingCurrentType(request);
 if(t->observed){WdfRequestSetCompletionRoutine(request,Completed,device);if(!WdfRequestSend(request,WdfDeviceGetIoTarget(device),WDF_NO_SEND_OPTIONS)){NTSTATUS status=WdfRequestGetStatus(request);Record(device,request);WdfRequestComplete(request,status);WdfObjectDereference(device);}}
 else{WDF_REQUEST_SEND_OPTIONS_INIT(&options,WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);if(!WdfRequestSend(request,WdfDeviceGetIoTarget(device),&options))WdfRequestComplete(request,WdfRequestGetStatus(request));}
}
// LifecycleLock held; data paths only access maps while holding StateLock.
static void Stop(FILTER* f){
 WDFMEMORY first,second;WdfSpinLockAcquire(StateLock);
 if(Current()==f)InterlockedExchangePointer((PVOID volatile*)&Active,NULL);
 first=f->memory[0];second=f->memory[1];f->memory[0]=f->memory[1]=NULL;f->maps[0]=f->maps[1]=NULL;f->invalid=TRUE;f->frozen=FALSE;f->session=++NextSession;
 WdfSpinLockRelease(StateLock);if(first)WdfObjectDelete(first);if(second)WdfObjectDelete(second);
}
VOID Open(WDFDEVICE device,WDFREQUEST request,WDFFILEOBJECT file){
 NTSTATUS status=STATUS_DELETE_PENDING;WDF_REQUEST_PARAMETERS parameters;ACCESS_MASK access;
 GENERIC_MAPPING mapping={FILE_GENERIC_READ,FILE_GENERIC_WRITE,FILE_GENERIC_EXECUTE,FILE_ALL_ACCESS};
 OwnerContext(file)->accepted=FALSE;WDF_REQUEST_PARAMETERS_INIT(&parameters);WdfRequestGetParameters(request,&parameters);
 // Every control operation requires both data rights. An unusable metadata-only
 // open must not take the sole owner slot before that IOCTL check can run.
 if(!parameters.Parameters.Create.SecurityContext){WdfRequestComplete(request,STATUS_ACCESS_DENIED);return;}
 access=parameters.Parameters.Create.SecurityContext->DesiredAccess;RtlMapGenericMask(&access,&mapping);
 if((access&(FILE_READ_DATA|FILE_WRITE_DATA))!=(FILE_READ_DATA|FILE_WRITE_DATA)){WdfRequestComplete(request,STATUS_ACCESS_DENIED);return;}
 WdfSpinLockAcquire(StateLock);if(Control==device&&DeviceCount){OwnerContext(file)->accepted=TRUE;OwnerOpen=TRUE;status=STATUS_SUCCESS;}WdfSpinLockRelease(StateLock);WdfRequestComplete(request,status);
}
VOID Close(WDFFILEOBJECT file){ULONG i;WDFDEVICE old=NULL;UNREFERENCED_PARAMETER(file);
 NT_ASSERT(KeGetCurrentIrql()==PASSIVE_LEVEL);_Analysis_assume_(KeGetCurrentIrql()==PASSIVE_LEVEL);
 if(!OwnerContext(file)->accepted)return;OwnerContext(file)->accepted=FALSE;
 WdfWaitLockAcquire(LifecycleLock,NULL);for(i=0;i<CB_MAX_DEVICES;++i)if(Filters[i])Stop(Filters[i]);
 WdfSpinLockAcquire(StateLock);OwnerOpen=FALSE;if(!DeviceCount){old=Control;Control=NULL;}WdfSpinLockRelease(StateLock);if(old)WdfObjectDelete(old);WdfWaitLockRelease(LifecycleLock);
}
VOID Command(WDFQUEUE queue,WDFREQUEST request,size_t outputLength,size_t inputLength,ULONG code){
 CB_REQUEST input;PVOID source=NULL,output=NULL;FILTER* f=NULL;NTSTATUS status;size_t needed=0,information=0;ULONG i;
 UNREFERENCED_PARAMETER(queue);
 if(inputLength!=sizeof(input)){WdfRequestComplete(request,STATUS_INFO_LENGTH_MISMATCH);return;}
 status=WdfRequestRetrieveInputBuffer(request,sizeof(input),&source,NULL);if(!NT_SUCCESS(status)){WdfRequestComplete(request,status);return;}RtlCopyMemory(&input,source,sizeof(input));
 if(input.version!=CB_VERSION||input.size!=sizeof(input)){WdfRequestComplete(request,STATUS_REVISION_MISMATCH);return;}
 status=IoValidateDeviceIoControlAccess(WdfRequestWdmGetIrp(request),FILE_READ_DATA|FILE_WRITE_DATA);if(!NT_SUCCESS(status)){WdfRequestComplete(request,status);return;}
 if(code==CB_ENUM)needed=sizeof(CB_INVENTORY);else if(code==CB_STATUS||code==CB_START||code==CB_CHECKPOINT)needed=sizeof(CB_STATE);else if(code==CB_READ_MAP)needed=sizeof(CB_BITMAP_PAGE);else if(code!=CB_STOP&&code!=CB_ACK){WdfRequestComplete(request,STATUS_INVALID_DEVICE_REQUEST);return;}
 if(needed){if(outputLength<needed){WdfRequestComplete(request,STATUS_BUFFER_TOO_SMALL);return;}status=WdfRequestRetrieveOutputBuffer(request,needed,&output,NULL);if(!NT_SUCCESS(status)){WdfRequestComplete(request,status);return;}}
 WdfWaitLockAcquire(LifecycleLock,NULL);
 WdfSpinLockAcquire(StateLock);if(code!=CB_ENUM)f=Find(input.device);WdfSpinLockRelease(StateLock);
 status=STATUS_SUCCESS;
 if(code==CB_ENUM){CB_INVENTORY* inventory=(CB_INVENTORY*)output;RtlZeroMemory(output,needed);inventory->size=sizeof(*inventory);inventory->version=CB_VERSION;
  WdfSpinLockAcquire(StateLock);for(i=0;i<CB_MAX_DEVICES;++i)if(Filters[i]){CB_DEVICE* d=&inventory->devices[inventory->count++];d->id=Filters[i]->id;RtlCopyMemory(d->pdo_name,Filters[i]->pdo,sizeof(d->pdo_name));}WdfSpinLockRelease(StateLock);information=needed;
 }else if(!f)status=STATUS_NO_SUCH_DEVICE;
 else if(code==CB_STOP)Stop(f);
 else if(code==CB_START){
  GET_LENGTH_INFORMATION length;WDF_MEMORY_DESCRIPTOR descriptor;WDF_REQUEST_SEND_OPTIONS options;PUCHAR a=NULL,b=NULL;WDFMEMORY ma=NULL,mb=NULL;WDF_OBJECT_ATTRIBUTES memoryAttributes;CB_U64 bytes,bits;
   if(Current())status=STATUS_DEVICE_BUSY;
   else if(NextGeneration==MAXULONGLONG)status=STATUS_INTEGER_OVERFLOW;
  else if(!input.capacity||input.capacity>CB_MAX_CAPACITY||input.capacity%512)status=STATUS_INVALID_PARAMETER;
  else{
   RtlZeroMemory(&length,sizeof(length));WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&descriptor,&length,sizeof(length));WDF_REQUEST_SEND_OPTIONS_INIT(&options,WDF_REQUEST_SEND_OPTION_TIMEOUT);WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options,WDF_REL_TIMEOUT_IN_SEC(5));
   status=WdfIoTargetSendIoctlSynchronously(WdfDeviceGetIoTarget(f->device),NULL,IOCTL_DISK_GET_LENGTH_INFO,NULL,&descriptor,&options,NULL);
   if(NT_SUCCESS(status)&&((CB_U64)length.Length.QuadPart!=input.capacity))status=STATUS_INVALID_PARAMETER;
    if(NT_SUCCESS(status)&&!Runtime.api)status=TsRuntimeConnect(&Runtime);
    if(NT_SUCCESS(status))status=TsRuntimeBegin(&Runtime,TITAN_SDK_CHANGED_BLOCK_TRACKING);
   if(NT_SUCCESS(status)){
    bits=(input.capacity+CB_BLOCK-1)/CB_BLOCK;bytes=((bits+31)/32)*4;
    if(bytes<4||bytes>2097152){status=STATUS_INVALID_BUFFER_SIZE;goto FinishCommand;}
    WDF_OBJECT_ATTRIBUTES_INIT(&memoryAttributes);memoryAttributes.ParentObject=f->device;
    status=WdfMemoryCreate(&memoryAttributes,NonPagedPoolNx,CB_TAG,2097152,&ma,(PVOID*)&a);
    if(NT_SUCCESS(status))status=WdfMemoryCreate(&memoryAttributes,NonPagedPoolNx,CB_TAG,2097152,&mb,(PVOID*)&b);
     if(NT_SUCCESS(status)){RtlZeroMemory(a,(SIZE_T)bytes);RtlZeroMemory(b,(SIZE_T)bytes);Stop(f);WdfSpinLockAcquire(StateLock);f->maps[0]=a;f->maps[1]=b;f->memory[0]=ma;f->memory[1]=mb;ma=mb=NULL;f->capacity=input.capacity;f->mapBytes=(bits+7)/8;f->current=0;f->generation=++NextGeneration;f->writes=0;f->invalid=FALSE;f->expired=FALSE;f->frozen=FALSE;f->frozenCount=0;InterlockedExchangePointer((PVOID volatile*)&Active,f);State(f,(CB_STATE*)output);WdfSpinLockRelease(StateLock);information=needed;}
    if(ma)WdfObjectDelete(ma);if(mb)WdfObjectDelete(mb);
   }
  }
 }else{
  WdfSpinLockAcquire(StateLock);
  if(Current()==f&&!TsRuntimeAllowed(&Runtime,TITAN_SDK_CHANGED_BLOCK_TRACKING)){f->invalid=TRUE;f->expired=TRUE;}
  if(code==CB_STATUS){State(f,(CB_STATE*)output);information=needed;}
  else if(f->invalid||Current()!=f)status=STATUS_DATA_ERROR;
  else if(code==CB_CHECKPOINT){
    if(f->frozen||f->inflight)status=STATUS_DEVICE_BUSY;
    else if(NextGeneration==MAXULONGLONG)status=STATUS_INTEGER_OVERFLOW;
    else{RTL_BITMAP bitmap;f->current=1-f->current;f->frozen=TRUE;f->generation=++NextGeneration;RtlInitializeBitMap(&bitmap,(PULONG)f->maps[1-f->current],(ULONG)((f->capacity+CB_BLOCK-1)/CB_BLOCK));
    WdfSpinLockRelease(StateLock);f->frozenCount=RtlNumberOfSetBits(&bitmap);WdfSpinLockAcquire(StateLock);State(f,(CB_STATE*)output);information=needed;}
  }else if(!f->frozen||input.generation!=f->generation)status=STATUS_INVALID_PARAMETER;
  else if(code==CB_READ_MAP){
   CB_BITMAP_PAGE* page=(CB_BITMAP_PAGE*)output;CB_U64 remaining;
   if(input.offset>f->mapBytes)status=STATUS_INVALID_PARAMETER;
   else{RtlZeroMemory(page,sizeof(*page));State(f,&page->state);page->offset=input.offset;remaining=f->mapBytes-input.offset;page->bytes=(ULONG)(remaining<CB_PAGE?remaining:CB_PAGE);RtlCopyMemory(page->data,f->maps[1-f->current]+(SIZE_T)input.offset,page->bytes);information=needed;}
  }else if(code==CB_ACK){PUCHAR old=f->maps[1-f->current];SIZE_T bytes=(SIZE_T)((((f->capacity+CB_BLOCK-1)/CB_BLOCK+31)/32)*4);f->frozen=FALSE;f->frozenCount=0;WdfSpinLockRelease(StateLock);RtlZeroMemory(old,bytes);WdfSpinLockAcquire(StateLock);}
  WdfSpinLockRelease(StateLock);
 }
FinishCommand:
 WdfWaitLockRelease(LifecycleLock);WdfRequestCompleteWithInformation(request,status,information);
}
static NTSTATUS CreateControl(WDFDRIVER driver){
 PWDFDEVICE_INIT init;WDFDEVICE device;WDF_IO_QUEUE_CONFIG queue;WDF_FILEOBJECT_CONFIG files;WDF_OBJECT_ATTRIBUTES attributes;NTSTATUS status;
 DECLARE_CONST_UNICODE_STRING(name,L"\\Device\\TitanChangedBlockTrackingDemo");DECLARE_CONST_UNICODE_STRING(link,L"\\DosDevices\\TitanChangedBlockTrackingDemo");DECLARE_CONST_UNICODE_STRING(security,L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
 init=WdfControlDeviceInitAllocate(driver,&security);if(!init)return STATUS_INSUFFICIENT_RESOURCES;
 status=WdfDeviceInitAssignName(init,&name);if(!NT_SUCCESS(status)){WdfDeviceInitFree(init);return status;}
 WdfDeviceInitSetDeviceType(init,FILE_DEVICE_UNKNOWN);WdfDeviceInitSetCharacteristics(init,FILE_DEVICE_SECURE_OPEN,FALSE);WdfDeviceInitSetExclusive(init,TRUE);WdfDeviceInitSetIoType(init,WdfDeviceIoBuffered);
 WDF_FILEOBJECT_CONFIG_INIT(&files,Open,WDF_NO_EVENT_CALLBACK,Close);WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,OWNER);WdfDeviceInitSetFileObjectConfig(init,&files,&attributes);
 WDF_OBJECT_ATTRIBUTES_INIT(&attributes);attributes.ExecutionLevel=WdfExecutionLevelPassive;status=WdfDeviceCreate(&init,&attributes,&device);if(!NT_SUCCESS(status)){if(init)WdfDeviceInitFree(init);return status;}
 WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue,WdfIoQueueDispatchSequential);queue.PowerManaged=WdfFalse;queue.EvtIoDeviceControl=Command;
 status=WdfIoQueueCreate(device,&queue,WDF_NO_OBJECT_ATTRIBUTES,WDF_NO_HANDLE);if(NT_SUCCESS(status))status=WdfDeviceCreateSymbolicLink(device,&link);if(!NT_SUCCESS(status)){WdfObjectDelete(device);return status;}
 Control=device;WdfControlFinishInitializing(device);return STATUS_SUCCESS;
}
VOID FilterCleanup(WDFOBJECT object){FILTER* f=FilterContext(object);ULONG i;WDFDEVICE old=NULL;
 NT_ASSERT(KeGetCurrentIrql()==PASSIVE_LEVEL);_Analysis_assume_(KeGetCurrentIrql()==PASSIVE_LEVEL);
 if(!f->counted)return;WdfWaitLockAcquire(LifecycleLock,NULL);Stop(f);WdfSpinLockAcquire(StateLock);
 if(f->registered){for(i=0;i<CB_MAX_DEVICES;++i)if(Filters[i]==f)Filters[i]=NULL;f->registered=FALSE;}
 if(f->counted){f->counted=FALSE;NT_ASSERT(DeviceCount>0);--DeviceCount;}if(!DeviceCount&&!OwnerOpen){old=Control;Control=NULL;}
 WdfSpinLockRelease(StateLock);if(old)WdfObjectDelete(old);WdfWaitLockRelease(LifecycleLock);
}
VOID DriverCleanup(WDFOBJECT object){UNREFERENCED_PARAMETER(object);NT_ASSERT(KeGetCurrentIrql()==PASSIVE_LEVEL);_Analysis_assume_(KeGetCurrentIrql()==PASSIVE_LEVEL);TsRuntimeClose(&Runtime);}
NTSTATUS AddDevice(WDFDRIVER driver,PWDFDEVICE_INIT init){
 WDFDEVICE device;FILTER* f;WDF_OBJECT_ATTRIBUTES attributes,requests;WDF_IO_QUEUE_CONFIG queue;NTSTATUS status;ULONG i,bytes=0;
 WdfFdoInitSetFilter(init);WdfDeviceInitSetIoType(init,WdfDeviceIoDirect);WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requests,TRACK);WdfDeviceInitSetRequestAttributes(init,&requests);
 WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,FILTER);attributes.ExecutionLevel=WdfExecutionLevelPassive;attributes.EvtCleanupCallback=FilterCleanup;status=WdfDeviceCreate(&init,&attributes,&device);if(!NT_SUCCESS(status))return status;f=FilterContext(device);f->device=device;
 status=WdfDeviceQueryProperty(device,DevicePropertyPhysicalDeviceObjectName,sizeof(f->pdo),f->pdo,&bytes);if(!NT_SUCCESS(status)||bytes<2||bytes>sizeof(f->pdo))RtlZeroMemory(f->pdo,sizeof(f->pdo));f->pdo[CB_NAME-1]=0;
 WdfWaitLockAcquire(LifecycleLock,NULL);WdfSpinLockAcquire(StateLock);++DeviceCount;f->counted=TRUE;WdfSpinLockRelease(StateLock);
 if(!Runtime.api){status=TsRuntimeConnect(&Runtime);UNREFERENCED_PARAMETER(status);}if(!Control){status=CreateControl(driver);UNREFERENCED_PARAMETER(status);}
 WdfSpinLockAcquire(StateLock);for(i=0;i<CB_MAX_DEVICES;++i)if(!Filters[i]){f->id=++NextId;Filters[i]=f;f->registered=TRUE;break;}WdfSpinLockRelease(StateLock);WdfWaitLockRelease(LifecycleLock);
 WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue,WdfIoQueueDispatchParallel);queue.PowerManaged=WdfFalse;queue.EvtIoDefault=Forward;WDF_OBJECT_ATTRIBUTES_INIT(&attributes);attributes.ExecutionLevel=WdfExecutionLevelDispatch;attributes.SynchronizationScope=WdfSynchronizationScopeNone;
 return WdfIoQueueCreate(device,&queue,&attributes,WDF_NO_HANDLE);
}
NTSTATUS DriverEntry(PDRIVER_OBJECT object,PUNICODE_STRING registry){
 WDF_DRIVER_CONFIG config;WDFDRIVER driver;WDF_OBJECT_ATTRIBUTES attributes;NTSTATUS status;
 WDF_DRIVER_CONFIG_INIT(&config,AddDevice);WDF_OBJECT_ATTRIBUTES_INIT(&attributes);attributes.EvtDestroyCallback=DriverCleanup;
 status=WdfDriverCreate(object,registry,&attributes,&config,&driver);if(!NT_SUCCESS(status))return status;WDF_OBJECT_ATTRIBUTES_INIT(&attributes);attributes.ParentObject=driver;
 status=WdfSpinLockCreate(&attributes,&StateLock);if(!NT_SUCCESS(status))return status;return WdfWaitLockCreate(&attributes,&LifecycleLock);
}
