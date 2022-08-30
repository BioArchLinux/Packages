#ifndef _GAMMA_PROPOSER_HPP
#define _GAMMA_PROPOSER_HPP

// #include "Branch.hpp"
#include "BranchLength.hpp"
#include "extensions.hpp"

class AbstractParameter; 
class Randomness; 
class TreeAln; 

class GammaProposer
{
public: 
  GammaProposer(double nrOpt, double nrd1, double nrd2, double convParameter, double nonConvParameter); 
  GammaProposer(const GammaProposer &rhs) = default; 
  BranchLength proposeBranch(BranchPlain b, TreeAln& traln, AbstractParameter* param, Randomness& rand) const ; 
  log_double getLogProbability(double val) const;

  bool isConverged() const {return _nrD2 < 0  && _nrD1 < 1 ; }

  static std::string getName()
  {
    return std::string( "Gamma") ; 
  }

  friend std::ostream& operator<<(std::ostream& out, const GammaProposer &rhs)
  {
    out << SHOW(rhs._nrOpt) << SHOW(rhs._nrD1) << SHOW(rhs._nrD2) << SHOW(rhs._alpha) << SHOW(rhs._beta); 
    return out; 
  }

private: 			// ATTRIBUTES
  double _nrOpt; 		// branch length optimum 
  double _nrD1; 		// first derivative 
  double _nrD2; 		// second derivative
  
  double _alpha; 		
  double _beta; 

  double _convParameter; 
  double _nonConvParameter; 
}; 


#endif
