#include "../SDK/ChangedBlocks.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <random>
#include <stdexcept>
namespace {
unsigned checks=0;
void Require(bool value,const char* error){++checks;if(!value)throw std::runtime_error(error);}
CB_STATE State(uint64_t capacity){CB_STATE s{};s.size=sizeof(s);s.version=CB_VERSION;s.flags=CB_FLAG_ACTIVE|CB_FLAG_FROZEN;s.block_bytes=CB_BLOCK;s.device=s.session=s.generation=1;s.capacity=capacity;s.bitmap_bytes=(((capacity+CB_BLOCK-1)/CB_BLOCK)+7)/8;return s;}
}
int main(){try{
 auto valid=State(64ull*1024*1024);Require(ChangedBlocks::Valid(valid),"Known state rejected");
 {auto pending=valid;pending.inflight=9;pending.write_requests=0;Require(ChangedBlocks::Valid(pending),"Pending mutations inherited before Start were incorrectly tied to active-session write count");}
 for(unsigned i=0;i<12;++i){auto s=valid;switch(i){case 0:s.size=0;break;case 1:s.version=0;break;case 2:s.flags=0x100;break;case 3:s.block_bytes=512;break;case 4:s.device=0;break;case 5:s.session=0;break;case 6:s.generation=0;break;case 7:s.capacity=CB_MAX_CAPACITY+512;break;case 8:s.capacity=513;break;case 9:s.bitmap_bytes=0;break;case 10:s.changed_blocks=1025;break;case 11:s.flags=CB_FLAG_ACTIVE;s.changed_blocks=1;break;}Require(!ChangedBlocks::Valid(s),"Malformed driver state accepted");}
 CB_INVENTORY inventory{};inventory.size=sizeof(inventory);inventory.version=CB_VERSION;Require(ChangedBlocks::Valid(inventory),"Empty inventory rejected");inventory.count=1;inventory.devices[0].id=1;const wchar_t pdo[]=L"\\Device\\CBTTest";memcpy(inventory.devices[0].pdo_name,pdo,sizeof(pdo));Require(ChangedBlocks::Valid(inventory),"Known inventory rejected");inventory.count=2;inventory.devices[1]=inventory.devices[0];Require(!ChangedBlocks::Valid(inventory),"Duplicate opaque device ID accepted");inventory.count=17;Require(!ChangedBlocks::Valid(inventory),"Oversize inventory accepted");
 std::mt19937_64 random(0x434254);for(unsigned i=0;i<10000;++i){const uint64_t blocks=1+random()%4096;const uint64_t capacity=blocks*CB_BLOCK-(random()%128)*512;ChangedBlocks::Snapshot snapshot;snapshot.state=State(capacity);const auto actual=(capacity+CB_BLOCK-1)/CB_BLOCK;snapshot.bitmap.assign(static_cast<size_t>(snapshot.state.bitmap_bytes),0);std::vector<bool> expected(static_cast<size_t>(actual));for(uint64_t bit=0;bit<actual;++bit)if(random()%9==0){expected[static_cast<size_t>(bit)]=true;snapshot.bitmap[static_cast<size_t>(bit/8)]|=static_cast<uint8_t>(1u<<(bit%8));++snapshot.state.changed_blocks;}
  std::vector<bool> observed(static_cast<size_t>(actual));uint64_t previous=0,count=0;bool first=true;Require(snapshot.ForEachRange([&](const ChangedBlocks::Range& range){Require(range.length&&range.offset%CB_BLOCK==0&&range.offset<capacity&&range.length<=capacity-range.offset,"Decoded range out of disk bounds");Require(first||range.offset>previous,"Decoded ranges overlap or were not coalesced");first=false;previous=range.offset+range.length;for(uint64_t bit=range.offset/CB_BLOCK;bit<((range.offset+range.length+CB_BLOCK-1)/CB_BLOCK);++bit){observed[static_cast<size_t>(bit)]=true;++count;}return true;}),"Complete range scan stopped unexpectedly");Require(observed==expected&&count==snapshot.state.changed_blocks,"Independent bitset/range comparison failed");
 }
 ChangedBlocks::Snapshot malformed;malformed.state=valid;malformed.bitmap={0};bool refused=false;try{malformed.ForEachRange([](const auto&){return true;});}catch(const std::invalid_argument&){refused=true;}Require(refused,"Wrong bitmap length accepted");
 std::cout<<checks<<" bitmap, state and independent range checks passed; no driver or disk was opened.\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}}
