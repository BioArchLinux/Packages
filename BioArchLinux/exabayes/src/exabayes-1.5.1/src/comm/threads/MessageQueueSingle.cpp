#include "MessageQueueSingle.hpp"
#include "GlobalVariables.hpp" 
#include <algorithm>


using std::move; 


MessageQueueSingle::MessageQueueSingle( )
  : _first( new Node(std::vector<uint8_t>() ))
  , _divider(_first)
  , _last{_first}
  {
  }



MessageQueueSingle& MessageQueueSingle::operator=(MessageQueueSingle rhs) 
{
  swap(rhs, *this); 
  return *this; 
}


void swap(MessageQueueSingle& lhs, MessageQueueSingle& rhs)
{
  using std::swap; 

  swap(lhs._first, rhs._first); 

  rhs._divider = lhs._divider.exchange(rhs._divider); 
  rhs._last = lhs._last.exchange(rhs._last) ; 
} 



MessageQueueSingle::MessageQueueSingle( MessageQueueSingle&& rhs)  
  : _first(std::move(rhs._first))
  , _divider(rhs._divider.load())
  , _last(rhs._last.load())
{
  rhs._divider = nullptr; 
  rhs._last = nullptr; 
  rhs._first = nullptr; 
}


MessageQueueSingle::MessageQueueSingle(const MessageQueueSingle& rhs)  
  : _first(new Node(std::vector<uint8_t>()))
  , _divider(_first)
  , _last(_first)
{
} 


MessageQueueSingle::~MessageQueueSingle()
{
  while(_first != nullptr)
    {
      auto tmp = _first; 
      _first = tmp->_next; 
      delete tmp; 
    }
}

 
MessageQueueSingle::Node::Node(std::vector<byte> msg )
  : _message(msg)
  , _next(nullptr)
{
}

