#ifndef _THREAD_RESOURCE_HPP
#define _THREAD_RESOURCE_HPP

#include "threadDefs.hpp"

#include <memory>
#include <vector>
#include <unordered_map>

#include "common.h"

class CommandLine; 
class ParallelSetup; 

////////////////////////////////////////////////////////////////////////////////
//                              THREAD RESOURCE                               //
////////////////////////////////////////////////////////////////////////////////
class ThreadResource
{
  //////////////////////////////////////////////////////////////////////////////
  //                               PUBLIC TYPES                               //
  //////////////////////////////////////////////////////////////////////////////
public:
  typedef std::unordered_map<std::thread::id,int> threadToTid; 

  //////////////////////////////////////////////////////////////////////////////
  //                             PUBLIC INTERFACE                             //
  //////////////////////////////////////////////////////////////////////////////
public: 
  ThreadResource(CommandLine &cl, ParallelSetup* pl);
  // ___________________________________________________________________________
  ThreadResource(ThreadResource&& rhs) = default;
  // ___________________________________________________________________________
  virtual ~ThreadResource(); 
  // ___________________________________________________________________________
  ThreadResource&                       operator=(ThreadResource rhs) ; 
  // ___________________________________________________________________________
  friend void                           swap(ThreadResource &lhs,
                                             ThreadResource& rhs); 

  int                                   getNumThreads() const
  { return int(_threads.size()) + 1; }
  // ___________________________________________________________________________
  static void                           threadStart(CommandLine& cl,
                                                    ParallelSetup* pl);
  // ___________________________________________________________________________
  static void                           releaseThreads();
  // ___________________________________________________________________________
  threadToTid                           getTid2Ranking() const
  {return _tid2rank; }
  // ___________________________________________________________________________
  void                                  pinThreads(int numProcPerNode,
                                                   int remoteRank); 

  //////////////////////////////////////////////////////////////////////////////
  //                               PRIVATE DATA                               //
  //////////////////////////////////////////////////////////////////////////////
private: 
  std::vector<std::thread>              _threads; 
  threadToTid                           _tid2rank; // thread rank 
  static volatile bool                  _threadsAreReleased; 
}; 

#endif
