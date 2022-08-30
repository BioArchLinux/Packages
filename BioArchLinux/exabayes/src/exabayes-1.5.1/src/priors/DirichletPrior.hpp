
#ifndef _DIRICHLET_PRIOR
#define _DIRICHLET_PRIOR

#include "AbstractPrior.hpp"

class DirichletPrior : public AbstractPrior
{
public: 
  DirichletPrior(std::vector<double> a) 
    : alphas(a)
  {
  }

  virtual ParameterContent drawFromPrior(Randomness &rand)  const {assert(0); return ParameterContent{}; } ; 

  virtual log_double getLogProb( const ParameterContent& content) const ; 
  virtual void print(std::ostream& out ) const ; 

  virtual ParameterContent getInitialValue() const; 

  virtual bool needsIntegration() const {return true; } 

  virtual log_double accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param, double myOld, double myNew ) const {assert(0) ; return log_double::fromAbs(0) ; } 

  virtual AbstractPrior* clone() const { return new  DirichletPrior(*this) ; }

  double getFirstDerivative( const AbstractParameter& param) const {assert(0); return 0; } // doesnt have that

  virtual log_double getUpdatedValue(double oldRawVal, double newRawVal, const AbstractParameter* param) const
    { assert(0); return log_double();}
  
private: 
  std::vector<double> alphas; 

} ;

#endif
