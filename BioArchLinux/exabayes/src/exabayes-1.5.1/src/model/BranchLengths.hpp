#ifndef BRANCHLENGTHS_H
#define BRANCHLENGTHS_H

#include <vector>

#include "BranchPlain.hpp"
#include "InternalBranchLength.hpp"
#include "Serializable.hpp"

class BranchLengths : public BranchPlain
{
public: 
  explicit BranchLengths(const BranchPlain &b = BranchPlain(),
                         std::vector<InternalBranchLength> lengths = {{}})
    : BranchPlain(b)
    , _lengths(lengths)
  {
  }

  virtual ~BranchLengths(){}

  BranchLengths getInverted() const 
  {
    return BranchLengths(BranchPlain::getInverted(), _lengths); 
  }

  void setLengths(std::vector<InternalBranchLength> lengths)
  {
    _lengths = lengths; 
  }

  std::vector<InternalBranchLength> getLengths() const 
  {
    return _lengths; 
  }

  virtual void deserialize( std::istream &in ); 
  virtual void serialize( std::ostream &out) const; 

private: 
  std::vector<InternalBranchLength> _lengths; 
};



#endif /* BRANCHLENGTHS_H */
