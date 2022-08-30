#include "NodeSlider.hpp"
#include "LikelihoodEvaluator.hpp"
#include "BoundsChecker.hpp"
#include "AbstractPrior.hpp"
#include "BranchLength.hpp"

NodeSlider::NodeSlider( double _multiplier)
  : AbstractProposal( Category::BRANCH_LENGTHS, "nodeSlider", 5.,  0,0, false)
  , multiplier(_multiplier)
  , oneBranch{}
  , otherBranch{}
{
}


std::pair<BranchPlain,BranchPlain> NodeSlider::prepareForSetExecution(TreeAln& traln, Randomness &rand) 
{
  assert(_inSetExecution); 

  auto a = determinePrimeBranch(traln, rand); 
  auto descendents = traln.getDescendents(a); 
  auto b = rand.drawRandDouble01() < 0.5 ? descendents.first : descendents.second; 	
  return std::pair<BranchPlain,BranchPlain>(a,b); 
}


BranchPlain NodeSlider::determinePrimeBranch(const TreeAln &traln, Randomness& rand) const
{
  return TreeRandomizer::drawInnerBranchUniform(traln,rand); 
} 


BranchPlain NodeSlider::proposeBranch(const TreeAln &traln, Randomness &rand) const 
{
  if(_inSetExecution)
    {
      return _preparedBranch;
    }
  else  
    {
      return determinePrimeBranch(traln, rand); 
    }
}


BranchPlain NodeSlider::proposeOtherBranch(const BranchPlain &firstBranch, const TreeAln& traln, Randomness& rand) const 
{
  if(_inSetExecution)
    return _preparedOtherBranch; 
  else 
    {
      auto descendents = traln.getDescendents(firstBranch); 
      return rand.drawRandDouble01() < 0.5 ? descendents.first : descendents.second; 	
    }
}


void NodeSlider::prepareForSetEvaluation( TreeAln &traln, LikelihoodEvaluator& eval) const 
{
  nat middleNode = getCommonNode(oneBranch,otherBranch) ;
  nat otherNode = oneBranch.getOtherNode(middleNode); 
  
  auto b = BranchPlain(middleNode, otherNode); 
  nodeptr p = traln.findNodePtr(b);    

  eval.invalidateArray( traln, p->number); 
}


void NodeSlider::applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) 
{
  auto blParams = getPrimaryParameterView(); 
  auto param = blParams[0];
  assert(blParams.size() == 1); 

  // TODO outer branches have a lower probability of getting drawn? =/ 
  oneBranch = traln.getBranch(proposeBranch(traln, rand),param); 
  otherBranch = traln.getBranch(proposeOtherBranch(oneBranch, traln, rand),param); 

  auto oldA = oneBranch.getLength().getValue(),
    oldB = otherBranch.getLength().getValue();

  double bothZ = oldA * oldB; 

  // correct the multiplier 
  double drawnMultiplier = rand.drawMultiplier(multiplier); 
  double newZ = pow(bothZ, drawnMultiplier); 
  auto testBranch = oneBranch; 
  testBranch.setLength(sqrt(newZ)); 
  if(not BoundsChecker::checkBranch(testBranch))
    {
      BoundsChecker::correctBranch(testBranch); 
      newZ = pow(testBranch.getLength( ).getValue(),2); 
      drawnMultiplier = log(newZ)   / log(bothZ); 
    }

  double uniScaler = rand.drawRandDouble01(), 
    newA = pow(newZ, uniScaler ),
    newB = pow(newZ, 1-uniScaler); 

  bool problemWithA = false; 
  testBranch.setLength(newA); 
  if(not BoundsChecker::checkBranch(testBranch))
    {
      BoundsChecker::correctBranch(testBranch); 
      newA = testBranch.getLength().getValue(); 
      problemWithA = true; 
    }
  testBranch.setLength(newB); 
  if(not BoundsChecker::checkBranch(testBranch))
    {
      BoundsChecker::correctBranch(testBranch); 
      newB = testBranch.getLength().getValue(); 
      assert(not problemWithA); // should have been detected, when we checked the multiplier 
    }

  testBranch = oneBranch; 
  testBranch.setLength(newA); 
  // tout << "changing " << oneBranch << " to " << testBranch << std::endl; 
  traln.setBranch(testBranch, param); 
  
  auto lnPrA = param->getPrior()->getUpdatedValue(oneBranch.getLength().getValue(), testBranch.getLength().getValue(), param);

  // auto lnPrA = param->getPrior()->getLogProb( ParameterContent{{ testBranch.getInterpretedLength(param) } } )
  //   /   param->getPrior()->getLogProb( ParameterContent{{ oneBranch.getInterpretedLength(param) } } ); 

  testBranch = otherBranch; 
  testBranch.setLength(newB); 
  // tout << "changing " << otherBranch << " to " << testBranch << std::endl; 
  traln.setBranch(testBranch, param); 

  auto lnPrB = param->getPrior()->getUpdatedValue(otherBranch.getLength().getValue(), testBranch.getLength().getValue(), param); 

  // auto lnPrB = param->getPrior()->getLogProb( ParameterContent{{ testBranch.getInterpretedLength(param) } } )
  //   /   param->getPrior()->getLogProb( ParameterContent{{ otherBranch.getInterpretedLength(param) } } ); 

  // AbstractProposal::updateHastingsLog(hastings, log(), _name); 
  hastings *= log_double::fromAbs(pow(drawnMultiplier,2));

  prior.addToRatio(lnPrA * lnPrB); 
}

void NodeSlider::evaluateProposal(  LikelihoodEvaluator &evaluator, TreeAln &traln, const BranchPlain &branchSuggestion) 
{
  nat middleNode = getCommonNode(oneBranch,otherBranch) ;
  nat otherNode = oneBranch.getOtherNode(middleNode); 
  assert(_primParamIds.size() == 1); 
  auto b = BranchPlain(middleNode, otherNode); 

  auto parts = _allParams->at(_primParamIds[0])->getPartitions(); 

  for(auto aNode : getInvalidatedNodes(traln) ) 
    { 
      for(auto part : parts)
	evaluator.invalidateArray(traln, part, aNode); 
    }

#ifdef PRINT_EVAL_CHOICE
  tout << "EVAL "  << b << std::endl; 
#endif

  evaluator.evaluatePartitionsWithRoot(traln,b, parts, false); 
}


void NodeSlider::resetState(TreeAln &traln) 
{
  auto view = getPrimaryParameterView();
  assert(view.size() == 1);

  traln.setBranch(oneBranch, view[0]); 
  traln.setBranch(otherBranch, view[0]); 
}


AbstractProposal* NodeSlider::clone() const
{
  return new NodeSlider(*this); 
}  


std::vector<nat> NodeSlider::getInvalidatedNodes(const TreeAln& traln) const
{
  nat middleNode = getCommonNode(oneBranch,otherBranch) ;
  return { middleNode } ; 
}
