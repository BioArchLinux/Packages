#ifndef _EXPONENTIAL_PRIOR 
#define _EXPONENTIAL_PRIOR 

#include "AbstractPrior.hpp"
#include "Density.hpp"
 
class ExponentialPrior : public AbstractPrior
{
public: 
  ExponentialPrior(double lambda) ; 
  virtual bool needsIntegration() const {return true; } 
  virtual log_double getLogProb(const ParameterContent& content) const  ; 
  virtual log_double getUpdatedValue(double oldRawVal, double newRawVal, const AbstractParameter* param) const  ;
  virtual ParameterContent drawFromPrior(Randomness &rand)  const ; 
  virtual void print(std::ostream& out ) const  ; 
  virtual double getLamda()  const  { return _lambda; } 
  virtual ParameterContent getInitialValue() const; 
  virtual log_double accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param, double myOld, double myNew )  const ; 

  virtual double getFirstDerivative( const AbstractParameter& param) const; 

  virtual AbstractPrior* clone() const { return new ExponentialPrior(*this) ; }

private: 
  double _lambda; 
}; 

#endif
