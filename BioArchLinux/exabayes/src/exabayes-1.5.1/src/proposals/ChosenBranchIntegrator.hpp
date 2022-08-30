#ifndef _CHOSEN_BRANCH_INTEGRATOR_HPP
#define _CHOSEN_BRANCH_INTEGRATOR_HPP

#include "BranchLengthMultiplier.hpp"


class ChosenBranchIntegrator : public BranchLengthMultiplier
{
public: 
  ChosenBranchIntegrator(double multiplier)
    : BranchLengthMultiplier(multiplier)
    , _chosenBranches{}
  {
  }

  virtual BranchPlain proposeBranch(const TreeAln &traln, Randomness &rand) const 
  {
    auto elem = rand.drawIntegerOpen(_chosenBranches.size());
    return _chosenBranches[elem]; 
  }   
  
  void setChosenBranches(std::vector<BranchPlain> branches)
  {
    _chosenBranches = branches; 
  }

  
  AbstractProposal* clone() const
  {
    return new ChosenBranchIntegrator(*this);
  }

private: 
    std::vector<BranchPlain> _chosenBranches; 


}; 

#endif
