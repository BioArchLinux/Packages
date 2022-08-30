#ifndef _ARRAY_ORIENTATION_HPP
#define _ARRAY_ORIENTATION_HPP

#include <vector>
#include <iostream>

#include "TreeAln.hpp"

#define INVALID 0 

class ArrayOrientation
{
public: 
  ArrayOrientation(const TreeAln &traln); 

  bool isCorrect(nat part, nat id, nat value) const  
  { 
    return orientation.at(part).at(id) ==  value; 
  }

  bool searchNodeInSubtree(const TreeAln &traln, nodeptr p, nat nodeId, nat depth ) const ; 
  
  // NEW 
  bool isCorrectNew( const TreeAln& traln, nat part, nodeptr p) const; 

  nat getOrientation(nat part, nat id ) const ; 
  
  void setOrientation(nat part, nat id, nat value)  { orientation[part][id] = value; }
  void setOrientationForAllPartitions(nat id ,nat value)  ; 

  void setPartitionInvalid(nat part)  ; 
  void setInvalid(nat part, nat id); 

  friend std::ostream& operator<<(std::ostream& out, const ArrayOrientation &rhs); 

  // decides how far down the tree we search to determine the
  // correctness of the array that belongs to a moved subtree
  // (this is a hack)
  const static nat maxSprSearch; 
  
  static nat nodeNum2ArrayNum(nat nodeNum, nat numTax); 

private: 
  std::vector<std::vector<nat> > orientation ; 
}; 

#endif
