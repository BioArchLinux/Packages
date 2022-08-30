#ifndef BRANCHLENGTH_H
#define BRANCHLENGTH_H

#include "BranchPlain.hpp"
#include "InternalBranchLength.hpp"
#include "Serializable.hpp"


class BranchLength :  public BranchPlain 
{
public : 			// INHERITED 
  virtual void deserialize( std::istream &in )  ; 
  virtual void serialize( std::ostream &out) const;   

public:
  explicit BranchLength(const BranchPlain &b = BranchPlain(), InternalBranchLength len = 0 )
    : BranchPlain(b)
    , _length(len)
  {
  }
  
    virtual ~BranchLength(){}

  double toMeanSubstitutions(double meanSubstRate) const 
  {
    return _length.toMeanSubstitutions(meanSubstRate); 
  }

  InternalBranchLength getLength() const 
  {
    return _length; 
  }

  void setLength(InternalBranchLength length)  
  {
    _length = length; 
  }


  BranchLength getInverted() const 
  {
    return BranchLength(BranchPlain::getInverted(), _length); 
  }

  
private: 
  InternalBranchLength _length; 
};




#endif /* BRANCHLENGTH_H */
