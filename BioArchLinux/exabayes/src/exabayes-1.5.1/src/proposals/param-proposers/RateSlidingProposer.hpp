#ifndef RATES_LIDINGP_ROPOSER_HPP
#define RATES_LIDINGP_ROPOSER_HPP

#include "AbstractProposer.hpp"


class RateSlidingProposer : public AbstractProposer
{
public: 
  RateSlidingProposer(double minValI, double maxValI); 
  RateSlidingProposer(const RateSlidingProposer &rhs); 

  virtual std::vector<double> proposeValues(std::vector<double> oldValues, double parameter, Randomness &rand, log_double &hastings) ; 
  virtual AbstractProposer* clone() const { return new RateSlidingProposer(*this) ; } 


}; 




#endif
