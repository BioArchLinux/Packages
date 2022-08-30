#include "DistributionBranchLength.hpp"
#include "BoundsChecker.hpp"
#include "AbstractPrior.hpp"
#include "ParallelSetup.hpp"
#include "DistributionProposer.hpp"
#include "BranchLengthOptimizer.hpp"


#define MAX_OPT_ITER 8 

// ONLY gamma right now 


template<class C> 
DistributionBranchLength<C>::DistributionBranchLength( ) 
  : BranchLengthMultiplier(0)
  , _proposer()
  , _converged{false}
{
  _name = std::string("blDist") + C::getName() ; 
  _category = Category::BRANCH_LENGTHS; 
  _usingOptimizedBranches = true; 
  _relativeWeight = 7.;
}
 


template<class C>
std::pair<BranchPlain,BranchPlain> DistributionBranchLength<C>::prepareForSetExecution(TreeAln &traln, Randomness &rand) 
{
  assert(_inSetExecution); 

  auto chosenBranch = determinePrimeBranch(traln,rand);
  auto dummyBranch = BranchPlain(0,0); 

  return std::make_pair(chosenBranch, dummyBranch);
} 


template<class C>
void DistributionBranchLength<C>::createProposer(TreeAln &traln, LikelihoodEvaluator& eval, BranchPlain b)
{
  auto params = getBranchLengthsParameterView();
  assert(params.size() == 1 );     
  auto param = params[0]; 
  
  _savedBranch = traln.getBranch(b,param); 
  
  eval.evaluateSubtrees(traln, b, param->getPartitions(), false);
  eval.evaluatePartitionsDry(traln, b, param->getPartitions());

  auto blo= BranchLengthOptimizer(traln, b, MAX_OPT_ITER, eval.getParallelSetup().getChainComm(), params); 
  blo.optimizeBranches(traln);
  auto optParams = blo.getOptimizedParameters();
  assert(optParams.size() == 1);
  
  auto attr = _allParams->at(_primParamIds[0])->getAttributes();
    
  _proposer = optParams[0].getProposerDistribution< C >(traln, attr._convTuner.getParameter(), attr._nonConvTuner.getParameter());
}


template<class C>
void DistributionBranchLength<C>::applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) 
{
  auto b = proposeBranch(traln, rand); 
  
  auto params = getBranchLengthsParameterView();
  assert(params.size() == 1 );     
  auto param = params[0]; 

  _savedBranch = traln.getBranch(b, param);

  if(not _inSetExecution)
    createProposer(traln, eval, b);

  auto newBranch  = _proposer.proposeBranch(b, traln, param, rand); 

  _converged = _proposer.isConverged(); 

  auto prevAbsLen = _savedBranch.toMeanSubstitutions(param->getMeanSubstitutionRate()); 
  auto curAbsLen  = newBranch.toMeanSubstitutions(param->getMeanSubstitutionRate()); 

  auto oldPr = param->getPrior()->getLogProb( ParameterContent{{ prevAbsLen }} ); 
  auto newPr = param->getPrior()->getLogProb( ParameterContent{{ curAbsLen }} ); 

  // update prior 
  prior.addToRatio( newPr / oldPr );

  // update hastings 
  auto propDensCur = _proposer.getLogProbability(curAbsLen); 
  auto propDensPrev = _proposer.getLogProbability(prevAbsLen);

  hastings *= (propDensPrev / propDensCur) ;

  traln.setBranch(newBranch, param);
}


template<class C>
void DistributionBranchLength<C>::autotune()
{
  auto res = getEnvironmentVariable("TUNE"); 

  auto attr = _allParams->at(_primParamIds[0])->getAttributes(); 

  if(res.compare("0") != 0 )
    {
      if( attr._convTuner.getRecentlySeen()  > 100 )
	{
	  // tout << "tuning conv" << std::endl;
	  attr._convTuner.tune();
	}
      
      if( attr._nonConvTuner.getRecentlySeen() > 100)
      	{
	  // tout << "tuning non-conv" << std::endl;
      	  attr._nonConvTuner.tune();
      	}
    }
  
  _allParams->at(_primParamIds[0])->setAttributes(attr);

  _sctr.nextBatch();
}

 

template<class C>
void DistributionBranchLength<C>::accept() 
{
  _sctr.accept();  

  auto attr = _allParams->at(_primParamIds[0])->getAttributes();

  if(_converged)
    attr._convTuner.accept();
  else 
    attr._nonConvTuner.accept();
  
  _allParams->at(_primParamIds[0])->setAttributes(attr); 
}


template<class C>
void DistributionBranchLength<C>::reject() 
{
  _sctr.reject();

  auto attr = _allParams->at(_primParamIds[0])->getAttributes();

  if( _converged )
    attr._convTuner.reject();
  else 
    attr._nonConvTuner.reject();

  _allParams->at(_primParamIds[0])->setAttributes(attr); 
}

