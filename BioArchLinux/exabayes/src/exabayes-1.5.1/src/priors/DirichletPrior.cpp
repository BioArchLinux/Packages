#include "DirichletPrior.hpp"
#include "RateHelper.hpp"
#include <numeric>


log_double DirichletPrior::getLogProb(const ParameterContent& content) const 
{
  auto &values = content.values; 
  assert(values.size() == alphas.size() ); 
  double sum = 0; 
  for(auto v: values)
    sum += v; 
  assert(fabs(sum - 1.0) < 1e-6); 

  auto result = Density::lnDirichlet(values, alphas ); 
  return result; 
}


void DirichletPrior::print(std::ostream& out ) const 
{
  out << "Dirichlet("   ; 
  bool first = true; 
  for(auto v : alphas)
    {      
      out << ( first ? "" : "," )  << v ;
      if(first)
	first = false;
    }    
  out << ")";
}


ParameterContent DirichletPrior::getInitialValue() const
{
  auto result = ParameterContent{}; 
  result.values = alphas; 
  RateHelper::convertToSum1(result.values); 
  return result; 
}
