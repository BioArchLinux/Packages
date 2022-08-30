#ifndef _CHECKPOINTABLE_H 
#define _CHECKPOINTABLE_H 

#include <iostream>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <limits>
#include "common.h"


class Serializable
{
public: 			// INHERITED METHODS 
  virtual void deserialize( std::istream &in )  = 0 ; 
  virtual void serialize( std::ostream &out) const = 0;   

public: 
  Serializable()
    : DELIM('&')
    , checkpointIsBinary(true)
  {}

  Serializable(const Serializable& rhs) = default; 
  Serializable( Serializable&& rhs) = default; 
  Serializable& operator=(const Serializable &rhs)  = default; 
  Serializable& operator=( Serializable &&rhs)  = default; 

  virtual ~Serializable(){}

  void getOfstream(std::string name, std::ofstream &result); 
  void getIfstream(std::string name, std::ifstream &result ); 

  template<typename T> void cWrite(std::ostream &out, const T &toWrite) const; 
  template<typename T> T cRead(std::istream &in); 

  std::string readString(std::istream &in ); 
  void writeString(std::ostream &out, std::string toWrite) const; 

private: 			// METHODS
  void readDelimiter(std::istream &in) ; 

private: 			// ATTRIBUTES 
  char DELIM; 
  bool checkpointIsBinary; 
}; 


#endif
