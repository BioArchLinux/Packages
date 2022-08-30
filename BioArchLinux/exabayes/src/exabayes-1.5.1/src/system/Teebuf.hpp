#ifndef _TEE_BUF_H
#define _TEE_BUF_H

#include "threadDefs.hpp"
#include <streambuf>

class Teebuf : public std::streambuf
{
public: 
  Teebuf(std::streambuf &_sb1, std::streambuf &_sb2, std::thread::id masterThread); 
  void disable() {isDisabled = true; }
  
private: 
  virtual int overflow(int c); 
  virtual int sync(); 

  std::reference_wrapper<std::streambuf> sb1; 
  std::reference_wrapper<std::streambuf> sb2; 
  bool isDisabled; 
  std::thread::id _masterThread; 
}; 


#endif
