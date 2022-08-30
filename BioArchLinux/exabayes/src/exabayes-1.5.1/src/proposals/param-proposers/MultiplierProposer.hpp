#ifndef MULTIPLIERPROPOSAL_H
#define MULTIPLIERPROPOSAL_H

#include "AbstractProposer.hpp"

////////////////
// MULTIPLIER //
////////////////
class MultiplierProposer : public AbstractProposer
{
public: 
  MultiplierProposer(double minVal, double maxVal); 
  
  MultiplierProposer(const MultiplierProposer& rhs): AbstractProposer(rhs) {}

  virtual AbstractProposer* clone() const  {return new MultiplierProposer(*this);  }
  virtual std::vector<double> proposeValues(std::vector<double> oldValues, double parameter, Randomness &rand, log_double &hastings); 
}; 

#endif
