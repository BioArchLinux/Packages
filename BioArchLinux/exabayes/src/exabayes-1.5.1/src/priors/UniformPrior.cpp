#include "UniformPrior.hpp"


UniformPrior::UniformPrior(double minV, double maxV) 
  : minVal(minV), maxVal(maxV)
{
}

log_double UniformPrior::getLogProb(const ParameterContent& content)  const 
{
  auto &values = content.values; 
  assert(values.size() == 1); 
  double value = values[0];

  if(minVal <= value && value <= maxVal )      
    return log_double::fromAbs(1 / (maxVal - minVal)); 
  else  
    {
      return log_double::lowest();
    }
}


void UniformPrior::print(std::ostream& out ) const  
{ 
  out << "Uniform("  << minVal << "," << maxVal << ")" ; 
}

ParameterContent UniformPrior::getInitialValue() const
{
  auto result = ParameterContent(); 

  assert(minVal < maxVal); 
  result.values.push_back( minVal + ( maxVal - minVal ) / 2  ) ; 
  return result; 
} 



log_double UniformPrior::accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param, double oldFc, double newFc )const 
{
  auto result = 0.; 

  auto maxV =  exp( - getMin() / newFc) , 
    minV = exp(  - getMax() / newFc) ; 

  for(auto &b : traln.extractBranches(param))
    {
      bool less = b.getLength().getValue() < minV; 
      bool greater = b.getLength().getValue() > maxV; 
	      
      assert(not (less && greater)); 

      if(less || greater)
	{
	  // tout << b.getLength() << " is not okay (" << b.getInterpretedLength(traln,param) << ")" << std::endl; 
	  result += - std::numeric_limits<double>::infinity();
	}
    }
  
  return log_double::fromAbs(result); 
} 


double UniformPrior::getFirstDerivative( const AbstractParameter& param) const
{
  return 0; 
} 
