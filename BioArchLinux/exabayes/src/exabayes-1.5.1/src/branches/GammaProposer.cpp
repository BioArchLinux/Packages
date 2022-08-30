#include "GammaProposer.hpp"

#include "BoundsChecker.hpp"
#include <cmath>
#include <cassert>

#include "TreeAln.hpp"
#include "AbstractParameter.hpp"
#include "Randomness.hpp"

GammaProposer::GammaProposer(double nrOpt, double nrd1, double nrd2, double convParameter, double nonConvParameter)
  : _nrOpt{ nrOpt }
  , _nrD1{ nrd1 }
  , _nrD2{ nrd2 }
  , _alpha{0}
  , _beta{0}
  , _convParameter{convParameter}
  , _nonConvParameter{nonConvParameter}
{
  bool hasConverged = (_nrD2 < 0) ; 
  
  if(not hasConverged)
    {
      _alpha = 1; 
      _beta = _nonConvParameter *  _nrD1; 
    }
  else 
    {
      auto estB = getEnvironmentVariable("EST_B"); 
      auto estC = getEnvironmentVariable("EST_C"); 

      assert(   (estB.compare("") == 0  ) ==  (estC.compare("") == 0 )   ); 

      double fit_b; 
      double fit_c; 

      if( estB.compare("") != 0 )
	{
	  fit_b = strtod(estB.c_str(), NULL); 
	  fit_c = strtod(estC.c_str(), NULL); 
	}
      else 
	{
	  fit_b = _convParameter; 
	  fit_c = -0.473 ; 
	}
    
      double c = fit_b *  pow( fabs(_nrD2),   2 * fit_c); 
      _beta = ( _nrOpt + sqrt( _nrOpt * _nrOpt + 4. * c)) / (2 * c); 
      _alpha = _nrOpt  * _beta + 1 ; 
    }
}


BranchLength GammaProposer::proposeBranch(BranchPlain b, TreeAln& traln, AbstractParameter* param, Randomness& rand) const 
{
  auto proposedVal = rand.drawRandGamma(_alpha, _beta); 

  auto bl = BranchLength(b, InternalBranchLength::fromAbsolute(proposedVal, param->getMeanSubstitutionRate())); 
  
  // we will not propose a branch outside the bounds 
  if(not BoundsChecker::checkBranch(bl) )
    BoundsChecker::correctBranch(bl); 
    
  return bl; 
}



log_double GammaProposer::getLogProbability(double absVal) const
{
  return Density::lnGamma(absVal, _alpha, _beta);
} 
