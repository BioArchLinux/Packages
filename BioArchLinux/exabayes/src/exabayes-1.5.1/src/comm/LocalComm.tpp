#include <cassert>

#include "GlobalVariables.hpp"


template<typename T> 
std::vector<T>
LocalComm::scatterVariableKnownLength( std::vector<T> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root)   
{
  // does not need to be terrible efficient...(binary tree of course would be better) 

  assert(root == 0); 

  int myRank = getRank(); 
  int myCol = getColor();
  int myIdx = getIdx();
  int rootIdx = getIdx(myCol, root); 
  auto result = std::vector<T>();

  if(myRank == root)
    {
      for(auto i = 0; i < int(size()) ;++i)
	{
	  if(i == root)
	    continue; 
	  
	  auto toSend = std::vector<T>(begin(allData) + displPerProc[i], begin(allData) + displPerProc[i] + countsPerProc[i]);
	  _newMessages[myIdx][i].produce(toSend); 
	}
      
      result.insert(end(result), begin(allData) + displPerProc[myRank] , begin(allData) + displPerProc[myRank] + countsPerProc[myRank]); 
    }
  else 
    {
      bool found = false; 
      while(not found)
	std::tie(found, result) =   _newMessages[rootIdx][myRank].consume<T>(myRank);
    }

  return result; 
}
 


template<typename T>
std::vector<T>
LocalComm::gatherVariableKnownLength(std::vector<T> myData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc , int root)   
{
  assert(root == 0); 

  DATA_COMBINE_FUN gatherer = [](std::vector<T>& acc, typename std::vector<T>::const_iterator  beginDon, typename std::vector<T>::const_iterator endDon)
    {
      acc.insert(end(acc), beginDon, endDon);
    }; 

  return commTreeUp(myData,root, gatherer) ; 
} 




template<typename T> 
std::vector<T>
LocalComm::broadcast(std::vector<T> array, int root ) 
{
  if(size() == 1 )
    return array; 
  else 
    return commTreeDownAsync(array, root);
}


template<typename T>
std::vector<T> LocalComm::reduce(std::vector<T> data, int root )
{
  // sorry this rank translation is too much effort now..
  assert(root == 0); 

  if(size() == 1 )
    return data; 
  else 
    {

      DATA_COMBINE_FUN reducer = [](  std::vector<T> &acc,  typename std::vector<T>::const_iterator beginDonator, typename std::vector<T>::const_iterator endDonator)
	{
	  std::transform( 
			 begin(acc), end(acc),
			 beginDonator, begin(acc),
			 std::plus<double>() 	     
			  ); 
	}; 

      return commTreeUpAsync(data,root, reducer); 
    } 
}

template<typename T> 
std::vector<T> 
LocalComm::gatherVariableLength(std::vector<T> myData, int root )  
{
  assert(root == 0); 
  DATA_COMBINE_FUN gatherer = []( std::vector<T> &acc, typename std::vector<T>::const_iterator beginDonator,  typename std::vector<T>::const_iterator endDonator)
    {
      acc.insert(end(acc), beginDonator, endDonator); 
    }; 

  return commTreeUp(myData,root, gatherer) ; 
}




template<typename T>
std::vector<T> LocalComm::allReduce(std::vector<T> myData)
{
  int root = 0 ; 
  if(size() > 1)
    {
      myData = reduce(myData, root);
      myData = broadcast(myData,root); 
    }
  return myData; 
}


template<typename T>
void LocalComm::postAsyncMessage(const std::vector<T> &message, int tag, int runBatch)
{
  // TODO multiple queues could be better here ... 
  auto readers = std::vector<int>(size(),1); 
  _mgsPerTag.at(runBatch).at(tag).produce(message, readers); 
}


template<typename T>
std::tuple<bool,std::vector<T> > LocalComm::readAsyncMessage(int tag, int runBatch)
{
  return _mgsPerTag.at(runBatch).at(tag).consume<T>(getRank());
}



// not using a combine function right now, since we only need this for
// the bcast, where it must be as efficient as possible
template<typename T>
std::vector<T> LocalComm::commTreeDownAsync(std::vector<T> data, int root)
{
  assert(root == 0); 
  
  int myIdx = getIdx();
  // int myCol = getColor();
  int myRank = getRank(); 
  assert(nat(root) < size());

  // auto &myMsg = _newMessages.at(myIdx); 

  auto recvFrom = 0; 
  auto sendTo = std::vector<int>( );

  for( int offset = 1 ;
       ( myRank % offset) == 0 && size_t(offset) < size()  ;
       offset *= 2 )
    {
      if( (myRank % ( offset * 2 ) ) == 0 )
	{
	  // receive downwards 
	  auto hisRank = myRank+offset; 
	  
	  if( size() <= size_t(hisRank) )
	    continue;
	  
	  if(hisRank != myRank)
	    sendTo.push_back(hisRank)  ; 
	}
      else 
	{
	  recvFrom = myRank - offset; 
	}
    }

  // first receive 
  auto msg = std::vector<T>{}; 
  if(myRank != root)
    {
      bool gotMsg  = false; 
      auto hisIdx = getIdx(getColor(), recvFrom); 
      auto &hisMsg =  _newMessages.at(hisIdx);
      while(not gotMsg) 
	std::tie(gotMsg, msg) = hisMsg[myRank].consume<T>(myRank); // must be the real rank to avoid confusion 
    }
  else 
    msg = data; 

  for(auto v : sendTo)
    _newMessages[myIdx][v].produce(msg); 

  // then send everything 
  return msg; 
}



template<typename T>
std::vector<T> LocalComm::commTreeUpAsync(std::vector<T> data, int root, DATA_COMBINE_FUN fun)
{
  // opmitized for the case that the root is rank 0; TODO implement
  // that more generally
  assert(root == 0); 

  int myIdx = getIdx();
  // int myCol = getColor();
  int myRank = getRank(); 
  assert(nat(root) < size());

  // auto &myMsg = _newMessages.at(myIdx); 

  auto sendTo = 0; 
  auto receiveFrom = std::vector<int>();

  for( auto offset = 1u ;
       ( myRank % offset) == 0u && offset < size()  ;
       offset *= 2 )
    {
      if( (myRank % ( offset * 2 ) ) == 0 )
	{
	  // receive downwards 
	  auto hisRank = myRank+offset; 
	  
	  if( size() <= hisRank )
	    continue;

	  receiveFrom.push_back(hisRank); 

	}
      else 
	{
	  // send upwards 
	  sendTo = myRank - offset; 
	}
    }

  
  // first receive everything 
  while(not receiveFrom.empty())
    {
      // auto iter = receiveFrom
      auto iter = begin(receiveFrom); 
      while( iter != end(receiveFrom) )
	{
	  auto hisRank = *iter; 
	  auto hisIdx = getIdx(getColor(), hisRank);
	  auto &hisMsg = _newMessages.at(hisIdx);

	  bool gotMsg  = false; 
	  auto msg = std::vector<T>{}; 
	  std::tie(gotMsg, msg) = hisMsg[myRank].consume<T>(myRank); // must be the real rank to avoid confusion 
	  
	  if(gotMsg)
	    {
	      fun(data, begin(msg), end(msg)); 
	      iter = receiveFrom.erase(iter); 
	    }
	  else 
	    ++iter; 
	}
    }

  // then send 
  _newMessages[myIdx][sendTo].produce(data);

  return data; 
}



template<typename T>
std::vector<T> LocalComm::commTreeUp(std::vector<T> data, int root, DATA_COMBINE_FUN fun)
{
  // opmitized for the case that the root is rank 0; TODO implement
  // that more generally
  assert(root == 0); 

  int myIdx = getIdx();
  // int myCol = getColor();
  int myRank = getRank(); 
  assert(root < int(size()));

  auto &myMsg = _newMessages.at(myIdx); 

  for( int offset = 1 ;
       ( myRank % offset) == 0 && offset < int(size())  ;
       offset *= 2 )
    {
      if( (myRank % ( offset * 2 ) ) == 0 )
	{
	  // receive downwards 
	  auto hisRank = myRank+offset; 
	  
	  if( int(size()) <= hisRank )
	    continue;

	  auto hisIdx = getIdx(getColor(), hisRank);
	  auto &hisMsg = _newMessages.at(hisIdx);

	  bool gotMsg  = false; 
	  auto msg = std::vector<T>{}; 
	  while(not gotMsg)
	    std::tie(gotMsg, msg) = hisMsg[myRank].consume<T>(myRank); // must be the real rank to avoid confusion 

	  fun(data, begin(msg), end(msg)); 
	}
      else 
	{
	  // send upwards 
	  auto hisRank = myRank - offset; 
	  auto readers = std::vector<int>(size(),0); 
	  readers[hisRank] = 1 ; 

	  myMsg[hisRank].produce(data); 
	}
    }

  return data; 
}
