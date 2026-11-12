#include "ChangedBlocks.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <limits>
namespace ChangedBlocks {namespace {
std::string Error(const char* stage,DWORD code){char text[512]{};FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,code,MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),text,512,nullptr);return std::string(stage)+" failed (Windows error "+std::to_string(code)+"): "+(text[0]?text:"Operation failed.");}
CB_REQUEST Request(uint64_t device=0){CB_REQUEST r{};r.size=sizeof(r);r.version=CB_VERSION;r.device=device;return r;}
uint64_t BlockCount(uint64_t capacity){return(capacity+CB_BLOCK-1)/CB_BLOCK;}
unsigned Count(uint8_t value){unsigned count=0;while(value){value=static_cast<uint8_t>(value&(value-1));++count;}return count;}
void Usable(const CB_STATE& state){if(!(state.flags&CB_FLAG_ACTIVE)||(state.flags&(CB_FLAG_INVALID|CB_FLAG_ACCESS_EXPIRED)))throw std::runtime_error("Tracking history is invalid or access expired. Treat all changes as unknown; deliberately establish a new baseline before starting again. Activation cannot repair missing history.");}
}
bool Valid(const CB_INVENTORY& value)noexcept{if(value.size!=sizeof(value)||value.version!=CB_VERSION||value.count>CB_MAX_DEVICES||value.reserved)return false;for(unsigned i=0;i<value.count;++i){const auto& device=value.devices[i];if(!device.id)return false;bool ended=false,nonempty=false;for(auto c:device.pdo_name){if(!c)ended=true;else{if(ended||c<32)return false;nonempty=true;}}if(!ended||!nonempty)return false;for(unsigned j=0;j<i;++j)if(value.devices[j].id==device.id)return false;}return true;}
bool Valid(const CB_STATE& s)noexcept{
 if(s.size!=sizeof(s)||s.version!=CB_VERSION||(s.flags&~15u)||s.block_bytes!=CB_BLOCK||!s.device||!s.session||!s.generation||!s.capacity||s.capacity>CB_MAX_CAPACITY||s.capacity%512)return false;
 if((s.flags&CB_FLAG_ACCESS_EXPIRED)&&!(s.flags&CB_FLAG_INVALID))return false;
 const auto blocks=BlockCount(s.capacity);if(s.bitmap_bytes!=(blocks+7)/8||s.bitmap_bytes>2097152||s.changed_blocks>blocks||(!(s.flags&CB_FLAG_FROZEN)&&s.changed_blocks))return false;
 return true;
}
bool Snapshot::ForEachRange(const std::function<bool(const Range&)>& visitor)const{
 if(!Valid(state)||!(state.flags&CB_FLAG_FROZEN)||state.flags&(CB_FLAG_INVALID|CB_FLAG_ACCESS_EXPIRED)||bitmap.size()!=state.bitmap_bytes||!visitor)throw std::invalid_argument("A complete usable checkpoint and range visitor are required.");
 const auto blocks=BlockCount(state.capacity);uint64_t start=0;bool active=false;
 for(uint64_t i=0;i<=blocks;++i){const bool set=i<blocks&&(bitmap[static_cast<size_t>(i/8)]&(1u<<(i%8)))!=0;if(set&&!active){active=true;start=i;}if(!set&&active){const auto offset=start*CB_BLOCK,end=std::min(state.capacity,i*CB_BLOCK);if(!visitor({offset,end-offset}))return false;active=false;}}
 return true;
}
struct Tracker::State {HANDLE handle=INVALID_HANDLE_VALUE;uint64_t id=0,session=0;std::mutex mutex;~State(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}};
Tracker::Tracker():state_(std::make_unique<State>()){}
Tracker::~Tracker()=default;
void Tracker::Command(DWORD operation,const CB_REQUEST& request,void* output,DWORD capacity){
 if(state_->handle==INVALID_HANDLE_VALUE){state_->handle=CreateFileW(LR"(\\.\TitanChangedBlockTracking)",GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);if(state_->handle==INVALID_HANDLE_VALUE)throw std::runtime_error(Error("Open CBT driver (install package, attach the explicit scratch disk and run as administrator)",GetLastError()));}
 DWORD returned=0;if(!DeviceIoControl(state_->handle,operation,const_cast<CB_REQUEST*>(&request),sizeof(request),output,capacity,&returned,nullptr)){const DWORD error=GetLastError();if(operation==CB_CHECKPOINT&&error==ERROR_BUSY)throw std::runtime_error("Checkpoint is temporarily unavailable because a potentially mutating disk request is still pending (Windows error 170). Wait for that workload to complete, then retry. Tracking remains active; no checkpoint was acknowledged.");throw std::runtime_error(Error(("CBT operation "+std::to_string(operation)).c_str(),error));}if(returned!=capacity)throw std::runtime_error("The CBT driver returned an unexpected response size.");
}
std::vector<CB_DEVICE> Tracker::Devices(){std::lock_guard<std::mutex> lock(state_->mutex);CB_INVENTORY inventory{};try{Command(CB_ENUM,Request(),&inventory,sizeof(inventory));if(!Valid(inventory))throw std::runtime_error("The CBT driver returned invalid device inventory.");if(!state_->id){CloseHandle(state_->handle);state_->handle=INVALID_HANDLE_VALUE;}return {inventory.devices,inventory.devices+inventory.count};}catch(...){if(!state_->id&&state_->handle!=INVALID_HANDLE_VALUE){CloseHandle(state_->handle);state_->handle=INVALID_HANDLE_VALUE;}throw;}}
std::string Tracker::Verification(std::string_view key){return TitanSdk::Verification(TITAN_SDK_CHANGED_BLOCK_TRACKING,key);}
CB_STATE Tracker::Start(uint64_t device,uint64_t capacity){
 std::lock_guard<std::mutex> lock(state_->mutex);if(state_->id)throw std::runtime_error("Stop the current tracking owner before starting a new baseline.");if(!device||!capacity||capacity>CB_MAX_CAPACITY||capacity%512)throw std::invalid_argument("Use an enumerated device and its exact sector-aligned capacity.");
 try{CB_INVENTORY inventory{};Command(CB_ENUM,Request(),&inventory,sizeof(inventory));if(!Valid(inventory))throw std::runtime_error("The CBT driver returned invalid device inventory.");const CB_DEVICE* selected=nullptr;for(unsigned i=0;i<inventory.count;++i)if(inventory.devices[i].id==device)selected=&inventory.devices[i];if(!selected)throw std::invalid_argument("The selected opaque device ID is no longer present.");
 const std::wstring pdo(reinterpret_cast<const wchar_t*>(selected->pdo_name));bool accepted=false;for(const auto& disk:Disks())if(!_wcsicmp(disk.pdo.c_str(),pdo.c_str())&&disk.eligible&&disk.attached&&disk.capacity==capacity)accepted=true;if(!accepted)throw std::runtime_error("This initial edition accepts only its explicitly attached, unpartitioned 64 MiB Titan Storage Fault Lab scratch disk. Physical/system/unknown disks are refused.");
 auto request=Request(device);request.capacity=capacity;CB_STATE status{};Command(CB_START,request,&status,sizeof(status));if(!Valid(status)||status.device!=device||status.capacity!=capacity)throw std::runtime_error("The driver returned invalid start state; owner closed to invalidate tracking.");Usable(status);state_->id=device;state_->session=status.session;return status;
 }catch(...){if(state_->handle!=INVALID_HANDLE_VALUE)CloseHandle(state_->handle);state_->handle=INVALID_HANDLE_VALUE;state_->id=state_->session=0;throw;}
}
CB_STATE Tracker::Query(){if(!state_->id)throw std::runtime_error("Start tracking only after establishing the required baseline.");CB_STATE status{};Command(CB_STATUS,Request(state_->id),&status,sizeof(status));if(!Valid(status)||status.device!=state_->id||status.session!=state_->session)throw std::runtime_error("Tracking state no longer matches this session; establish a new baseline.");return status;}
CB_STATE Tracker::Status(){std::lock_guard<std::mutex> lock(state_->mutex);return Query();}
Snapshot Tracker::Checkpoint(){
 std::lock_guard<std::mutex> lock(state_->mutex);auto status=Query();Usable(status);if(!(status.flags&CB_FLAG_FROZEN)){Command(CB_CHECKPOINT,Request(state_->id),&status,sizeof(status));if(!Valid(status)||status.device!=state_->id||status.session!=state_->session||!(status.flags&CB_FLAG_FROZEN))throw std::runtime_error("The driver returned an invalid frozen checkpoint.");Usable(status);}
 Snapshot result;result.state=status;result.bitmap.reserve(static_cast<size_t>(status.bitmap_bytes));uint64_t position=0;
 while(position<status.bitmap_bytes){auto request=Request(state_->id);request.generation=status.generation;request.offset=position;CB_BITMAP_PAGE page{};Command(CB_READ_MAP,request,&page,sizeof(page));if(!Valid(page.state)||page.state.device!=status.device||page.state.session!=status.session||page.state.generation!=status.generation||page.state.capacity!=status.capacity||page.state.bitmap_bytes!=status.bitmap_bytes||page.state.changed_blocks!=status.changed_blocks||page.state.flags!=status.flags||page.offset!=position||page.reserved||!page.bytes||page.bytes>CB_PAGE||page.bytes>status.bitmap_bytes-position)throw std::runtime_error("The frozen bitmap page is malformed or changed generation.");Usable(page.state);result.bitmap.insert(result.bitmap.end(),page.data,page.data+page.bytes);position+=page.bytes;}
 const auto blocks=BlockCount(status.capacity);if(blocks%8&&(result.bitmap.back()&static_cast<uint8_t>(0xffu<<(blocks%8))))throw std::runtime_error("The bitmap contains bits outside the disk capacity.");uint64_t count=0;for(auto byte:result.bitmap)count+=Count(byte);if(count!=status.changed_blocks)throw std::runtime_error("The bitmap bit count differs from the driver's frozen checkpoint.");return result;
}
void Tracker::Acknowledge(uint64_t generation){std::lock_guard<std::mutex> lock(state_->mutex);auto status=Query();Usable(status);if(!(status.flags&CB_FLAG_FROZEN)||!generation||generation!=status.generation)throw std::invalid_argument("Only the exact current frozen generation can be acknowledged.");auto request=Request(state_->id);request.generation=generation;Command(CB_ACK,request,nullptr,0);}
void Tracker::Stop()noexcept{std::lock_guard<std::mutex> lock(state_->mutex);if(state_->handle!=INVALID_HANDLE_VALUE){if(state_->id){auto request=Request(state_->id);DWORD returned=0;DeviceIoControl(state_->handle,CB_STOP,&request,sizeof(request),nullptr,0,&returned,nullptr);}CloseHandle(state_->handle);state_->handle=INVALID_HANDLE_VALUE;}state_->id=state_->session=0;}
}
