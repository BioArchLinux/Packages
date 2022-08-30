#include "BandWidthTest.hpp"
#include "GlobalVariables.hpp"

#include <cassert>
// #include "time.hpp"


nat BandWidthTest::determineOptimum(Communicator& comm, std::string fileName)
{
#if 1 
  // assert(0); 
  // return 0; 
  // return 1; 
#else 
  auto numNodes = comm.getNumberOfPhysicalNodes();

  auto &&in = std::ifstream(fileName, std::ios::binary); 
  in.seekg(0, std::ios::end);
  auto toRead = in.tellg() / 100 ; // read 1% of the file 

  auto bestBw = 0; 
  uint32_t bestNum = 0; 

  for(auto i = int(numNodes) ; i <= comm.getSize() ; i *= 2 )
    {
      auto tp = getTimePoint(); 
      comm.waitAtBarrier();
      nat everyNth = comm.getSize() / i;
      if(comm.getRank() % everyNth == 0)
	{
	  auto mySubRank = comm.getRank() / everyNth; 
	  auto toReadNow = toRead / i ;
	  auto tmp = std::vector<uint8_t>(toReadNow, '\0'); 
	  in.seekg(toReadNow * mySubRank, std::ios::beg); 
	  in.read((char*)tmp.data(), toReadNow * sizeof(uint8_t));
	}
      comm.waitAtBarrier();
      auto dur = getDuration(tp); 
      auto bw = ((double(toRead) / 1024.) / 1024. ) / dur; 
      tout << i << " procs: " << bw << " MiB/sec" << std::endl;

      if(bestBw < bw)
	{
	  bestBw = bw; 
	  bestNum = everyNth;  
	}
    }

  bestNum = comm.broadcastVar(bestNum); 
  
  tout << "best bandwidth was "<< bestBw <<  ". This means that every  " << bestNum << " process should be used for reading." << std::endl; 

  return bestNum;
#endif
  return 0; 
} 
