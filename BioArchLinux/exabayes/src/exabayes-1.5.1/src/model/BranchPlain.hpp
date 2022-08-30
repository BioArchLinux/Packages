#ifndef BRANCHBASE_H
#define BRANCHBASE_H

#include <cassert>

#include <fstream>

#include "common.h"


#include "Serializable.hpp"

class BranchPlain   : public Serializable 
{
public: 			// INHERITED
  virtual void deserialize( std::istream &in )   ; 
  virtual void serialize( std::ostream &out) const;   
  
public:
  explicit BranchPlain(nat a = 0, nat b = 0)
    : _primNode{a}
    , _secNode{b}
  {}

  virtual ~BranchPlain(){}

  BranchPlain getInverted() const 
  {
    return BranchPlain(_secNode, _primNode); 
  } 

  bool operator==(const BranchPlain  &other) const 
  {
    return other._primNode == _primNode && other._secNode == _secNode; 
  }


  bool operator!=(const BranchPlain &other) const 
  {
    return not (*this == other); 
  }

  bool hasNode(nat aNode) const
  {
    return _primNode == aNode  || _secNode == aNode; 
  }


  nat getPrimNode() const 
  {
    return _primNode; 
  }
  
  nat getSecNode() const 
  {
    return _secNode;
  }

  void setPrimNode(nat p)
  {
    _primNode = p; 
  }

  void setSecNode(nat p)
  {
    _secNode = p; 
  }


  nat getOtherNode(nat aNode) const
  {
    assert(hasNode(aNode)); 
    return aNode == _primNode ? _secNode : _primNode; 
  }

  friend std::ostream& operator<<(std::ostream& s, const BranchPlain& c)
  {
    s << c._primNode << "," << c._secNode; 
    return s;
  }  


private: 
  nat _primNode; 
  nat _secNode; 
};




namespace std
{
  template<> 
  struct hash<BranchPlain>
  {
    size_t operator()(const BranchPlain & x) const
    {
      return std::hash<size_t>()(x.getSecNode()) ^ std::hash<size_t>()(x.getPrimNode()); 
    }
  }; 

  template<> 
  struct equal_to<BranchPlain>
  {
    bool operator()(const BranchPlain &a, const BranchPlain &b)  const
    {
      return a == b || a.getInverted() == b  ; 
    }
  }; 
}

nat getCommonNode(const BranchPlain &oneBranch, const BranchPlain &otherBranch) ; 
bool isAdjacent(const BranchPlain &oneBranch, const BranchPlain& otherBranch); 

#endif /* BRANCHBASE_H */



