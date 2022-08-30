#ifndef COMMCORENEW_H
#define COMMCORENEW_H

#include <cassert>

// this class *could* be used for implementing CRTP with communicators. However, the overhead is enormous. Instead use the more clumsy CommCore.hpp       


template<class DERIVED>
class CommCoreNew
{
public: 

  template<typename T> 
  std::vector<T>
  gather(std::vector<T> myData, nat root = 0 ) 
  {
    return static_cast<DERIVED*>(this)->gather_impl(myData,root); 
  } 

  template<typename T> 
  std::vector<T>
  gather_impl(std::vector<T> myData, nat root = 0 ) {assert(0); return {}; } 


  template<typename T> 
  std::vector<T>
  gatherVariableLength(std::vector<T> myData, int root = 0)  
  {
    return static_cast<DERIVED*>(this)->gatherVariableLength_impl(myData,root); 
  } 

  template<typename T>
  std::vector<T> gatherVariableKnownLength(std::vector<T> myData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc , int root = 0)   
  {
    return static_cast<DERIVED*>(this)->gatherVariableKnownLength_impl(myData, countsPerProc, displPerProc, root);
  }

  template<typename T> 
  std::vector<T>
  scatterVariableKnownLength( std::vector<T> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root) // 
  {
    return static_cast<DERIVED*>(this)->scatterVariableKnownLength_impl(allData, countsPerProc, displPerProc, root);
  }

  template<typename T> 		// done
  std::vector<T>
  broadcast(std::vector<T> array, int root = 0 )   
  {
    return static_cast<DERIVED*>(this)->broadcast_impl(array,root);
  }

  template<typename T>		// ok
  T  broadcastVar(T var, int root = 0 )    
  {
    return static_cast<DERIVED*>(this)->broadcastVar_impl(var, root);
  }

  template<typename T> 
  std::vector<T> 
  allReduce( std::vector<T> myValues) // 
  {
    return  static_cast<DERIVED*>(this)->allReduce_impl(myValues);
  }



  template<typename T>
  std::vector<T>
  reduce(std::vector<T> data, int root )  
  {
    return static_cast<DERIVED*>(this)->reduce_impl(data, root);
  } 

  
  template<typename T> 
  T  receive( int source, int tag )  
  {
    return static_cast<DERIVED*>(this)->receive_impl(source,tag);
  }
 
  template<typename T>  
  void send( T elem, int dest, int tag ) 
  {
    static_cast<DERIVED*>(this)->send_impl(elem,dest,tag); 
  }


  int getRank() const
  {
    return static_cast<DERIVED*>(this)->getRank_impl(); 
  }

  size_t size() const
  {
    return static_cast<DERIVED*>(this)->size_impl(); 
  } 

  bool isValid() const 
  {
    return static_cast<DERIVED*>(this)->isValid_impl();
  }


  bool haveThreadSupport() const 
  {
    return static_cast<DERIVED*>(this)->haveThreadSupport_impl();
  } 

  DERIVED split(const std::vector<int> &color, const std::vector<int> &rank) const
  {
    return static_cast<DERIVED*>(this)->split_impl(color, rank);
  } 

  void waitAtBarrier()
  {
    static_cast<DERIVED*>(this)->waitAtBarrier_impl();
  }

  static void finalize()
  {
    DERIVED::finalize_impl();
  }
  
  static void initComm(int argc, char **argv)
  {
    DERIVED::initComm_impl(argc,argv);
  }

  static void abort(int code, bool waitForAll )
  {
    DERIVED::abort_impl(code,waitForAll); 
  }
 
} ; 


#endif /* COMMCORENEW_H */
