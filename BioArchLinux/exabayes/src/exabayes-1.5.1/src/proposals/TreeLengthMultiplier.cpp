
#include "BoundsChecker.hpp"

#include "TreeLengthMultiplier.hpp"
#include "Randomness.hpp"
#include "TreeAln.hpp"
#include "UniformPrior.hpp"
#include "AbstractPrior.hpp"

TreeLengthMultiplier::TreeLengthMultiplier( double _multiplier)
  : AbstractProposal(Category::BRANCH_LENGTHS, "TL-Mult", 1., 0.0001, 100, true)
  , multiplier(_multiplier)    
  , initTreeLength{0.}
  , storedBranches{}
{
}

void TreeLengthMultiplier::applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) 
{
  storedBranches.clear(); 

  assert(_primParamIds.size() == 1); 
  auto blParam = (*_allParams)[_primParamIds[0]]; 
  // getPrimaryParameterView()[0]; 



  storedBranches = traln.extractBranches(blParam);

  auto  newBranches = storedBranches; 
  
  double treeScaler = rand.drawMultiplier(multiplier); 
  double initTL = 0; 
  double newTL = 0; 

  auto haveUniformPrior = dynamic_cast<UniformPrior*>(blParam->getPrior()) != nullptr;

  auto meanSubstRate = blParam->getMeanSubstitutionRate(); 

  for(auto &b : newBranches)
    {
      auto initLength = b.getLength();
      initTL += b.toMeanSubstitutions(meanSubstRate); 

      b.setLength(  InternalBranchLength(pow(initLength.getValue(), treeScaler) )); 
      
      if( not BoundsChecker::checkBranch(b))
	BoundsChecker::correctBranch(b);

      double realScaling = log(b.getLength().getValue()) / log(initLength.getValue()); 

      hastings *= log_double::fromAbs(realScaling); 

      double tmp = b.toMeanSubstitutions(meanSubstRate) ; 
      newTL += tmp;
      
      // correct? 
      if(haveUniformPrior && blParam->getPrior()->getLogProb(ParameterContent{{tmp}}).isNegativeInfinity())
	prior.addToRatio( log_double::lowest()); 
    }

  for(auto &b : newBranches)
    traln.setBranch(b, blParam);

  if(not haveUniformPrior)
    {
      auto tmp = blParam->getPrior()->getLogProb( ParameterContent{{ newTL} }  ) / blParam->getPrior()->getLogProb( ParameterContent{{ initTL} } ); 
      prior.addToRatio( tmp ); 
    }
}


void TreeLengthMultiplier::resetState(TreeAln &traln)  
{
  auto blParams = getPrimaryParameterView(); 
  assert(blParams.size( )== 1 ) ;
  auto blParam = blParams[0]; 
  for(auto &b : storedBranches)
    traln.setBranch(b, blParam); 
} 


void TreeLengthMultiplier::autotune()
{
  double parameter = multiplier; 

  double newParam = tuneParameter(_sctr.getBatch(), _sctr.getRatioInLastInterval(), parameter, PLL_FALSE);

  multiplier = newParam; 

  _sctr.nextBatch();
}
 
 
void TreeLengthMultiplier::evaluateProposal(  LikelihoodEvaluator &evaluator, TreeAln &traln, const BranchPlain &branchSuggestion) 
{
  assert(_primParamIds.size( )== 1); 
  auto parts = _allParams->at(_primParamIds[0])->getPartitions(); 

#ifdef PRINT_EVAL_CHOICE
  tout << "EVAL-CHOICE " << branchSuggestion << std::endl; 
#endif

  evaluator.evaluatePartitionsWithRoot(traln,branchSuggestion, parts, true); 
}


AbstractProposal* TreeLengthMultiplier::clone() const
{
  return new TreeLengthMultiplier(*this);
}  


void TreeLengthMultiplier::readFromCheckpointCore(std::istream &in)
{
  multiplier = cRead<decltype(multiplier)>(in); 
} 

void TreeLengthMultiplier::writeToCheckpointCore(std::ostream &out) const
{
  cWrite<decltype(multiplier)>(out, multiplier); 
} 


std::vector<nat> TreeLengthMultiplier::getInvalidatedNodes(const TreeAln& traln) const
{
  auto result = std::vector<nat>{}; 
  for(nat i = traln.getNumberOfTaxa() + 1 ; i < traln.getNumberOfNodes() + 1  ; ++i)
    result.push_back(i); 
  return result; 
} 


