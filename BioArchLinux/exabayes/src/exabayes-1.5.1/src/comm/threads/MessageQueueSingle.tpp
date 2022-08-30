#include "GlobalVariables.hpp"

#include <algorithm>

#include <cstring>
#include <iostream>
#include <cassert>

using std::move;


template<typename T>
void MessageQueueSingle::produce(std::vector<T> msg)
{
  auto tmpMsg = std::vector<byte>(reinterpret_cast<byte*>(msg.data()), reinterpret_cast<byte*>(msg.data() + msg.size())); 
  
  (*_last)._next = new Node(tmpMsg); 
  _last = (*_last)._next; 

  while( _first != _divider )
    {
      Node *tmp = _first; 
      _first = _first->_next; 
      delete tmp; 
    }
}


template<typename T>
std::tuple<bool,std::vector<T> >  MessageQueueSingle::consume(int threadId) 
{
  auto result = std::vector<T>(); 
  bool found = false; 

  if(_divider != _last)
    {
      auto msg =  std::vector<byte>();
      msg = (*_divider)._next->_message; 

      _divider = (*_divider)._next; 

      assert(msg.size() % sizeof(T) == 0); 
      result.resize(msg.size() / sizeof(T)); 
      memcpy(result.data(), msg.data(),  msg.size()); 
      found = true; 
    }

  return std::make_tuple(found, result); 
}

