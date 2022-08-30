#ifndef _DISTRIBUTION_PROPOSER_HPP
#define _DISTRIBUTION_PROPOSER_HPP

/** 
    @brief a CRTP wrapper for proposal distributions (template only)
*/ 
    

#include <string>

#include "extensions.hpp"

#include "Randomness.hpp"
#include "BoundsChecker.hpp"

template<class T>
class DistributionProposer
{
public: 
  DistributionProposer(double nrOpt = 0, double nrD1 = 0 , double nrD2 = 0, double convParameter = 0 , double nonConvParameter = 0 )  
    : _dist{nrOpt, nrD1, nrD2, convParameter, nonConvParameter} 
  {
  }


  static std::string getName()
  {
    return T::getName();
  }

  bool isConverged() const 
  {
    return _dist.isConverged();
  }


  BranchLength proposeBranch(BranchPlain b, TreeAln& traln, AbstractParameter* param, Randomness& rand) const  
  {
    auto branch = _dist.proposeBranch(b,traln, param, rand ); 
    if(not BoundsChecker::checkBranch(branch))
      BoundsChecker::correctBranch(branch); 
    return branch; 
  }
 
  log_double getLogProbability(double val) const 
  {
    return _dist.getLogProbability(val);
  } 

  friend auto operator<<(std::ostream& out, const DistributionProposer &rhs)
    -> std::ostream&
  {
    out << rhs._dist; 
    return out; 
  }

private: 
  T _dist; 
};


#endif
