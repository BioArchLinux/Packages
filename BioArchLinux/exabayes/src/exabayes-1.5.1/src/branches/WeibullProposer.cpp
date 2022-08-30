#include "WeibullProposer.hpp"
#include "brent.hpp"
#include "Density.hpp"
#include <functional>
#include "AbstractParameter.hpp"

#include "Randomness.hpp"
#include <cmath>


class OptFunc : public brent::func_base
{
public:   
  OptFunc(std::function< double(double)> fun)
    : _fun(fun)
  {
  }

  virtual double operator() (double val)
  {
    return _fun(val); 
  }

private: 
  std::function< double(double) > _fun; 

}; 





WeibullProposer::WeibullProposer(double nrOpt, double nrd1, double nrd2, double convParameter, double nonConvParameter)
  : _nrOpt{nrOpt}
  , _nrD1{nrd1}
  , _nrD2{nrd2}
  , _lambda{0}
  , _k{0}  
  , _convParameter{convParameter}
  , _nonConvParameter{nonConvParameter}
{
  bool hasConverged = _nrD2 < 0 ; 

  if(not hasConverged)
    {
      _lambda = _nonConvParameter / _nrD1; 
      _k = 1; 
    }
  else 
    {
      double fit_c = -0.473; 

      double estVar = _convParameter  * pow ( fabs(_nrD2) , 2 * fit_c); 
      
      auto lam = [=](double v) -> double 
	{
	  return pow(_nrOpt,2) 
	  * pow( (v-1) / v,  - 2 / v ) 
	  * ( Density::gammaFunction(1+(2/v)) - pow(Density::gammaFunction (1+(1/v)),2) )
	  - estVar ;
	}; 
      
      auto myFun = OptFunc(lam); 

      _k  = brent::zero(1,1000000, 1e-12, myFun);
      _lambda = _nrOpt / pow( (_k-1) / _k  , 1/ _k); 
    }
}


BranchLength WeibullProposer::proposeBranch(BranchPlain b, TreeAln& traln, AbstractParameter* param, Randomness& rand) const 
{
  auto proposedVal = rand.drawRandWeibull(_lambda, _k); 
  return BranchLength(b, InternalBranchLength::fromAbsolute(proposedVal, param->getMeanSubstitutionRate()));
} 

log_double WeibullProposer::getLogProbability(double val) const 
{
  auto res = Density::lnWeibull(val, _lambda, _k); 
  return res; 
} 

