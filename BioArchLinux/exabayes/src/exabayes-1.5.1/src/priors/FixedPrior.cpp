#include "FixedPrior.hpp"
#include "AbstractParameter.hpp"
#include "Category.hpp"
#include "BoundsChecker.hpp"
#include <numeric>

FixedPrior::FixedPrior(std::vector<double> fixedValues)
  : _fixedValues(fixedValues) 
{
  if(_fixedValues.size() >  1 )
    {
      auto sum = std::accumulate(begin(_fixedValues), end(_fixedValues),0.); 
      for(auto &v : _fixedValues)
	v /= sum; 
    }
}


log_double FixedPrior::getLogProb(const ParameterContent& content)  const
{    
  auto &values = content.values; 

  // branch lengths case! 
  if(_fixedValues.size( )== 1 )
    {
      if(fabs (values[0] - _fixedValues.at(0) ) > 1e-6)
	{
	  tout << "Fatal: for fixed prior, value should be "   << _fixedValues.at(0) << " but actually is " << values[0] << std::endl; 
	  assert(0); 
	}
    }
  else 
    {
      for(nat i = 0; i < _fixedValues.size() ; ++i)
	{
	  if(std::fabs(_fixedValues[i] - values[i]) > 1e-6 )
	    {
	      tout << "error: expected " << SHOW(_fixedValues) << " but obtained " << SHOW(values) << std::endl; 
	      assert(0); 
	    }
	}
    }

  return log_double::fromAbs(1.); 
}


ParameterContent FixedPrior::getInitialValue() const
{
  auto result = ParameterContent{}; 

  if(not _keepInitData) 
    result.values = _fixedValues; 
  else 
    result.values = {0.1}; 
       
  return result; 
} 


void FixedPrior::print(std::ostream &out) const 
{
  out << "Fixed(" ;     
  bool first = true; 
  for(auto v : _fixedValues)
    {
      out << (first ? "" : ",") << v ; 
      if(first) first = false; 
    }
  out << ")"; 
}


log_double FixedPrior::accountForMeanSubstChange(TreeAln  &traln, const AbstractParameter* param, double myOld, double myNew ) const 
{
  if(param->getCategory() == Category::BRANCH_LENGTHS)
    {
      // first check, if re-scaling is possible 
      auto bls = traln.extractBranches(param); 

      for(auto &b : bls)
	{
	  if(_keepInitData)
	    {
	      auto len = b.getLength().getValue();
	      double frac = myNew / myOld; 
	      b.setLength( InternalBranchLength(pow(len, 1. / frac ) ));
	    }
	  else 
	    {
	      b.setLength(InternalBranchLength::fromAbsolute(_fixedValues.at(0), param->getMeanSubstitutionRate())); 
	    }
	}
      
      if( std::any_of(bls.begin(), bls.end(), [](BranchLength &b){ return not BoundsChecker::checkBranch(b) ;  }) ) 
	return log_double::negativeInfinity();

      for(auto &b : bls)
	traln.setBranch(b,param); 

      return log_double::fromAbs(1); 
    }
  else 
    {
      assert(0); 
      return log_double::fromAbs(0); 
    }
}
