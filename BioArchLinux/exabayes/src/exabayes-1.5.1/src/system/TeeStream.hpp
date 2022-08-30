#ifndef _TEE_STREAM_H
#define _TEE_STREAM_H

#include "Teebuf.hpp"
#include <iostream>
#include <unordered_set>
#include <vector>


// START definition 
template<typename T> std::ostream& operator<<(std::ostream& out, std::vector<T> elems); 
template<typename T> std::ostream& operator<<(std::ostream& out, std::unordered_set<T> elems); 
// END 



template<typename T>
std::ostream& operator<<(std::ostream& out, std::vector<T> elems)
{
  bool isFirst = true; 
  for(auto &elem : elems)
    {
      if(isFirst) 
	isFirst = false; 
      else 
	out << "," ; 
      out << elem ; 
    }
  return out; 
}


template<typename T>
std::ostream& operator<<(std::ostream& out, std::unordered_set<T> elems)
{
  bool isFirst = true; 
  for(auto &elem : elems)
    {
      if(isFirst)
	isFirst = false; 
      else 
	out << "," ; 
      out << elem; 
    }

  return out; 
}






class TeeStream : public std::ostream
{
public:   
  TeeStream(std::ostream &o1, std::ostream &o2, std::thread::id tid)
    : std::ostream(&tbuf)
    ,tbuf(*o1.rdbuf(), *o2.rdbuf(), tid)
  {}

  void disable(){tbuf.disable();}
  
private :
  Teebuf tbuf; 
}; 

#endif
