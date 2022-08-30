#ifndef PHYLIPALIGNMENT_H
#define PHYLIPALIGNMENT_H

#include <iostream>
#include <memory>

#include "pll.h"
#include "AbstractAlphabet.hpp"


class AlignmentPLL
{
public:
  AlignmentPLL(); 
  AlignmentPLL(const AlignmentPLL& rhs) = delete; 
  AlignmentPLL& operator=(const AlignmentPLL &rhs) = delete; 
  AlignmentPLL(AlignmentPLL&& rhs); 
  AlignmentPLL& operator=( AlignmentPLL &&rhs) ; 
  
  
  void substituteBases () ;

  void initAln(std::string alnFile, int fileType);
  void initPartitions(std::string partitionFile); 
  void print() const ;
  virtual ~AlignmentPLL(); 
  void writeToFile( std::string fileName) const ; 
  void writeHeader(std::ofstream &out) const ; 

  template<typename T>
  void myWrite(std::ostream& out, const T* ptr, size_t length) const 
  {
    out.write((const char*) ptr, sizeof(T) * length) ; 
  }

  void writeWeights(std::ofstream &out) const;  

  void createDummyPartition(Alphabet alphabet) ; 


  static int guessFormat(std::string file); 

private: 			// ATTRIBUTES 
  pllAlignmentData* _pllAlignmentData; 
  partitionList* _partitions; 
};
 



#endif /* PHYLIPALIGNMENT_H */
