#include "RateSlidingProposer.hpp" 



RateSlidingProposer::RateSlidingProposer(double minValI, double maxValI)
  : AbstractProposer(true, true, minValI, maxValI)
{
}



RateSlidingProposer::RateSlidingProposer(const RateSlidingProposer &rhs)
  : AbstractProposer(rhs)
{
}


std::vector<double> RateSlidingProposer::proposeValues(std::vector<double> allOldValues, double parameter, Randomness &rand, log_double &hastings) 
{
  static const nat numFreq = 20; // meh
  assert(allOldValues.size() == (numFreq * numFreq - numFreq) / 2); 
  

  // TODO implement. There are quite some problems with the slider here. 

  // choose a reference rate 
  // auto whichFreq = rand.drawIntegerOpen(numFreq);

  // convertRelativeToLast(allOldValues); 
  
  // find upper and lower bound 
  
  
  
  assert(0); 
  
  return {}; 
} 
