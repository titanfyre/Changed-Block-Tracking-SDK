#pragma once
#include "CbtProtocol.h"
#include "License.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace ChangedBlocks {
struct Disk {std::wstring instance,name,pdo,interface_path;unsigned number=MAXDWORD;uint64_t capacity=0;bool eligible=false,attached=false;std::wstring reason;};
std::vector<Disk> Disks(); // Read-only inventory. Does not attach or write a disk.
struct Range {uint64_t offset=0,length=0;};
struct Snapshot {
 CB_STATE state{};std::vector<uint8_t> bitmap;
 // Stream coalesced byte ranges without expanding a large bitmap in memory.
 // Return false from the visitor to stop enumeration early.
 bool ForEachRange(const std::function<bool(const Range&)>& visitor)const;
};
bool Valid(const CB_INVENTORY&)noexcept;
bool Valid(const CB_STATE&)noexcept;
std::string Utf8(const std::wstring&);
class Tracker {
 struct State;std::unique_ptr<State> state_;
 void Command(DWORD,const CB_REQUEST&,void*,DWORD);
 CB_STATE Query();
public:
 Tracker();~Tracker();Tracker(const Tracker&)=delete;Tracker& operator=(const Tracker&)=delete;
 std::vector<CB_DEVICE> Devices();
 std::string Verification(std::string_view key);
 CB_STATE Start(uint64_t device,uint64_t capacity);
 CB_STATE Status(); // INVALID/EXPIRED is visible; never treat it as a valid map.
 // Return an existing frozen generation, or freeze a new one. A new checkpoint
 // fails with a detailed retry message while any possible mutation is pending.
 Snapshot Checkpoint();
 void Acknowledge(uint64_t generation); // Only after the consumer saved/used it.
 void Stop()noexcept; // Close owner and invalidate tracking; disk I/O keeps flowing.
};
}
