#include "SlidingProposer.hpp"
#include "RateHelper.hpp"
#include "BoundsChecker.hpp"
#include <numeric>

SlidingProposer::SlidingProposer(double minVal, double maxVal, bool _minMaxIsRelative)
  : AbstractProposer{true, true, minVal, maxVal}
  , minMaxIsRelative{_minMaxIsRelative}
{
}


double SlidingProposer::proposeOneValue(double oldVal, double parameter, Randomness &rand, log_double &hastings)
{
  double newVal = rand.drawFromSlidingWindow(oldVal, parameter);
  if(newVal < 0 )
    newVal = - newVal ; 

  // todo currently only asserting, that it is not used 
  assert(0);
  return newVal; 
} 


std::vector<double> SlidingProposer::proposeRelativeMany(std::vector<double> oldValues, double parameter, Randomness &rand, log_double &hastings)
{

#if 1

  auto posA = rand.drawIntegerOpen(oldValues.size()) ; 
  auto posB = rand.drawIntegerOpen(oldValues.size()-1); 
  if(posA == posB)
    posB = oldValues.size() - 1 ; 

  
  // tout << "proposing " << posA << " and " << posB << std::endl ; 
  
  auto &rateA = oldValues.at(posA); 
  auto &rateB = oldValues.at(posB); 
  double sum = rateA + rateB; 

  // tout << MAX_SCI_PRECISION << "oldVals="  << rateA << "," << rateB  << "\t"; 
  
  double oldProb = rateA / (sum); 
  double newProb = rand.drawFromSlidingWindow(oldProb, parameter) ; 
  
  // tout << "new prob= " << newProb << std::endl; 

  if(newProb <= 0)
    newProb = - newProb; 
  else if(1 <= newProb )
    newProb = newProb-1; 

  // tout << "new prob after correct= " << newProb << std::endl; 
  
  assert(0. < newProb && newProb < 1. ); 

  rateA = newProb *   sum ; 
  rateB = (1-newProb) * sum ; 

  // tout << "newVals=" << rateA << ","  << rateB << std::endl; 
  // assert((rateA + rateB) -  sum < 1e-6); 

#else 

  nat posA = rand.drawIntegerOpen(oldValues.size()); 
  auto &rate = oldValues.at(posA)  ; 
  double newVal = rand.drawFromSlidingWindow(rate, parameter) ; 

  if(newVal <= 0)
    newVal = - newVal; 
  else if(1 <= newVal )
    newVal = newVal-1; 
  
  rate = newVal; 
  
  RateHelper::convertToSum1(oldValues); 

#endif

  // check if contracts are met  
  if(minMaxIsRelative)
    {
      RateHelper::convertRelativeToLast(oldValues); 
      BoundsChecker::correctRevMat(oldValues); 
      RateHelper::convertToSum1(oldValues); 
    }
  else 
    {
      // sum may not be one 
      RateHelper::convertToSum1(oldValues) ; 
      correctAbsoluteRates(oldValues);
      RateHelper::convertToSum1(oldValues) ; 
    }

  if( fabs( std::accumulate(oldValues.begin(), oldValues.end(), 0.) - 1.0 ) > 1e-6 )
    {
      std::cerr << "Danger: while proposing values,  sum was " << std::accumulate(oldValues.begin(), oldValues.end(), 0.) << ". values: "   << oldValues  << std::endl; 
      assert(0); 
    }

  return oldValues; 
}



std::vector<double> SlidingProposer::proposeValues(std::vector<double> oldValues, double parameter, Randomness &rand, log_double &hastings)
{
  if(oldValues.size() == 1 )
    return {proposeOneValue(oldValues[0], parameter,rand, hastings)}; 
  else
    return proposeRelativeMany(oldValues, parameter, rand, hastings);
}


