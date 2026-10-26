#include "ChangedBlocks.h"
#include <setupapi.h>
#include <winioctl.h>
#include <initguid.h>
#include <ntdddisk.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <mutex>
#include <cwchar>

namespace {
struct Handle {HANDLE h=INVALID_HANDLE_VALUE;~Handle(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}};
struct Devices {HDEVINFO h=INVALID_HANDLE_VALUE;~Devices(){if(h!=INVALID_HANDLE_VALUE)SetupDiDestroyDeviceInfoList(h);}};
std::string Error(const char* stage,DWORD code) {
 char text[512]{};FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,code,MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),text,512,nullptr);
 return std::string(stage)+" (Windows error "+std::to_string(code)+"): "+(text[0]?text:"Operation failed.");
}
std::wstring Property(HDEVINFO h,SP_DEVINFO_DATA& device,DWORD property) {
 DWORD type=0,bytes=0;SetupDiGetDeviceRegistryPropertyW(h,&device,property,&type,nullptr,0,&bytes);
 if(type!=REG_SZ||bytes<2||bytes>65536)return {};
 std::vector<wchar_t> text(bytes/2+1,0);
 if(!SetupDiGetDeviceRegistryPropertyW(h,&device,property,&type,reinterpret_cast<BYTE*>(text.data()),bytes,nullptr))return {};
 return text.data();
}
bool Attached(HDEVINFO h,SP_DEVINFO_DATA& device) {
 DWORD type=0,bytes=0;SetupDiGetDeviceRegistryPropertyW(h,&device,SPDRP_UPPERFILTERS,&type,nullptr,0,&bytes);
 if(type!=REG_MULTI_SZ||bytes<4||bytes>65536)return false;
 std::vector<wchar_t> filters(bytes/2+2,0);
 if(!SetupDiGetDeviceRegistryPropertyW(h,&device,SPDRP_UPPERFILTERS,&type,reinterpret_cast<BYTE*>(filters.data()),bytes,nullptr))return false;
 for(const wchar_t* p=filters.data();*p;p+=wcslen(p)+1)if(_wcsicmp(p,L"TitanChangedBlockTracking")==0)return true;
 return false;
}
bool OSDisks(std::vector<DWORD>& numbers) {
 wchar_t windows[MAX_PATH]{},root[MAX_PATH]{},volume[MAX_PATH]{};
 if(!GetWindowsDirectoryW(windows,MAX_PATH)||!GetVolumePathNameW(windows,root,MAX_PATH)||!GetVolumeNameForVolumeMountPointW(root,volume,MAX_PATH))return false;
 std::wstring path(volume);if(!path.empty()&&path.back()==L'\\')path.pop_back();
 Handle handle;handle.h=CreateFileW(path.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);if(handle.h==INVALID_HANDLE_VALUE)return false;
 std::vector<BYTE> buffer(65536);DWORD returned=0;
 if(!DeviceIoControl(handle.h,IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,nullptr,0,buffer.data(),static_cast<DWORD>(buffer.size()),&returned,nullptr)||returned<sizeof(VOLUME_DISK_EXTENTS))return false;
 auto extents=reinterpret_cast<VOLUME_DISK_EXTENTS*>(buffer.data());
 if(!extents->NumberOfDiskExtents||extents->NumberOfDiskExtents>128||returned<FIELD_OFFSET(VOLUME_DISK_EXTENTS,Extents)+extents->NumberOfDiskExtents*sizeof(DISK_EXTENT))return false;
 for(DWORD i=0;i<extents->NumberOfDiskExtents;++i)numbers.push_back(extents->Extents[i].DiskNumber);
 return true;
}
bool Unpartitioned(HANDLE file) {
 std::vector<BYTE> bytes(65536);DWORD returned=0;
 if(!DeviceIoControl(file,IOCTL_DISK_GET_DRIVE_LAYOUT_EX,nullptr,0,bytes.data(),static_cast<DWORD>(bytes.size()),&returned,nullptr)||returned<FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX,PartitionEntry))return false;
 auto layout=reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX*>(bytes.data());
 if(layout->PartitionCount>128||returned<FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX,PartitionEntry)+layout->PartitionCount*sizeof(PARTITION_INFORMATION_EX))return false;
 for(DWORD i=0;i<layout->PartitionCount;++i)if(layout->PartitionEntry[i].PartitionLength.QuadPart>0)return false;
 return true;
}
bool OwnedSyntheticAncestor(const std::wstring& instance){
 DEVINST node=0;if(CM_Locate_DevNodeW(&node,const_cast<wchar_t*>(instance.c_str()),CM_LOCATE_DEVNODE_NORMAL)!=CR_SUCCESS)return false;
 for(unsigned depth=0;depth<16;++depth){
  wchar_t ids[2048]{},service[256]{};ULONG bytes=sizeof(ids),type=0;bool hardware=false;
  if(CM_Get_DevNode_Registry_PropertyW(node,CM_DRP_HARDWAREID,&type,ids,&bytes,0)==CR_SUCCESS&&type==REG_MULTI_SZ&&bytes<sizeof(ids))
   for(const wchar_t* p=ids;*p;p+=wcslen(p)+1)if(!_wcsicmp(p,L"ROOT\\TitanStorageFaultLab"))hardware=true;
  bytes=sizeof(service);type=0;
  if(hardware&&CM_Get_DevNode_Registry_PropertyW(node,CM_DRP_SERVICE,&type,service,&bytes,0)==CR_SUCCESS&&type==REG_SZ&&bytes<sizeof(service)&&!_wcsicmp(service,L"TitanStorageFaultLab"))return true;
  DEVINST parent=0;if(CM_Get_Parent(&parent,node,0)!=CR_SUCCESS)return false;node=parent;
 }
 return false;
}
bool OwnedSyntheticHandle(HANDLE file){
 DWORD returned=0;GET_LENGTH_INFORMATION length{};DISK_GEOMETRY geometry{};
 if(!DeviceIoControl(file,IOCTL_DISK_GET_LENGTH_INFO,nullptr,0,&length,sizeof(length),&returned,nullptr)||returned!=sizeof(length)||length.Length.QuadPart!=64*1024*1024)return false;
 if(!DeviceIoControl(file,IOCTL_DISK_GET_DRIVE_GEOMETRY,nullptr,0,&geometry,sizeof(geometry),&returned,nullptr)||returned!=sizeof(geometry)||geometry.BytesPerSector!=512)return false;
 STORAGE_PROPERTY_QUERY query{};query.PropertyId=StorageDeviceProperty;query.QueryType=PropertyStandardQuery;std::vector<BYTE> bytes(4096,0);
 if(!DeviceIoControl(file,IOCTL_STORAGE_QUERY_PROPERTY,&query,sizeof(query),bytes.data(),static_cast<DWORD>(bytes.size()),&returned,nullptr)||returned<sizeof(STORAGE_DEVICE_DESCRIPTOR)||returned>bytes.size())return false;
 auto descriptor=reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(bytes.data());
 if(descriptor->Size<sizeof(STORAGE_DEVICE_DESCRIPTOR)||descriptor->Size>returned)return false;
 auto field=[&](DWORD offset){if(!offset||offset>=returned)return std::string();const char* text=reinterpret_cast<const char*>(bytes.data()+offset);size_t n=strnlen_s(text,returned-offset);if(n==returned-offset)return std::string();std::string value(text,n);while(!value.empty()&&value.back()==' ')value.pop_back();return value;};
 return field(descriptor->VendorIdOffset)=="TITANCSA"&&field(descriptor->ProductIdOffset)=="STORAGE FAULTLAB"&&field(descriptor->SerialNumberOffset)=="TITAN-SFL-0001";
}
}
namespace ChangedBlocks {
std::vector<Disk> Disks() {
 std::vector<Disk> result;std::vector<DWORD> os;bool knownOS=OSDisks(os);
 Devices devices;devices.h=SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
 if(devices.h==INVALID_HANDLE_VALUE)throw std::runtime_error(Error("Enumerate disk interfaces",GetLastError()));
 for(DWORD index=0;;++index){
  SP_DEVICE_INTERFACE_DATA iface{};iface.cbSize=sizeof(iface);
  if(!SetupDiEnumDeviceInterfaces(devices.h,nullptr,&GUID_DEVINTERFACE_DISK,index,&iface)){if(GetLastError()!=ERROR_NO_MORE_ITEMS)throw std::runtime_error(Error("Enumerate disk interface",GetLastError()));break;}
  DWORD needed=0;SetupDiGetDeviceInterfaceDetailW(devices.h,&iface,nullptr,0,&needed,nullptr);if(needed<sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)||needed>65536)continue;
  std::vector<BYTE> bytes(needed);auto detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(bytes.data());detail->cbSize=sizeof(*detail);
  SP_DEVINFO_DATA device{};device.cbSize=sizeof(device);if(!SetupDiGetDeviceInterfaceDetailW(devices.h,&iface,detail,needed,nullptr,&device))continue;
  wchar_t id[2048]{};if(!SetupDiGetDeviceInstanceIdW(devices.h,&device,id,2048,nullptr))continue;
  Disk disk{};disk.instance=id;disk.name=Property(devices.h,device,SPDRP_FRIENDLYNAME);if(disk.name.empty())disk.name=Property(devices.h,device,SPDRP_DEVICEDESC);
  disk.interface_path=detail->DevicePath;disk.pdo=Property(devices.h,device,SPDRP_PHYSICAL_DEVICE_OBJECT_NAME);disk.attached=Attached(devices.h,device);disk.number=MAXDWORD;
  Handle file;file.h=CreateFileW(detail->DevicePath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
  STORAGE_DEVICE_NUMBER number{};DWORD returned=0;
  if(file.h==INVALID_HANDLE_VALUE||!DeviceIoControl(file.h,IOCTL_STORAGE_GET_DEVICE_NUMBER,nullptr,0,&number,sizeof(number),&returned,nullptr)||returned!=sizeof(number)||number.DeviceType!=FILE_DEVICE_DISK){disk.reason=L"Disk identity/layout unavailable; administrator access may be required.";}
  else {
   disk.number=number.DeviceNumber;GET_LENGTH_INFORMATION capacity{};DWORD actual=0;if(DeviceIoControl(file.h,IOCTL_DISK_GET_LENGTH_INFO,nullptr,0,&capacity,sizeof(capacity),&actual,nullptr)&&actual==sizeof(capacity)&&capacity.Length.QuadPart>0)disk.capacity=static_cast<uint64_t>(capacity.Length.QuadPart);
   if(!knownOS)disk.reason=L"Cannot establish the Windows system disk identity.";
   else if(std::find(os.begin(),os.end(),number.DeviceNumber)!=os.end())disk.reason=L"Windows system disk: attachment refused.";
   else if(!Unpartitioned(file.h))disk.reason=L"Partitioned or unknown media: only unpartitioned disposable scratch disks are accepted.";
   else if(!OwnedSyntheticAncestor(disk.instance)||!OwnedSyntheticHandle(file.h))disk.reason=L"This edition attaches only the owned 64 MiB Titan Storage Fault Lab disk. Physical and other virtual disks are inventory-only.";
   else {disk.eligible=true;disk.reason=L"Owned unpartitioned 64 MiB Fault Lab scratch disk. Explicit attachment required.";}
  }
  result.push_back(std::move(disk));
 }
 return result;
}

std::string Utf8(const std::wstring& input) {
 if(input.size()>static_cast<size_t>((std::numeric_limits<int>::max)()))throw std::length_error("Text is too long.");
 int count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,input.data(),static_cast<int>(input.size()),nullptr,0,nullptr,nullptr);if(count<=0&&!input.empty())throw std::runtime_error("Invalid UTF-16 text.");
 std::string result(static_cast<size_t>(count),0);if(count)WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,input.data(),static_cast<int>(input.size()),result.data(),count,nullptr,nullptr);return result;
}
}
