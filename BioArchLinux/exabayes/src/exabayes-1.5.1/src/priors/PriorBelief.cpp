#include "PriorBelief.hpp"

#include <cmath>

#include "BoundsChecker.hpp"
#include "GlobalVariables.hpp"
#include "AbstractPrior.hpp"
#include "UniformPrior.hpp"
#include "ExponentialPrior.hpp"
#include "Category.hpp"

#include "TreePrinter.hpp"


PriorBelief::PriorBelief()
  : _lnPrior(log_double::fromAbs(1.))
  , _lnPriorRatio(log_double::fromAbs(1.))
  , wasInitialized(false)
{
}


void PriorBelief::initialize(const TreeAln &traln , ParameterList &params)
{
  _lnPrior = scoreEverything(traln, params); 
  _lnPriorRatio = log_double::fromAbs(1.); 
  wasInitialized = true; 
}


log_double PriorBelief::scoreEverything(const TreeAln &traln, ParameterList &parameters) const 
{
  auto result = log_double::fromAbs(1.);

  for( auto& v : parameters)
    {
      auto partialResult = log_double::fromAbs(1.); 
      
      switch(v->getCategory()) 
	{	  
	case Category::DIVERGENCE_RATES: 
	case Category::DIVERGENCE_TIMES: 
	  {
	    // that's how it is supposed to be ... this entire switch
	    // should be converted into respective methods in the
	    // parameters ...
	    partialResult *= v->getPriorValue(traln); 
	  }
	  break; 
	case Category::TOPOLOGY: 	  
	  partialResult *= log_double::fromAbs(1.); 	// well...
	  break; 
	case Category::BRANCH_LENGTHS: 
	  {
	    auto bs = traln.extractBranches(v); 
	    auto pr = v->getPrior(); 
	    auto meanSubstRate = v->getMeanSubstitutionRate();
	    for(auto &b : bs)	      
	      {
		auto len = b.toMeanSubstitutions(meanSubstRate); 
		auto logProb = pr->getLogProb( ParameterContent {  { len  }  }); 
		partialResult *= logProb; 
	      }
	  }
	  break; 
	case Category::FREQUENCIES: 
	  {
	    auto freqs = traln.getFrequencies(v->getPartitions()[0]); 
	    partialResult = v->getPrior()->getLogProb( freqs) ; 
	  } 
	  break; 
	case Category::SUBSTITUTION_RATES: 
	  {
	    auto revMat = traln.getRevMat(v->getPartitions()[0] , false); 
	    partialResult = v->getPrior()->getLogProb(revMat); 
	  }
	  break; 
	case Category::RATE_HETEROGENEITY: 
	  {
	    double alpha = traln.getAlpha(v->getPartitions()[0]) ;
	    partialResult = v->getPrior()->getLogProb( ParameterContent{{ alpha} }); 
	  }
	  break; 
	case Category::AA_MODEL: 
	  {
	    auto p = v->getPartitions()[0]; 
	    auto model = traln.getProteinModel(p); 
	    auto content =  ParameterContent(); 
	    content.protModel.push_back(model); 
	    partialResult = v->getPrior()->getLogProb(content);
	  }
	  break; 
	default : assert(0); 
	}
      
      // std::cout << "pr(" <<  v << ") = "<< partialResult << std::endl ; 
      result *= partialResult; 
    }

  // the lnPriorRatio may be infinite. But never the assumed value   
  assert(not result.isInfinity() && not result.isNaN()); 
  
  return result; 
} 


void PriorBelief::verifyPrior(const TreeAln &traln, ParameterList& parameters)  
{
  // assert( _lnPriorRatio.toAbs() == 1. ); 
  
  auto verified = scoreEverything(traln, parameters); 

  if (   log_double( verified / _lnPrior).toAbs() - 1.  >=  ACCEPTED_LNPR_EPS)
    {
      tout << MAX_SCI_PRECISION << "ln prior was " << _lnPrior << " while it should be " << verified << std::endl; 
      tout << "difference: "<< fabs( verified.toAbs() - _lnPrior.toAbs()) << std::endl; 
      assert(0); 
    }

  _lnPrior = verified; 
}


void PriorBelief::addToRatio(log_double val) 
{
  assert(wasInitialized) ;
  // tout << MAX_SCI_PRECISION << "adding " << val << std::endl; 
  _lnPriorRatio *= val; 
}
