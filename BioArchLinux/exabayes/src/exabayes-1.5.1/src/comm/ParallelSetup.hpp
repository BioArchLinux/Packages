#ifndef _PARALLEL_SETUP_PARA_HPP
#define _PARALLEL_SETUP_PARA_HPP

#include <unordered_map>
#include "CommandLine.hpp"
#include "IncompleteMesh.hpp"
#include "FlagType.hpp" 
#include "Communicator.hpp"

#include "ThreadResource.hpp"

#include "CommFlag.hpp"

class CoupledChains; 

// remember: the implementation of this class depends on whether we
// have MPI. Do NOT implement anything here inline.

class ParallelSetup
{
public: 
  ParallelSetup(CommandLine& cl); 
  ParallelSetup(ParallelSetup &&rhs) =default ; 
  ParallelSetup(const ParallelSetup& rhs) = delete ; 
  ParallelSetup& operator=( ParallelSetup rhs) ; 
  friend void swap(ParallelSetup &lhs, ParallelSetup &rhs); 

  void warn()  ; 

  /** 
      @brief initializes the examl environment  
   */ 
  void initialize(  ); 


  static void initializeRemoteComm(int argc, char **argv); 
  
  /** 
      @brief finalize the parallel environment 
   */ 
  static void finalize(); 
  /** 
      @brief gets the number of runs executed in parallel  
   */ 
  size_t getRunsParallel() const ; 
  /** 
      @brief gets the number of (coupled) chains executed in paralel (in addition to run-level parallelism)
   */ 
  size_t getChainsParallel() const ; 
  /** 
     @brief indicates whether the process should conduct output operatons (for his run) 
   */ 
  bool isRunLeader() const; 
  bool isRunLeader(nat gRank) const ; 
  /**
     @brief indicates whether a process is the leader in a batch of chains 
   */ 
  bool isChainLeader() const; 
  /** 
      @brief indicates whether the process is the process to conduct global logging output
   */ 
  bool isGlobalMaster() const ; 
  /** 
      @brief sends a serialized chain representation to the other involved process  
      @param run relevant run  
      @param otherChainIndex the index of the chain that is involved in the swapping attempt   
      @return serialized representation  of the chain we swap with 
   */ 
  std::string
  sendRecvChain(const CoupledChains& run, nat myIndex, nat otherChainIndex  , std::string myChainSer, CommFlag flags )  ; 

  nat chainIdToLeaderRank(nat runId, nat chainId) const ; 

  /** 
      @brief synchronize all core chain information at the master node // 
      @notice this only concerns the global masterprocess in a
      parallel environtment.  
  */ 
  void synchronizeChainsAtMaster( std::vector<CoupledChains>& runs, CommFlag commFlags) ;  

  /** 
      @brief a printing function that allows to print properly 
   */ 
  static void blockingPrint( Communicator &comm,std::string ss );  

  std::string printLoadBalance(const TreeAln& traln, nat numRun, nat numCoupled )  ; 

  /** 
      @brief indicates whether the chain belongs to a process
   */ 
  bool isMyChain(nat runid, nat chainIndex) const ; 
  bool isMyChain(nat gRank, nat runid, nat chainIndex )const;
  /** 
      @brief indicates whether the run belongs to a process
   */   
  bool isMyRun(nat runid) const ; 
  bool isMyRun(nat gRank, nat runid) const; 

  Communicator& getGlobalComm();
  Communicator& getRunComm() ; 
  Communicator& getChainComm(); 

  friend std::ostream& operator<<(std::ostream& out, const ParallelSetup& rhs); 

  IncompleteMesh getMesh() const; 
  
  int getRankInRun( std::array<nat,3> coords) const ; 

  std::array<nat,3> getMyCoordinates() const; 

  static void abort(int code, bool waitForAll);

  void releaseThreads(); 

  /** 
   * @brief determines, whether the swap can be executed within a
   * local communicator only
   */ 
  bool swapIsLocal(nat chainIdA, nat chainIdB, nat runId ) const ; 
  
  uint64_t getMaxTag(); 

  void pinThreads(); 

private: 			// METHODS
  auto serializeAllChains(std::vector<CoupledChains> &runs, CommFlag commFlags) const
    -> std::tuple<nat,std::vector<char>> ; 
  auto serializeSwapMatrices(std::vector<CoupledChains>& runs, CommFlag &commFlags ) const 
    -> std::tuple<nat, std::vector<char>> ; 
  
  int getColorInChain(std::array<nat,3> coords) const ; 

private:   			// ATTRIBUTES
  ThreadResource _threadResource; 

  Communicator _globalComm; 
  Communicator _runComm; 
  Communicator _chainComm; 

  IncompleteMesh _mesh; 
}; 


#endif
