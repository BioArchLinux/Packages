#ifndef _MESSAGE_QUEUE_SINGLE_HPP
#define _MESSAGE_QUEUE_SINGLE_HPP 

/** 
    @brief 
    this is a true lockless single producer, single consumer message queue. 
    
    see sutters article in drdobbs
 */ 

#include <vector>
#include <limits>
#include <atomic>


typedef uint8_t byte;  

class  MessageQueueSingle 
{
  struct Node; 

public: 
  MessageQueueSingle(); 
  MessageQueueSingle(const MessageQueueSingle& rhs) ; 
  MessageQueueSingle( MessageQueueSingle&& rhs) ; 
  MessageQueueSingle& operator=(MessageQueueSingle rhs); 
  friend void swap(MessageQueueSingle& lhs, MessageQueueSingle& rhs); 
  ~MessageQueueSingle(); 
  
  template<typename T>
  void produce(std::vector<T> msg); 
  template<typename T>
  std::tuple<bool,std::vector<T> >  consume(int myId); 

  
private: 
  Node* _first;			// producer only 
  // char pad[CACHE_LINE_SIZE - sizeof(Node*)]; 
  std::atomic<Node*> _divider; 	// shared 
  // char pad2[CACHE_LINE_SIZE - sizeof(std::atomic<Node*>)]; 
  std::atomic<Node*> _last; 	// shared 
}; 

struct MessageQueueSingle::Node 
{
  Node(std::vector<byte> msg ); 
  Node(const Node& rhs ) = default; 
  Node& operator=(const Node &rhs)  = default; 

  std::vector<byte> _message; 
  Node *_next; 
}; 


#include "MessageQueueSingle.tpp"


#endif
