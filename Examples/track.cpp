#include "../SDK/ChangedBlocks.h"
#include <iostream>
int wmain(int argc,wchar_t** argv){
 std::cout<<"Track changed block ranges on an explicitly attached disposable scratch disk. This does not create a backup or baseline.\n";
 if(argc!=2){std::cout<<"Usage: ChangedBlocksExample.exe DEVICE_ID\n";return 2;}
 try{
  ChangedBlocks::Tracker tracker;
  tracker.Start(std::stoull(argv[1]),64ull*1024*1024);
  std::cout<<"Generate your own scratch-disk workload, then press Enter.\n";std::cin.get();
  auto changes=tracker.Checkpoint();
  changes.ForEachRange([](const auto& range){std::cout<<range.offset<<" + "<<range.length<<" bytes\n";return true;});
  tracker.Acknowledge(changes.state.generation);
  // Optional paid activation: tracker.Verification("YOUR-PRODUCT-KEY").
  return 0; // Scope exit closes the tracking owner; disk I/O remains unchanged.
 }catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}
}
