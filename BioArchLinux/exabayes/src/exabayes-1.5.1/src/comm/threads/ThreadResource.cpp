#include "ThreadResource.hpp"

#include "ParallelSetup.hpp"
#include "CommandLine.hpp"
#include <cstring>

extern void exa_main ( CommandLine &cl,  ParallelSetup* pl ); 

volatile bool ThreadResource::_threadsAreReleased = false; 

ThreadResource::ThreadResource(CommandLine& cl, ParallelSetup* pl)
  : _threads{}
  , _tid2rank{}
{
  auto haveThreadSupport = pl->getGlobalComm().haveThreadSupport(); 
  if(haveThreadSupport)
    {
      // this starts the threads and makes them wait in the function threadStart
      int highestRank = 0; 
      _tid2rank[MY_TID] = highestRank++; 
      for(int i = 1 ; i < cl.getNumThreads()  ; ++i)
	{
	  _threads.emplace_back(&threadStart, std::ref(cl), pl);
	  _tid2rank[_threads.back().get_id()] = highestRank++; 
	}
    }
  else if( not haveThreadSupport)
    {
      if(cl.getNumThreads() > 1 )
	std::cout << "You wanted to start threads using -T. However, "
                  << PROGRAM_NAME << " could not detect thread support. "
          " Will attempt to continue without threads."  << std::endl;
      
      auto highestRank = 0; 
      _tid2rank[MY_TID] = highestRank++; 
    }
}



void swap(ThreadResource &lhs, ThreadResource& rhs)
{
  using std::swap; 
  swap(lhs._threads, rhs._threads); 
  swap(lhs._tid2rank, rhs._tid2rank); 
  swap(lhs._threadsAreReleased, rhs._threadsAreReleased); 
} 


ThreadResource& ThreadResource::operator=(ThreadResource rhs) 
{
  swap(*this, rhs); 
  return *this; 
} 


void ThreadResource::pinThreads(int numProcPerNode, int remoteRank)
{
#ifndef __APPLE__
  cpu_set_t  mask;
  CPU_ZERO(&mask);

  auto pinTo =(remoteRank * numProcPerNode)   +   _tid2rank[MY_TID]; 
  // std::cout << SyncOut() << "pinnig thread  " << _tid2rank[MY_TID ]
  // << " of process "  << remoteRank << " to core "  << pinTo << std::endl; 

  CPU_SET(pinTo, &mask);
  int result = sched_setaffinity(0, sizeof(mask), &mask); 

  if(result != 0)
    {
      std::cout << SyncOut() << "Problem with pinning! Probably the number "
        "of processes and threads\n"
        "started on this machine exceeds the number of available cores. "
        "Thread\n"
        "pinning is disabled. In the worst case," << PROGRAM_NAME <<
        " will run substantially slower (use a tool like htop to monitor,\n"
        "whether all cores are loaded)." << std::endl; 
    }
#else 
  
#endif
}


void ThreadResource::threadStart(CommandLine& cl, ParallelSetup* pl)
{
  while(not _threadsAreReleased)
    ; 

  exa_main(cl, pl); 
}


void ThreadResource::releaseThreads()
{
  _threadsAreReleased = true; 
}


ThreadResource::~ThreadResource()
{
  // nat ctr = 0; 
  for(auto &t : _threads)
    t.join();
}

