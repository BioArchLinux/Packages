#ifndef DIRICHLETPROPOSAL_H
#define DIRICHLETPROPOSAL_H

#include "AbstractProposer.hpp"
#include "RateHelper.hpp"

class DirichletProposer : public AbstractProposer
{				
public: 
  /** 
      @brief constructs a dirichlet proposal
      
      @param minMaxIsRelative
      indicates whether the previous two boundary arguments are
      relative to a value (e.g., revmat) or whether they sum up to 1.
   */ 
  DirichletProposer( double minVal, double maxVal, bool minMaxIsRelative); 

  DirichletProposer(const DirichletProposer& rhs)  = default; 
  //   : AbstractProposer(rhs)
  // {
  //   minMaxIsRelative = rhs.minMaxIsRelative; 
  // }

  virtual ~DirichletProposer(){}
  virtual std::vector<double> proposeValues(std::vector<double> oldValues, double parameter, Randomness &rand, log_double &hastings); 
  virtual AbstractProposer* clone() const  {return new DirichletProposer(*this);  }

private: 
  bool minMaxIsRelative;

  RateHelper _rateHelper; 
}; 

#endif
