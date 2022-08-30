#include "RemoteCommImpl.hpp"

#include <set>

#include "CommRequest.hpp"
#include "CommRequestImpl.hpp"
#include "GlobalVariables.hpp"
#include "RemoteComm.hpp"

uint64_t RemoteComm::Impl::_maxTagValue = 0 ; 

RemoteComm::Impl::Impl(const RemoteComm::Impl& rhs)
  : _comm{MPI_COMM_WORLD}
{
  MPI_Comm_dup(rhs._comm, &_comm); 
}

RemoteComm::Impl& RemoteComm::Impl::operator=( RemoteComm::Impl rhs)
{
  swap(*this, rhs);
  return *this; 
}
  
void swap(RemoteComm::Impl &lhs, RemoteComm::Impl& rhs)
{
  std::swap(lhs._comm, rhs._comm);
}


RemoteComm::Impl::~Impl()
{
  MPI_Comm_free(&_comm);
}


int RemoteComm::Impl::getRank() const 
{
  int result = 0; 
  MPI_Comm_rank(_comm, &result);
  return result; 
}


size_t RemoteComm::Impl::size() const 
{
  int result = 0; 
  MPI_Comm_size(_comm, &result); 
  return size_t(result); 
}

  
bool RemoteComm::Impl::isValid() const
{
  return _comm != MPI_COMM_NULL; 
}


void RemoteComm::Impl::waitAtBarrier() const
{
  MPI_Barrier(_comm);
} 


RemoteComm::Impl RemoteComm::Impl::split(const std::vector<int> &color, const std::vector<int> &rank) const
{
  auto result = RemoteComm::Impl{}; 
  MPI_Comm_split(_comm, color[0], rank[0], &result._comm);
  return result; 
}


template<> bool RemoteComm::Impl::broadcastVar(bool var, int root)  
{
  char elem = var ? 1 : 0; 
  auto res = broadcastVar(elem, root); 
  return res == 1 ; 
}


template<typename T>
T RemoteComm::Impl::broadcastVar(T var, int root)  
{
  MPI_Bcast(&var, 1, mpiType<T>::value, root, _comm); 
  return var;  
}
template uint64_t RemoteComm::Impl::broadcastVar(uint64_t var, int root) ; 
template uint32_t RemoteComm::Impl::broadcastVar(uint32_t var, int root) ; 


void RemoteComm::Impl::createSendRequest(std::vector<char> array, int dest, int tag, CommRequest& req)
{
  // using friendship here. I have barely an idea, how to do this properly...  
  auto &impl = *(req._impl);
  impl._array = array; 
  assert(impl._array.size() != 0 ) ;
  MPI_Isend(  (void*)impl._array.data(), int(impl._array.size()), MPI_CHAR, dest, tag, _comm, &(impl._req)); 
}

 
void RemoteComm::Impl::createRecvRequest(int src, int tag, nat length, CommRequest &req )
{
  auto &impl = *(req._impl);
  impl._array = std::vector<char>(length); 
  assert(impl._array.size() != 0 ) ;
  MPI_Irecv( impl._array.data(), int(impl._array.size()), MPI_CHAR, src, tag, _comm, &impl._req); 
}  


nat RemoteComm::Impl::getNumberOfPhysicalNodes()   
{
  auto result = 0u;  
  auto name = std::vector<char>(MPI_MAX_PROCESSOR_NAME, '\0'); 
  auto len = 0; 
  auto root = 0; 

  MPI_Get_processor_name(name.data(), &len);
  
  // std::cout << SHOW(name) << std::endl; 
  
  auto everything = gather(name, root); 
  
  // std::cout << SHOW(everything) << std::endl; 
  
  if( getRank() == 0 )
    {
      auto uniqStr = std::unordered_set<std::string>{}; 
      for(auto i = 0; i < int(size()) ; ++i)
	uniqStr.insert(std::string(begin(everything) + (i * MPI_MAX_PROCESSOR_NAME), begin(everything) + ((i+1) * MPI_MAX_PROCESSOR_NAME) )); 
      result = nat(uniqStr.size()); 
    }
  
  result = broadcastVar(result, root); 

  return result; 
}


void RemoteComm::Impl::initComm(int argc, char **argv )
{
  int provided = 0; 
  int numThreads= -1; 
  for(int i = 0 ; i < argc ; ++i)
    {
      if(std::string(argv[i]).compare("-T") == 0 && i + 1 < argc  )
	numThreads = std::stoi(std::string{argv[i+1]}); 
    }

  if(numThreads > 1)
    {
      MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided); 
      int rank = 0; 
      MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
      if( provided != MPI_THREAD_MULTIPLE && rank == 0 )
	{
	  std::cout << "You called the mpi version of " << PROGRAM_NAME << " with threads (-T x).  However, the MPI implementation you installed\n" 
		    << "does not support threads (MPI_THEAD_MULTIPLE to be exact). Please\n"
		    << "choose a different MPI installation OR do not use threads in"
		    << "combination with MPI OR configure and compile a MPI implementation\n"
		    << "yourself." << std::endl ;
	}
      else if ( provided == MPI_THREAD_MULTIPLE && rank == 0)
	{
	  std::cout << "Initialized multi-threaded MPI environment" << std::endl; 
	}
    }
  else 
    {
      MPI_Init(&argc, &argv); 
    }

  int flag = 0; 
  MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &_maxTagValue, &flag);
  assert(flag != 0 ); 
}



bool RemoteComm::Impl::haveThreadSupport()  const 
{
  int provided  = 0; 
  MPI_Query_thread(&provided); 
  return provided == MPI_THREAD_MULTIPLE ; 
}




void RemoteComm::Impl::finalize()
{
  MPI_Finalize();
}


void RemoteComm::Impl::abort(int code, bool waitForAll)
{
  if(_masterThread == std::this_thread::get_id())
    {
      if(waitForAll)
	{
	  MPI_Barrier(MPI_COMM_WORLD); 
	  MPI_Finalize();
	}
      else 
	{
	  MPI_Abort(MPI_COMM_WORLD, code); 
	}
    }
}


void RemoteComm::Impl::waitAtBarrier()
{
  MPI_Barrier(_comm);
}


