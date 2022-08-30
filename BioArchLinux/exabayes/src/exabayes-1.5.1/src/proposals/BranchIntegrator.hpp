#ifndef _BRANCH_INTEGRATOR
#define _BRANCH_INTEGRATOR

#include "BranchLengthMultiplier.hpp"

class BranchIntegrator  : public BranchLengthMultiplier
{
public: 
  BranchIntegrator( double _mult )
    : BranchLengthMultiplier( _mult)
    , toPropose{}
  {
    _name =  "blInt"; 
    _relativeWeight = 20; 
    _category = Category::BRANCH_LENGTHS;     
  }

  virtual BranchPlain proposeBranch(const TreeAln &traln, Randomness &rand) const
  {
    
    return toPropose; 
  } 
  
  void setToPropose(BranchPlain b) { toPropose = b; }

  virtual AbstractProposal* clone() const {return new BranchIntegrator(*this); }   

  virtual std::vector<nat> getInvalidatedNodes(const TreeAln &traln ) const  {return {}; }

  virtual void autotune(){}

private:
  BranchPlain toPropose; 
  

}; 


#endif
