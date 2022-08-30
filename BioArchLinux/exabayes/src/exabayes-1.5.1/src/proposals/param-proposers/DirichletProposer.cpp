#include "DirichletProposer.hpp"
#include "BoundsChecker.hpp" 
#include <numeric>

DirichletProposer::DirichletProposer( double minValI, double maxValI, bool _minMaxIsRelative ) 
  : AbstractProposer{true, false, minValI, maxValI}
  , minMaxIsRelative{_minMaxIsRelative}
  , _rateHelper{}
{
} 


std::vector<double> DirichletProposer::proposeValues(std::vector<double> oldValues, double parameter, Randomness &rand, log_double &hastings)
{
  assert(fabs(std::accumulate(oldValues.begin(), oldValues.end(), 0. ) - 1.0 ) < 1e-6); // sum of rates equals 1 ? 

  auto newValues = std::vector<double> (); 
  auto scaledOld = _rateHelper.getScaledValues(oldValues, parameter); 

  newValues = rand.drawRandDirichlet(scaledOld); 
  
  if( minMaxIsRelative)		// for revmat 
    {
      for(auto &v : newValues)
	v /= newValues.back(); 

      BoundsChecker::correctRevMat(newValues);       

      RateHelper::convertToSum1(newValues);
    }
  else 				// for freuqencies (or everything else, that sums up to 1)
    {      
      correctAbsoluteRates(newValues); 

      RateHelper::convertToSum1(newValues);
    }  

  auto scaledNew = _rateHelper.getScaledValues(newValues, parameter);

  auto backP = Density::lnDirichlet(oldValues, scaledNew); 
  auto forP = Density::lnDirichlet(newValues, scaledOld); 

  hastings *= ( backP / forP); 
    
  assert(fabs(std::accumulate(oldValues.begin(), oldValues.end(), 0. ) - 1.0 ) < 1e-6); // sum of rates equals 1 ? 
  return newValues; 
}
