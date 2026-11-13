#include "SDK/ChangedBlocks.h"
#include "WireTypes.h"
#include <map>
struct ProductSession{

std::unique_ptr<ChangedBlocks::Tracker> tracker;
ChangedBlocks::Tracker& Get(){if(!tracker)tracker=std::make_unique<ChangedBlocks::Tracker>();return *tracker;}
std::unique_ptr<ChangedBlocks::Snapshot> snapshot;

Json Invoke(const std::string& op,const Json& a){

if(op=="check"){auto state=FromWire<CB_STATE>(a.at("state"));return {{"valid",ChangedBlocks::Valid(state)},{"value",EncodeWire(state)}};}
if(op=="disks"){Json list=Json::array();for(const auto& d:ChangedBlocks::Disks())list.push_back({{"instance",Utf8(d.instance)},{"name",Utf8(d.name)},{"pdo",Utf8(d.pdo)},{"interface_path",Utf8(d.interface_path)},{"number",d.number},{"capacity",std::to_string(d.capacity)},{"eligible",d.eligible},{"attached",d.attached},{"reason",Utf8(d.reason)}});return list;}
if(op=="devices")return VectorWire(Get().Devices());
if(op=="verification"){CheckMessage(Get().Verification(a.at("key").get<std::string>()));return Json::object();}
if(op=="start")return EncodeWire(Get().Start(Identifier(a,"device_id"),Identifier(a,"capacity")));
if(op=="status")return EncodeWire(Get().Status());
if(op=="stop"){if(tracker)tracker->Stop();snapshot.reset();return Json::object();}
if(op=="checkpoint"){snapshot=std::make_unique<ChangedBlocks::Snapshot>(Get().Checkpoint());return EncodeWire(snapshot->state);}
if(op=="bitmap_page"){Require(bool(snapshot),"Take a checkpoint first");auto offset=Identifier(a,"offset");auto length=Number(a,"length",16384,65536);Require(offset<=snapshot->bitmap.size() && length<=snapshot->bitmap.size()-offset,"Bitmap range out of bounds");return {{"offset",std::to_string(offset)},{"data",std::vector<unsigned char>(snapshot->bitmap.begin()+offset,snapshot->bitmap.begin()+offset+length)}};}
if(op=="ranges"){Require(bool(snapshot),"Take a checkpoint first");auto skip=Number(a,"skip");auto limit=Number(a,"limit",256,4096);Require(limit>0,"Limit must be positive");Json ranges=Json::array();std::uint64_t index=0;bool complete=snapshot->ForEachRange([&](const ChangedBlocks::Range& range){if(index++<skip)return true;ranges.push_back({{"offset",std::to_string(range.offset)},{"length",std::to_string(range.length)}});return ranges.size()<limit;});return {{"ranges",ranges},{"next_skip",skip+ranges.size()},{"complete",complete}};}
if(op=="acknowledge"){Get().Acknowledge(Identifier(a,"generation"));snapshot.reset();return Json::object();}

throw SdkError(50,"Unknown operation");}
};
static Runtime<ProductSession> runtime;
extern "C" __declspec(dllexport) unsigned changed_block_tracking_sdk_json_open(std::uint64_t* id)noexcept{return runtime.Open(id);}
extern "C" __declspec(dllexport) unsigned changed_block_tracking_sdk_json_close(std::uint64_t id)noexcept{return runtime.Close(id);}
extern "C" __declspec(dllexport) char* changed_block_tracking_sdk_json_call(std::uint64_t id,const char* request,std::uint32_t length)noexcept{return runtime.Call(id,request,length);}
extern "C" __declspec(dllexport) void changed_block_tracking_sdk_json_free(char* response)noexcept{delete[] response;}
