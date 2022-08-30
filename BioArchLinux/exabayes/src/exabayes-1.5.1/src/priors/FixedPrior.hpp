#ifndef _FIXE_PRIOR
#define  _FIXE_PRIOR

#include "AbstractPrior.hpp"


class FixedPrior : public AbstractPrior
{
public: 
  FixedPrior(std::vector<double> fixedValues)   ; 

  virtual bool needsIntegration() const {return false; } 
  virtual log_double getLogProb(const ParameterContent &content )  const; 

  virtual ParameterContent drawFromPrior(Randomness &rand)  const {assert(0); return ParameterContent{}; } 

  virtual void print(std::ostream &out) const ; 
  virtual ParameterContent getInitialValue() const; 

  virtual AbstractPrior* clone() const { return new  FixedPrior(*this) ; }
  virtual log_double accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param, double myOld, double myNew ) const; 

  double getFirstDerivative( const AbstractParameter& param) const {assert(0); return 0; }

  virtual log_double getUpdatedValue(double oldRawVal, double newRawVal, const AbstractParameter* param) const
    { assert(0); return log_double();}

private: 
  std::vector<double> _fixedValues; 
}; 

#endif
