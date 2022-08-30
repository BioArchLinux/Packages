#include "GenericTopoProposal.hpp"
#include "Path.hpp"
#include "TreeRandomizer.hpp"
#include "BranchSetProposer.hpp"
#include "Arithmetics.hpp"

#define NUM_ITER 10

using std::get; 

// since this will not be a production level feature, the parameter is
// hard coded.

// this is the appropriate parameter for a uniform draw in [2/3 ; 1.5] on the exponential scale 
double GenericTopoProposal::multiplier = std::log(2. / 3. ) / -0.5 ; 



GenericTopoProposal::GenericTopoProposal( std::unique_ptr<TopoMoveProposer> moveDet , std::string name , double relWeight, MoveOptMode toOpt  ) 
  : AbstractProposal(Category::TOPOLOGY,  name, relWeight, 0., 0., false)
  , _move(  nullptr )	       
  , _moveProposer{std::move(moveDet)}
  , _moveOptMode{toOpt}
  , _backup{}
  , _useMultiplier{false}
{
}


GenericTopoProposal::GenericTopoProposal(const GenericTopoProposal& rhs)
  :  AbstractProposal(rhs)
  , _move( rhs._move == nullptr ? nullptr : rhs._move->clone())
  , _moveProposer(rhs._moveProposer->clone())
  , _moveOptMode{rhs._moveOptMode}
  , _backup{rhs._backup}
  , _useMultiplier{rhs._useMultiplier}
{ 
}


GenericTopoProposal& GenericTopoProposal::operator=( GenericTopoProposal rhs)  
{
  swap(*this, rhs); 
  return *this;
}


void swap(GenericTopoProposal& lhs, GenericTopoProposal& rhs) 
{
  std::swap(lhs._move, rhs._move); // abstract product  
  std::swap(lhs._moveProposer,rhs._moveProposer); // abstract factory 
  std::swap(lhs._backup, rhs._backup); 
  std::swap(lhs._moveOptMode, rhs._moveOptMode);
  std::swap(lhs._useMultiplier, rhs._useMultiplier);
}


BranchPlain GenericTopoProposal::determinePrimeBranch(const TreeAln &traln, Randomness& rand) const
{
  return _moveProposer->determinePrimeBranch(traln,rand);
}


log_double GenericTopoProposal::proposeAndApplyNewBranches(TreeAln &traln, const std::vector<BranchPlain> &bs, PriorBelief &prior,  Randomness &rand, LikelihoodEvaluator& eval) const 
{
  if(bs.size() == 0)
    return log_double::fromAbs(1.); 

  auto params = getBranchLengthsParameterView(); 

  auto hastForwBranch = log_double::fromAbs(1.); 
  
  if(_useMultiplier && not _moveProposer->hasOptimizedBranches())
    {
      for(auto b : bs)
	{
	  for(auto p : params)
	    {
	      auto multi = rand.drawMultiplier(multiplier);
	      auto meanSubstRate = p->getMeanSubstitutionRate(); 
	      auto bl = traln.getBranch(b,p); 
	      auto oldLen = bl.toMeanSubstitutions(meanSubstRate); 
	      
	      // bl.setConvertedInternalLength( p, oldLen * multi);
	      bl.setLength(InternalBranchLength::fromAbsolute(oldLen * multi, meanSubstRate));
	      if(not BoundsChecker::checkBranch(bl))
		BoundsChecker::correctBranch(bl);
	      auto newLen = bl.toMeanSubstitutions(meanSubstRate); 
	      auto realMulti = newLen / oldLen; 

	      // tout << SHOW(realMulti) << std::endl;
	      
	      traln.setBranch(bl,p);

	      prior.addToRatio(p->getPrior()->getLogProb( ParameterContent{{newLen}}) 
	      		       / p->getPrior()->getLogProb( ParameterContent{{oldLen}}) ) ;

	      hastForwBranch *= log_double::fromAbs(realMulti);
	    }
	}
    }
  else 
    {
      auto getOptParams = [&]() -> std::vector<OptimizedParameter>
	{
	  auto result = std::vector<OptimizedParameter>{}; 

	  if(_moveProposer->hasOptimizedBranches())
	    result =  _moveProposer->getOptimizedBranches();
	  else 
	    {
	      auto bsp = BranchSetProposer(traln, bs, params); 
	      bsp.findJointOptimum(eval, NUM_ITER, false);
	      for(auto elem : bsp.getResult() ) 
		result.insert(end(result), begin(elem.second), end(elem.second));
	    }
	  return result; 
	}; 

      for(auto &optParam : getOptParams())
	{
	  auto p = optParam.getParam();
	  auto b = optParam.getBranch();
	  auto meanSubstRate = p->getMeanSubstitutionRate();

	  auto convParam = p->getAttributes()._convTuner.getParameter();
	  auto nonConvParam = p->getAttributes()._nonConvTuner.getParameter();

	  auto dist = optParam.getProposerDistribution<GammaProposer>(traln, convParam, nonConvParam ); 
	  auto bl = dist.proposeBranch(b, traln, p, rand);
	  
	  auto oldLen = traln.getBranch(b,p).toMeanSubstitutions( meanSubstRate);
	  traln.setBranch(bl, p);
	  auto newLen = bl.toMeanSubstitutions(meanSubstRate); 
	  

	  // tout  << MAX_SCI_PRECISION << SHOW(newLen)<< std::endl; 
	  
	  // update log prior 
	  prior.addToRatio(p->getPrior()->getLogProb( ParameterContent{{newLen}}) 
			   / p->getPrior()->getLogProb( ParameterContent{{oldLen}}) ) ;

	  auto hastHere = dist.getLogProbability(newLen); 
	  hastForwBranch *= hastHere;
	  // tout << MAX_SCI_PRECISION << SHOW(oldLen) << " ->  " << SHOW(newLen) << SOME_FIXED_PRECISION << SHOW(hastHere)<< std::endl; 
	}

    }
  return hastForwBranch; 
}


/* 
   using the backup for this 
*/ 
log_double GenericTopoProposal::assessOldBranches(TreeAln& traln, LikelihoodEvaluator& eval, const std::vector<BranchPlain> &bs, const std::vector<AbstractParameter*> params) 
{
  auto result = log_double::fromAbs(1.);

  if(bs.size() == 0)
    return result;  

  // hastings is covered in the forward part 
  if(_useMultiplier)
    return result; 

  auto bsp = BranchSetProposer(traln, bs, params); 
  bsp.findJointOptimum(eval, NUM_ITER, false); 
  for(auto elem : bsp.getResult ())
    {
      auto b = elem.first; 
      auto& optParams = elem.second; 
      for(auto &optParam : optParams)
	{
	  auto p = optParam.getParam();
	  
	  auto convParam = p->getAttributes()._convTuner.getParameter();
	  auto nonConvParam = p->getAttributes()._nonConvTuner.getParameter();

	  auto found = _backup.find(b,p); 

	  if(not get<0>(found))
	    tout << "error: could not find " << b << std::endl; 

	  auto curLen = get<1>(found).toMeanSubstitutions(p->getMeanSubstitutionRate()); 
	  // tout << MAX_SCI_PRECISION << SHOW(curLen)<< std::endl; 
	  result *= optParam.getProposerDistribution<GammaProposer>(traln, convParam, nonConvParam).getLogProbability(curLen); 
	}
    }
  
  return result; 
}


// TODO EFFICIENCY (marking stuff used)

void GenericTopoProposal::applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) 
{    
  auto primeBranch =  determinePrimeBranch(traln, rand); 
  auto params = getBranchLengthsParameterView(); 

  _moveProposer->determineMove(traln, eval, rand, primeBranch, params); 
  _move = _moveProposer->getMove(); 

  // branches that we have to propose in order to get back to the start 
  auto bsBack = _move->getBranchesToPropose(traln, _moveOptMode); 
  _backup = BranchBackup(traln, bsBack, params);

  // tout << SHOW(_moveOptMode) << std::endl; 

  _move->apply(traln, params ); 
  _move->invalidateArrays(eval, traln, _moveOptMode); 

  // branches that we want to propose as part of the proposal  
  auto bsForw = _move->getInverse()->getBranchesToPropose(traln, _moveOptMode);
  
  auto bHastF = proposeAndApplyNewBranches(traln, bsForw, prior, rand, eval );

  // tout << SHOW(*_move) << std::endl; 

  _moveProposer->determineBackProb(traln, eval,params);

  hastings *= _moveProposer->getProposalDensity();

  _move->getInverse()->apply(traln, params); 
  for(auto n : _move->getDirtyNodes())
    eval.invalidateArray(traln, n);

  auto bHastB = assessOldBranches(traln, eval, bsBack, params );

  _move->apply(traln, params); 
  _move->invalidateArrays(eval, traln, _moveOptMode ); 

  hastings *= ( bHastB / bHastF); 
}


void GenericTopoProposal::evaluateProposal(  LikelihoodEvaluator &evaluator, TreeAln &traln, const BranchPlain &branchSuggestion)
{
  auto evalBranch = _move->getEvalBranch(traln); 
  _move->invalidateArrays(evaluator, traln, _moveOptMode);
  evaluator.evaluate(traln, evalBranch, false); 
}


void GenericTopoProposal::resetState(TreeAln &traln)  
{
  auto params = getSecondaryParameterView(); 
  auto inv = _move->getInverse(); 
  inv->apply(traln,params);
  _backup.resetFromBackup(traln);
}


AbstractProposal* GenericTopoProposal::clone()  const
{
  return new GenericTopoProposal( *this );
}


std::vector<nat> GenericTopoProposal::getInvalidatedNodes(const TreeAln& traln) const
{
  return _move->getDirtyNodes();
}
