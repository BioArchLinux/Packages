#ifndef DISCRETE_MODEL_PRIOR
#define DISCRETE_MODEL_PRIOR

#include <unordered_map>
#include "ProtModel.hpp"
#include "AbstractPrior.hpp"

class DiscreteModelPrior : public AbstractPrior
{
public: 
  DiscreteModelPrior(std::unordered_map<ProtModel,double> model); 
  
  // if we have only one model this is basically a fixed prior 
  virtual bool needsIntegration() const {assert(_modelProbs.size() > 0 ); return _modelProbs.size() > 1 ; } 

  virtual ParameterContent getInitialValue() const ; 
  virtual log_double accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param , double myOld, double myNew ) const ; 
  ParameterContent drawFromPrior(Randomness &rand)  const ; 
  virtual log_double getLogProb(const ParameterContent& content ) const ; 
  virtual void print(std::ostream &out) const ;

  double getFirstDerivative( const AbstractParameter& param) const {assert(0); return 0; } // doesnt have that

  virtual log_double getUpdatedValue(double oldRawVal, double newRawVal, const AbstractParameter* param) const
    { assert(0); return log_double();}

  virtual AbstractPrior* clone()const { return new  DiscreteModelPrior(*this) ; }
private: 
  std::unordered_map<ProtModel,double> _modelProbs; 
}; 

#endif
