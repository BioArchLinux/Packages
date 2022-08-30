#ifndef SLIDINGPROPOSAL
#define SLIDINGPROPOSAL

#include "AbstractProposer.hpp"


class SlidingProposer : public AbstractProposer
{
public: 
  SlidingProposer(double minVal, double maxVal, bool minMaxIsRelative); 
  virtual ~SlidingProposer(){}

  SlidingProposer(const SlidingProposer &rhs) 
    : AbstractProposer(rhs)
    , minMaxIsRelative(rhs.minMaxIsRelative)
  {
  }

  double proposeOneValue(double oldVal, double parameter, Randomness &rand, log_double &hastings); 

  std::vector<double> proposeRelativeMany(std::vector<double> oldValues, double parameter, Randomness &rand, log_double &hastings); 


  virtual std::vector<double> proposeValues(std::vector<double> oldValues, double parameter, Randomness &rand, log_double &hastings); 

  virtual AbstractProposer* clone() const  {return new SlidingProposer(*this);  }


private: 
  bool minMaxIsRelative ; 
}; 

#endif
