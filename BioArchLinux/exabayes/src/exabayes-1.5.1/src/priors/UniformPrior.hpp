#ifndef _UNIFORM_PRIOR
#define _UNIFORM_PRIOR

#include "AbstractPrior.hpp" 

class UniformPrior : public AbstractPrior
{
public: 
  UniformPrior(double minVal, double maxVal); 
  virtual log_double getLogProb(const ParameterContent& content)  const ; 
  virtual bool needsIntegration() const {return true; } 
  virtual void print(std::ostream& out ) const  ; 
  virtual ParameterContent getInitialValue() const; 

  virtual ParameterContent drawFromPrior(Randomness &rand)  const {assert(0); return ParameterContent{}; } ; 

  virtual log_double accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param, double myOld, double myNew ) const; 

  virtual AbstractPrior* clone() const { return new  UniformPrior(*this) ; }

  virtual log_double getUpdatedValue(double oldRawVal, double newRawVal, const AbstractParameter* param) const
    { assert(0); return log_double();}

  double getMin() const {return minVal; }
  double getMax() const {return maxVal; }

  double getFirstDerivative( const AbstractParameter& param) const;

private:
  double minVal; 
  double maxVal; 
}; 

#endif
