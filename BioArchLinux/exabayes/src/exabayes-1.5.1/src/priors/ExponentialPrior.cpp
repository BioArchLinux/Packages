#include "ExponentialPrior.hpp"
#include "AbstractParameter.hpp"
#include "BranchLengthsParameter.hpp"

ExponentialPrior::ExponentialPrior(double l) 
  : _lambda(l)
{
}


log_double ExponentialPrior::getLogProb(const ParameterContent& content) const 
{
  auto& values = content.values; 
  assert(values.size() == 1); 
  return Density::lnExponential(values[0], _lambda); 
}


void ExponentialPrior::print(std::ostream& out ) const  
{        
  out << "Exponential("  << _lambda << ")" ;       
}


log_double ExponentialPrior::getUpdatedValue(double oldRawVal, double newRawVal, const AbstractParameter* param) const  
{
  // tout << SHOW(newRawVal)e
  auto result = _lambda *  param->getMeanSubstitutionRate() * log(newRawVal / oldRawVal) ; 
  return log_double::fromLog(result)  ; 
}


log_double ExponentialPrior::accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param, double myOld, double myNew ) const 
{
  double blInfluence = 0; 
  for(auto &b : traln.extractBranches(param))
    blInfluence += log(b.getLength().getValue());

  auto result = log_double::fromLog((myNew - myOld) *  _lambda  *  blInfluence ); 
  return result; 
}


ParameterContent ExponentialPrior::getInitialValue() const
{
  auto result = ParameterContent{}; 
  result.values.push_back( 1. / _lambda); 
  return result; 
} 


double ExponentialPrior::getFirstDerivative(const AbstractParameter& param) const
{
  // this assert is clearly ridiculous; however it is not worth making
  // the situation even more complicated...
  
  // TODO properly cast it 
  assert( dynamic_cast<BranchLengthsParameter*>(const_cast<AbstractParameter*>(&param)) != nullptr ); 
  auto fracchange = param.getMeanSubstitutionRate();
  return fracchange * _lambda; 
}


ParameterContent ExponentialPrior::drawFromPrior(Randomness &rand)  const 
{
  auto val = rand.drawRandExp(_lambda); 
  return ParameterContent{{val}};
} 
