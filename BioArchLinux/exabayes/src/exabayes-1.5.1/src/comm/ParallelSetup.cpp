#include <iostream> 
#include <sstream>

#include "GlobalVariables.hpp"
#include "SwapMatrix.hpp"
#include "SampleMaster.hpp"
#include "IncompleteMesh.hpp"
#include "ParallelSetup.hpp"
#include "common.h"

ParallelSetup::ParallelSetup(CommandLine& cl)
  : _threadResource(cl, this)
  , _globalComm(_threadResource.getTid2Ranking())
  , _runComm(_threadResource.getTid2Ranking())
  , _chainComm(_threadResource.getTid2Ranking())
  , _mesh(_globalComm.size(), cl.getNumRunParallel(), cl.getNumChainsParallel())
{  
}


ParallelSetup& ParallelSetup::operator=(ParallelSetup rhs)
{
  // std::cout << "assigning ParallelSetup " << std::endl; 
  swap(*this, rhs); 
  return *this; 
}


void swap(ParallelSetup &lhs, ParallelSetup &rhs)
{
  using std::swap; 
  swap(lhs._runComm, rhs._runComm); 
  swap(lhs._chainComm, rhs._chainComm);; 
  swap(lhs._mesh, rhs._mesh); 
  swap(lhs._globalComm, rhs._globalComm); 
  swap(lhs._threadResource, rhs._threadResource); 
}


void ParallelSetup::initializeRemoteComm(int argc, char **argv)
{
  RemoteComm::initComm(argc, argv);
}



void ParallelSetup::warn()  
{
  auto numWorkers = _globalComm.size() ;

  if(  numWorkers <  _mesh.getRunDimSize() * _mesh.getChainDimSize()    )
    {
      if( _globalComm.getRank() == 0)
	std::cout << "\nError: You attempted to run " << _mesh.getRunDimSize() << " runs in parallel for which " <<  _mesh.getChainDimSize() <<  " coupled chains should be run in parallel. This requires at least " << (_mesh.getRunDimSize() * _mesh.getChainDimSize() ) << " processes (in the best case a multiple of this number). " << PROGRAM_NAME << " was started with " << _globalComm.size() << " processes though.\n" <<  std::endl; 
      exitFunction(-1, true);
    }

  if(  numWorkers % ( _mesh.getRunDimSize() * _mesh.getChainDimSize()) != 0 ) 
    {
      if(_globalComm.getRank() == 0 )
	std::cout << "\tWARNING! The number of processes and number chains run in parallel\n"
		  << "\tare not a multiple of the overall nmuber of processes. Parts of the\n"
		  << "\tcode may be executed less efficiently.\n" ; 
    }
  
}



void ParallelSetup::initialize( )
{
  warn();
  assert(_globalComm.isValid()); 

  auto coords = _mesh.getCoordinates(_globalComm.getRank());

  // sanity check 
  auto rank = _mesh.getRankFromCoordinates(coords);
  assert(int(rank) == _globalComm.getRank()); 

  // create runComm 
  auto ranks = std::vector<int>{}; 
  auto colors = std::vector<int>{}; 
  for(int i = 0; i < _threadResource.getNumThreads(); ++i)
    {
      auto c = _mesh.getCoordinates(_globalComm.getRank() + i ); 
      ranks.push_back(getRankInRun(c)); 
      colors.push_back(c[0]) ; 
    }
  _runComm = _globalComm.split(colors, ranks); 
  assert(_runComm.isValid()); 
  

  // create chainComm 
  ranks = std::vector<int>{}; 
  colors = std::vector<int>{};
  for(int i = 0; i < _threadResource.getNumThreads(); ++i)
    {
      auto c = _mesh.getCoordinates(_globalComm.getRank() + i); 
      colors.push_back(getColorInChain(c)); 
      ranks.push_back(c[2]); 
    }
  _chainComm = _runComm.split(colors, ranks); 
  assert(_chainComm.isValid());


  if(_threadResource.getNumThreads() > 1 && not _globalComm.haveThreadSupport())
    {
      if(isYggdrasil)
	{
	  if(isGlobalMaster())
	    std::cout << "You instructed " << PROGRAM_NAME << " to use threads. However, " << PROGRAM_NAME << " has not been compiled with thread support. Most likely, you do not have pthreads installed. Most likely, you still can use this executable without threads (i.e., without -T x). "  << std::endl; 
	  exitFunction(1, true);
	}
      else 
	{
	  if(isGlobalMaster() )
	    std::cout << "You instructed " << PROGRAM_NAME << " to use threads. However either " << PROGRAM_NAME << " has not been compiled with thread support (e.g., because the pthreads is not installed) or your MPI implementation does not support multi-threading." << std::endl; 
	  exitFunction(1, true); 
	}
    }
}


void ParallelSetup::releaseThreads()
{
  _threadResource.releaseThreads();
}


std::string ParallelSetup::printLoadBalance(const TreeAln& traln, nat numRun, nat numCoupled )  
{
  auto &&ss = std::stringstream{};
  ss << SOME_FIXED_PRECISION; 
  if(isGlobalMaster())
    ss <<  "load distribution (rank,coords,#numParts,#numPatterns,chainsPerRun):"  << std::endl;
  
  nat sumA = 0;   
  nat sumB = 0; 
  for(nat i = 0; i < traln.getNumberOfPartitions(); ++i)
    {
      auto& partition = traln.getPartition(i); 
      if(partition.getWidth() > 0)
	++sumA; 
      sumB += partition.getWidth(); 
    }

  auto coords = _mesh.getCoordinates(_globalComm.getRank());

  ss << "[ " << _globalComm.getRank() << " ] " ; 
  ss  <<  "\t["   <<  coords[0] << "," << coords[1] << "," << coords[2] << "]\t" << sumA << "\t" << sumB << "\t"; 

  bool isFirst = true; 
  for(nat i = 0; i < numRun; ++i )
    {
      if(not isMyRun(i))
	continue; 
      for(nat j = 0; j < numCoupled; ++j)
	{
	  if(not isMyChain(i,j))
	    continue; 
	  ss << (not isFirst ? "," : "") << "("<< i << "," << j << ")" ; 
	  isFirst = false; 
	}
    }
  ss << std::endl; 
  
  if(sumA == 0)
    {
      ss << std::endl << "WARNING: there is a process that does not evaluate any portion of the\n"
	 << "data. You could decrease the number of processes."  << std::endl; 
    }
  

  auto str = ss.str();
  auto myData =  std::vector<char>(begin(str), end(str));
  auto allData = _globalComm.gatherVariableLength(myData); 
  
  auto result = std::string(begin(allData), end(allData)); 
  result += "\n" ; 
  
  return result; 
}




std::tuple<nat,std::vector<char> > 
ParallelSetup::serializeAllChains(std::vector<CoupledChains> &runs, CommFlag commFlags) const
{
  auto serialized = std::vector<char>{}; 
  // serializing all chains that are assigned to me  
  int lengthOfChainStream = 0; 
  bool isFirst = true ; 
  for(auto &run : runs)
    {
      for(nat i = 0; i < run.getChains().size() ;++i)
	{
	  const Chain& chain = run.getChains()[i]; 
	  if(isMyChain(run.getRunid(), i))
	    {
	      auto &&ss = std::ostringstream{}; 
	      chain.serializeConditionally( ss, commFlags ); 
	      auto chainSer =  ss.str();

	      if(isFirst)
		lengthOfChainStream = int(chainSer.size()); 
	      isFirst = false; 
	      assert(nat(lengthOfChainStream) == chainSer.size()); 

	      serialized.insert(end(serialized), begin(chainSer), end(chainSer)); 
	    }
	}
    }

  // TODO 
  assert(not isFirst); 

  return std::make_tuple(lengthOfChainStream,serialized); 
}


std::tuple<nat, std::vector<char> > 
ParallelSetup::serializeSwapMatrices(std::vector<CoupledChains>& runs, CommFlag &commFlags ) const 
{
  auto serialized = std::vector<char>{};
  int lengthOfSwap = 0;   
  if( ( commFlags & CommFlag::SWAP ) != CommFlag::NOTHING )
    {
      bool isFirst = true; 
      for(auto &run : runs)
	{
	  if(isMyRun(run.getRunid()))
	    {
	      auto && part = std::stringstream{}; 
	      run.getSwapInfo().serialize(part); 

	      auto asString = part.str();
	      
	      if(isFirst)
		lengthOfSwap = int(asString.size());
	      
	      isFirst = false; 
	      assert(lengthOfSwap == int(asString.size())); 
	      serialized.insert(end(serialized), begin(asString), end(asString)); 
	    }
	}
      assert(not isFirst); 
    }

  return std::make_tuple(lengthOfSwap, serialized);
}


// TODO i guess we communicate the swap matrix mutliple times, this could be saved
void ParallelSetup::synchronizeChainsAtMaster( std::vector<CoupledChains>& runs, CommFlag commFlags) 
{
#if 0   
  ss << "Communicating " ; 
  if( ( commFlags & CommFlag::Swap ) != CommFlag::NOTHING)
    ss << " SWAP, " ; 
  if( ( commFlags & CommFlag::PrintStat )  != CommFlag::NOTHING)
    ss << " PRINTSTAT, "; 
  if( ( commFlags & CommFlag::Tree ) != CommFlag::NOTHING)
    ss << " TREE, "; 
  if( ( commFlags & CommFlag::Proposals ) != CommFlag::NOTHING)
    ss << " PROPOSALS, "; 
  ss << std::endl; 
  blockingPrint(chainLeaderComm, ss.str()); 
  ss.str(""); 
#endif

  // everybody serializes their data 
  auto myData = std::vector<char>{}; 
  auto lengthOfChainStream = 0; 
  std::tie(lengthOfChainStream, myData) = serializeAllChains(runs, commFlags); 
  
  nat lengthOfSwap = 0; 
  auto swapSer = std::vector<char>{}; 
  std::tie(lengthOfSwap, swapSer) = serializeSwapMatrices(runs, commFlags); 
  myData.insert(end(myData), begin(swapSer), end(swapSer)); 

  // only leaders communicate 
  auto countsPerProc = std::vector<int>(_globalComm.size() , 0 );
  auto displPerProc = std::vector<int>(_globalComm.size() , 0 ); 
  for(auto i = 0u; i< _globalComm.size() ; ++i)
    {
      auto coords = _mesh.getCoordinates(i); 
      if(coords[2] == 0 )
	countsPerProc[i] = int(myData.size()); 

      if(i != 0 )
	displPerProc[i] = displPerProc[i-1] + countsPerProc[i]; 
    }

  auto allData = _globalComm.gatherVariableKnownLength( countsPerProc[_globalComm.getRank()] == 0 ? std::vector<char>{} : myData , countsPerProc, displPerProc );
  auto streamIter = begin(allData); 

  // everybody deserializes 
  if(isGlobalMaster())
    {
      auto ranksofLeaders = std::vector<nat> {}; 
      for(int i = 0; i < int( _globalComm.size()); ++i)
	{
	  auto coords =  _mesh.getCoordinates(i);
	  if(coords[2] == 0) 
	    ranksofLeaders.push_back(i); 
	}

      // accumulate swap matrices 
      auto sws = std::vector<SwapMatrix>{}; 
      for(nat i = 0; i < runs.size() ;++i)
	sws.emplace_back(runs[0].getChains().size());

      for(auto rankOfChainLeader : ranksofLeaders)
	{
	  for(auto &run : runs)
	    {
	      for(nat i = 0; i < run.getChains().size(); ++i)
		{		  
		  if(isMyChain(rankOfChainLeader, run.getRunid(), i))
		    {		      
		      auto &chain = run.getChains().at(i);
		      auto &&iss =  std::istringstream{std::string(streamIter, streamIter + lengthOfChainStream )}; 
		      streamIter += lengthOfChainStream; 
		      chain.deserializeConditionally(iss, commFlags); 
		    }
		}
	    }
	  
	  if( ( commFlags & CommFlag::SWAP ) != CommFlag::NOTHING )
	    {
	      for(auto &run : runs)
		{
		  if(isMyRun(rankOfChainLeader, run.getRunid()))
		    { 
		      auto&& iss = std::istringstream{std::string{streamIter, streamIter + lengthOfSwap}};
		      streamIter += lengthOfSwap; 
		      auto sw = SwapMatrix(run.getChains().size()); 
		      sw.deserialize(iss); 

		      // HACK: with more efficient communication, we
		      // would need to do this only once

		      if(isRunLeader(rankOfChainLeader))
			sws[run.getRunid()] += sw ; 
		    }
		}
	    }
	}
      
      if( ( commFlags & CommFlag::SWAP )  != CommFlag::NOTHING)
	{
	  // set the accumulated swap matrices 
	  for(auto &run : runs)
	    run.setSwapInfo(sws.at(run.getRunid()));
	}
    }
  else 
    {
      // for reproducibility. Otherwise runs are not reproducible any
      // more with chain-level parallelism
      if(  ( commFlags & CommFlag::TREE ) != CommFlag::NOTHING )
      	{
      	  for(auto &run : runs)
      	    {
      	      nat ctr = 0; 
      	      for(auto &chain : run.getChains())
      		{
      		  if(isMyChain(run.getRunid(), ctr))
      		    {
      		      auto &&oss = std::ostringstream{};
      		      chain.serializeConditionally(oss, commFlags );
      		      auto &&iss = std::istringstream{oss.str()}; 
      		      chain.deserializeConditionally(iss,  commFlags);
      		    }
      		  ++ctr;
      		}
      	    }
      	}
    }

}


void ParallelSetup::finalize()
{
  RemoteComm::finalize();
}


bool 
ParallelSetup::isMyChain(nat gRank, nat runid, nat chainIndex) const 
{
  auto hisCoords= _mesh.getCoordinates(gRank);
  return isMyRun(gRank, runid ) &&  ( chainIndex % getChainsParallel()) == hisCoords[1]; 
}


bool 
ParallelSetup::isMyChain(nat runid, nat chainIndex) const 
{
  return isMyChain(_globalComm.getRank(), runid, chainIndex); 
}


bool 
ParallelSetup::isMyRun(nat runid) const 
{
  return isMyRun(_globalComm.getRank(), runid);
}

bool 
ParallelSetup::isMyRun(nat gRank, nat runid) const 
{
  auto hisCoords = _mesh.getCoordinates(gRank);
  return (runid % getRunsParallel() )  == hisCoords[0]; 
}


std::string
ParallelSetup::sendRecvChain(const CoupledChains& run, nat myIndex, nat otherChainIndex, std::string myChainSer, CommFlag flags )  
{
  assert(0); 
}


void ParallelSetup::blockingPrint( Communicator &comm,std::string ss )
{
  int myRank = comm.getRank();
  auto size = comm.size(); 

  // MEH 
  if(size == 1 )
    {
      std::cout << "[ " << myRank << " ] " << ss << std::endl; 
      std::cout.flush();
      return; 
    }

  if(myRank != 0)
    {
      int res = comm.receive<int>(myRank-1,0);
      assert(res == 1 ); 
    }

  std::cout << "[ " << myRank << " ] " << ss << std::endl; 
  std::cout.flush();
  
  if(myRank != int(size - 1)) 
    comm.send<int>(1, myRank + 1 , 0 ); 
  else 
    comm.send<int>(1,0,0 ); 
    
  if( myRank == 0  )
    {
      auto res =  comm.receive<int>(int(size-1),0);
      assert(res == 1); 
    }
}


bool ParallelSetup::isRunLeader(nat gRank) const 
{  
  auto coords = _mesh.getCoordinates(_globalComm.getRank());
  return coords[1] == 0 && coords[2] == 0; 
}  


bool ParallelSetup::isChainLeader() const
{
  auto coords = _mesh.getCoordinates(_globalComm.getRank()); 
  return coords[2] == 0; 
}


bool ParallelSetup::isGlobalMaster() const 
{
  auto coords = _mesh.getCoordinates(_globalComm.getRank()); 
  return coords[0] == 0 
    && coords[1] == 0
    && coords[2] == 0; 
}


bool ParallelSetup::swapIsLocal(nat chainIdA, nat chainIdB, nat runId ) const 
{
  assert(chainIdA != chainIdB); 

  auto rankA = chainIdToLeaderRank(runId, chainIdA);  
  auto rankB = chainIdToLeaderRank(runId, chainIdB); 

  auto remoteRankA =  _globalComm.mapToRemoteRank(rankA); 
  auto remoteRankB =_globalComm.mapToRemoteRank(rankB);  

  return remoteRankA ==  remoteRankB; 
}


nat ParallelSetup::chainIdToLeaderRank(nat runId, nat chainId) const  
{
  auto pBatch = nat(runId % _mesh.getRunDimSize()); 
  auto cBatch = nat(chainId % _mesh.getChainDimSize ()); 
  
  auto coords = std::array<nat,3> {{ pBatch, cBatch, 0 }}; 
  return _mesh.getRankFromCoordinates(coords);
}


int ParallelSetup::getRankInRun( std::array<nat,3> coords) const 
{
  auto localRootCoords = std::array<nat,3> { { coords[0], 0, 0  }}; 
  return _mesh.getRankFromCoordinates(coords)  - _mesh.getRankFromCoordinates(localRootCoords); 
} 


int ParallelSetup::getColorInChain(std::array<nat,3> coords) const 
{
  return  int(coords[0] * _mesh.getChainDimSize() + coords[1]); 
}

std::array<nat,3> ParallelSetup::getMyCoordinates() const
{
  return _mesh.getCoordinates(_globalComm.getRank()); 
}


Communicator& ParallelSetup::getChainComm() 
{ 
  return _chainComm; 
}


Communicator& ParallelSetup::getGlobalComm() 
{
  return _globalComm; 
}


Communicator& ParallelSetup::getRunComm() 
{
  return _runComm; 
}



IncompleteMesh ParallelSetup::getMesh() const 
{
  return _mesh;
}


bool ParallelSetup::isRunLeader() const
{
  return isRunLeader( _globalComm.getRank() ) ; 
}  


size_t ParallelSetup::getChainsParallel() const 
{
  return _mesh.getChainDimSize(); 
}


size_t ParallelSetup::getRunsParallel() const
{
  return _mesh.getRunDimSize(); 
}



void ParallelSetup::abort(int code, bool waitForAll)
{
  Communicator::abort(code, waitForAll); 
}



std::ostream& operator<<(std::ostream& out, const ParallelSetup& rhs)
{
  out <<  "GLOB: " << rhs._globalComm
      << "\nRUN: " << rhs._runComm
      << "\nCHAIN: " << rhs._chainComm; 
  return out; 
}

uint64_t ParallelSetup::getMaxTag()
{
  return RemoteComm::getMaxTag();
}


void ParallelSetup::pinThreads()
{
  // should also work for numThreads == 1, but let's not take the risk 
  if(_threadResource.getNumThreads() > 1 )
    {
      auto procsPerNode = _globalComm.getProcsPerNode(); 
      _threadResource.pinThreads(procsPerNode,_globalComm.getRemoteComm().getRank() ); 
    }
}
