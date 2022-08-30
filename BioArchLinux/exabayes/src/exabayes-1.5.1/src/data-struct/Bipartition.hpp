#ifndef BIPARTITION_H
#define BIPARTITION_H

#include <vector>  
#include <cassert>
#include <ostream>
#include <string>
#include <unordered_map>

#include "common.h"

class Bipartition
{
public: 
  explicit Bipartition();

  /** 
      @brief gets the number of elements that currently can be
      represented via this bipartition.
   */ 
  size_t getElemsReserved() const { return bip.size() * 32 ; } 
  
  void setHash(nat h) {assert(h != 0 ) ; hash = h; }
  nat getHash() const ; 
  /** 
      @brief indicates whether two bipartitions are compatible (i.e.,
      they can occur in a tree together)
   */ 
  bool isCompatible(const Bipartition& rhs, nat maxElem) const; 
  /** 
      @brief finds the index of the first bit set 
   */ 
  nat findIndex() const ; 
  /** 
      @brief gets the complement of a bipartition
   */ 
  Bipartition getComplement( nat maxElem) const; 
  /**
     @brief checks equality of two bipartitions
   */
  bool operator==(const Bipartition &rhs) const; 
  /** 
      @brief indicates whether this bipartition equals rhs.  
   */
  bool operator!=(const Bipartition &rhs) const {return not (  *this == rhs ) ; }
  /** 
      @brief performs an or-operation on two bipartitions and returns
      the result
      
      @notice the hash may loose its validity, if the bipartitions are
      or-ed that have an intersection.
   */ 
  Bipartition operator| (const Bipartition &rhs) const;
  /** 
      @brief performs an and-operaton on two bipartitions and results
      the result
   */ 
  Bipartition operator& (const Bipartition &rhs)const ; 
  
  void setRawBip(std::vector<nat> tmp){bip=tmp;}

  std::vector<nat> getRawBip() { return bip; }
  const std::vector<nat>& getRawBip() const {return bip; }
  /** 
      @brief indicates whether this bipartition is a subset of bipartition rhs. 
      @notice this method takes bipartitions as is and does not perform conversions (i.e., complement)      
  */ 
  bool isSubset(const Bipartition& rhs) const ; 
  /** 
      @brief sets the bit at position pos 
   */ 
  void set(nat pos) ; 
  /** 
      @brief unsets the bit at position pos 
   */ 
  // void unset(nat pos); 
  bool isSet(nat pos) const;
  void initializeWithTaxon(nat pos, nat ranNum); 
  /**
     @brief makes the bit vector sufficiently long, s.t. num can be
     set

     @notice do not overuse, this is somewhat expensive
   */ 
  void reserve(size_t num);
  /**
     @brief gets the number of bits set
   */ 
  nat count() const ;
  /** 
      @brief prints the bipartition in a readable manner
   */ 
  void printVerbose(std::ostream &outt, const std::vector<std::string> nameMap) const; 

  static std::vector<nat> perBitMask;
  static size_t numBits;
  static size_t numBitsMinusOne; 
  static size_t bitShift; 
  static nat allOne; 

  friend std::ostream& operator<<(std::ostream& out, const Bipartition& rhs); 

private: 
  std::vector<nat> bip; 
  nat hash; 

}; 


namespace std
{
  template<> struct hash<Bipartition>
  {
    size_t operator()(const Bipartition& b) const 
    {
      return b.getHash(); 
    }
  }; 
}


#endif
