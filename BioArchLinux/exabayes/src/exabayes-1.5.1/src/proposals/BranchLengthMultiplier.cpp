#include "BranchLengthMultiplier.hpp"
#include "TreeAln.hpp"
#include "BoundsChecker.hpp"
#include "TreeRandomizer.hpp"
#include "AbstractPrior.hpp"


BranchLengthMultiplier::BranchLengthMultiplier(  double multiplier)
  : AbstractProposal(Category::BRANCH_LENGTHS, "blMult", 7., 0.0001,  100, false)
  , _multiplier(multiplier)
  , _savedBranch{}
{
}


BranchPlain BranchLengthMultiplier::proposeBranch(const TreeAln &traln, Randomness &rand) const 
{
  if(_inSetExecution)
    return _preparedBranch;
  else  
    return determinePrimeBranch(traln,rand); 
}   


BranchPlain BranchLengthMultiplier::determinePrimeBranch(const TreeAln &traln, Randomness& rand) const 
{
  return TreeRandomizer::drawBranchUniform(traln, rand); 
} 


void BranchLengthMultiplier::applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) 
{
  auto b = BranchLength(proposeBranch(traln, rand)); 
  
  assert(_primParamIds.size() == 1); 
  auto param = _allParams->at(_primParamIds[0]); 

  _savedBranch = traln.getBranch(b, param); 

  double oldZ = _savedBranch.getLength().getValue();

  double
    drawnMultiplier = 0 ,
    newZ = oldZ; 

  drawnMultiplier= rand.drawMultiplier( _multiplier); 
  // tout << SHOW(drawnMultiplier) << std::endl; 
  assert(drawnMultiplier > 0.); 
  newZ = pow( oldZ, drawnMultiplier);

  // tout << MAX_SCI_PRECISION << "proposed " << newZ <<  " from " << multiplier << " and oldBranch=" << savedBranch << std::endl; 
  
  b.setLength(InternalBranchLength(newZ)); 

  if(not BoundsChecker::checkBranch(b))
    BoundsChecker::correctBranch(b); 
  traln.setBranch(b, param); 

  double realMultiplier = log(b.getLength().getValue()) / log(oldZ); 

  hastings *= log_double::fromAbs(realMultiplier); 

  prior.addToRatio(param->getPrior()->getUpdatedValue(oldZ, b.getLength().getValue(), param));
}


void BranchLengthMultiplier::evaluateProposal(LikelihoodEvaluator &evaluator,TreeAln &traln, const BranchPlain &branchSuggestion) 
{
  assert(_primParamIds.size() == 1 ); 
  auto parts = _allParams->at(_primParamIds[0])->getPartitions();
  
#ifdef PRINT_EVAL_CHOICE
  tout << "EVAL: " << savedBranch << std::endl; 
#endif
  evaluator.evaluatePartitionsWithRoot(traln,_savedBranch, parts, false); 

  // tout << SOME_FIXED_PRECISION << "lnl=" << traln.getTrHandle().likelihood << std::endl; 
}

 
void BranchLengthMultiplier::resetState(TreeAln &traln) 
{
  assert(_primParamIds.size() == 1)  ; 
  auto params = getBranchLengthsParameterView(); 
  assert(params.size() == 1); 
  auto param = params[0]; 
  traln.setBranch( _savedBranch, param); 
}


void BranchLengthMultiplier::autotune() 
{
  double ratio = _sctr.getRatioInLastInterval(); 
  double newParam = tuneParameter(_sctr.getBatch(), ratio , _multiplier, false);

  // tout << MAX_SCI_PRECISION << SHOW(_multiplier) << " -> " << SHOW(newParam) << std::endl; 

  // tout << MAX_SCI_PRECISION <<   SHOW(_minTuning) << SHOW(newParam) << SHOW(_maxTuning) << std::endl; 
  
  _multiplier = newParam; 

  _sctr.nextBatch();
}


AbstractProposal* BranchLengthMultiplier::clone() const
{
  return new BranchLengthMultiplier(*this);
}


void BranchLengthMultiplier::readFromCheckpointCore(std::istream &in) 
{
  _multiplier = cRead<decltype(_multiplier)>(in);
} 


void BranchLengthMultiplier::writeToCheckpointCore(std::ostream &out) const
{
  cWrite<decltype(_multiplier)>(out, _multiplier); 
} 


std::pair<BranchPlain,BranchPlain> BranchLengthMultiplier::prepareForSetExecution(TreeAln &traln, Randomness &rand) 
{
  assert(_inSetExecution); 
  return std::pair<BranchPlain,BranchPlain>( 
					    determinePrimeBranch(traln,rand),
					    BranchPlain(0,0)); 
}


std::vector<nat> BranchLengthMultiplier::getInvalidatedNodes(const TreeAln &traln ) const 
{
  return {}; 
}

