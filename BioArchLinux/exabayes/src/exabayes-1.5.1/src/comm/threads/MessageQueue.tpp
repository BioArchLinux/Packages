#include "GlobalVariables.hpp"

#include <algorithm>

#include <cstring>
#include <iostream>
#include <cassert>

using std::move;


template<typename T>
void MessageQueue::produce(std::vector<T> msg, const std::vector<int> &forWhom)
{
  auto tmpMsg = std::vector<byte>(reinterpret_cast<byte*>(msg.data()), reinterpret_cast<byte*>(msg.data() + msg.size())); 
  
  (*_last)._next = new Node(tmpMsg, forWhom); 
  _last = (*_last)._next; 

  while( _first != _divider )
    {
      Node *tmp = _first; 
      _first = _first->_next; 
      delete tmp; 
    }
}


template<typename T>
std::tuple<bool,std::vector<T> >  MessageQueue::consume(int threadId) 
{
  while(_consumerLock.exchange(true)) // LOCK
    ; 
  
  Node* dIter = _divider; 
  
  auto result = std::vector<T>(); 

  bool found = false; 
  bool inFirst = true; 
  while(dIter != _last)
    {
      if( dIter->_next->_whoReads.at(threadId) > 0 ) 
	{
	  auto num = 0; 
	  auto msg =  std::vector<byte>();
	  std::tie(num, msg) = dIter->_next->getConsumed(threadId); 

	  if(inFirst && num == 0 )
	    _divider = (*_divider)._next; 

	  assert(msg.size() % sizeof(T) == 0); 
	  result.resize(msg.size() / sizeof(T)); 
	  memcpy(result.data(), msg.data(),  msg.size()); 
	  found = true; 
	  break; 
	}

      dIter = dIter->_next;
      inFirst = false; 

      // much faster with this break here ... (would induce more code
      // changes) but with break, it can consume multiple elements 

      // break; 
    }

  _consumerLock.exchange(false); // UNLOCK
  return std::make_pair(found, result);
}

