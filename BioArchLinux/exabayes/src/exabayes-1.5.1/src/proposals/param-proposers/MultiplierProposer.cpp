#include "MultiplierProposer.hpp"


MultiplierProposer::MultiplierProposer(double minVal, double maxVal)
  : AbstractProposer{true, true, minVal, maxVal}
{
}


std::vector<double> MultiplierProposer::proposeValues(std::vector<double> oldValues, double parameter, Randomness &rand, log_double &hastings)
{    
  double newVal = 0,  multiplier = 0; 

  auto position = rand.drawIntegerOpen( oldValues.size() ); 
  multiplier =  rand.drawMultiplier( parameter);     
  newVal = oldValues[position] * multiplier; 

  // TODO allowed? 
  if(newVal < _minVal)
    newVal = _minVal; 
  else if(_maxVal < newVal)
    newVal = _maxVal; 

  hastings *= log_double::fromAbs(multiplier);

  oldValues[position] = newVal; 
  return oldValues;
}
