#include <sstream>		
#include <map> 
#include <unordered_map>
#include <algorithm>

#include "TopologyParameter.hpp"

#include "NodeSlider.hpp"
#include "Chain.hpp"		
#include "TreeAln.hpp"
#include "Randomness.hpp"
#include "GlobalVariables.hpp"

#include "BranchLengthOptimizer.hpp"

#include "LikelihoodEvaluator.hpp" 
#include "ParallelSetup.hpp"
#include "Category.hpp"
#include "TreePrinter.hpp"

// // TODO not too much thought went into this  

// actually, i do not think this is a good idea, but too afaid of
// removing it at this point in the release cycle
namespace std 
{
  template<> struct hash<AbstractParameter*>
  {
  // public: 
    size_t operator()(const AbstractParameter* rhs) const 
    {
       return rhs->getId(); 
    }
  } ;

  template<>
  struct equal_to<AbstractParameter*>
  {
    bool operator()(const AbstractParameter* a, const AbstractParameter* b ) const 
    {
      return a->getId() == b->getId(); 
    }
  }; 
}


Chain:: Chain(randKey_t seed, const TreeAln& traln,  ParameterList params, const std::vector<std::unique_ptr<AbstractProposal> > &proposals, 
	      std::vector<ProposalSet> proposalSets, LikelihoodEvaluator eval, bool isDryRun) 
  : Serializable()
  ,  _traln(traln)
  , _deltaT(0)
  , _runid(0)
  , _tuneFrequency(100)
  , _hastings(log_double::fromAbs(1))
  , _currentGeneration(0)
  , _couplingId(0)
  , _proposals{}
  , _proposalSets(proposalSets)
  , _chainRand(seed)
  , _relWeightSumSingle(0)
  , _relWeightSumSets(0)
  , _prior{}
  , _bestState(log_double::lowest())
  , _evaluator(eval)
  , _lnPr{log_double::fromAbs(1.)}
  , _params{params}
{
  for(auto &p : proposals)
    {
      _proposals.emplace_back(p->clone()); 
      _proposals.back()->setParams(&_params); 
    }

  for(auto &p : _proposalSets)
    p.setParameterListPtr(&_params);


  // experimental: initilize with random starting values 
  // for(auto &p : _params)
  //   {
  //     if(dynamic_cast<BranchLengthsParameter*>(p) != nullptr)
  // 	{
  // 	  for(auto bl : _traln.extractBranches(p))
  // 	    {
  // 	      auto newVal = p->getPrior()->drawFromPrior(_chainRand);
  // 	      tout << "init with " << newVal << std::endl; 
  // 	      bl.setConvertedInternalLength(p,newVal.values[0]); 
  // 	      if(BoundsChecker::checkBranch(bl))
  // 		BoundsChecker::correctBranch(bl); 
  // 	      _traln.setBranch(bl,p);
  // 	    }
  // 	}
  //   }

  _prior.initialize(_traln, _params);
  suspend(); 
  updateProposalWeights();

}



Chain::Chain(  const Chain& rhs)   
  : Serializable(rhs)
  ,  _traln(rhs._traln)
  , _deltaT(rhs._deltaT)
  , _runid(rhs._runid)
  , _tuneFrequency(rhs._tuneFrequency)
  , _hastings(rhs._hastings)
  , _currentGeneration(rhs._currentGeneration)
  , _couplingId(rhs._couplingId) 
  , _proposals{}
  , _proposalSets(std::move(rhs._proposalSets))
  , _chainRand(std::move(rhs._chainRand)) 
  , _relWeightSumSingle(rhs._relWeightSumSingle)
  , _relWeightSumSets(rhs._relWeightSumSets) 
  , _prior(std::move(rhs._prior))
  , _bestState(rhs._bestState)
  , _evaluator(std::move(rhs._evaluator))
  , _lnPr(rhs._lnPr)
  , _params{rhs._params}
{
  for(auto &p : rhs._proposals)
    {
      _proposals.emplace_back(p->clone());
      _proposals.back()->setParams(&_params);
    }

  for(auto &p : _proposalSets)
    p.setParameterListPtr(&_params);
}

 
Chain& Chain::operator=(Chain rhs)
{
  swap(*this, rhs); 
  return *this; 
}


void swap(Chain &lhs, Chain &rhs)
{
  using std::swap; 

  swap(lhs._traln,rhs._traln); 
  swap(lhs._deltaT,rhs._deltaT);
  swap(lhs._runid,rhs._runid);
  swap(lhs._tuneFrequency,rhs._tuneFrequency);
  swap(lhs._hastings,rhs._hastings);
  swap(lhs._currentGeneration,rhs._currentGeneration);
  swap(lhs._couplingId,rhs._couplingId); 
  swap(lhs._proposals,rhs._proposals);
  swap(lhs._proposalSets,rhs._proposalSets);
  swap(lhs._chainRand,rhs._chainRand);
  swap(lhs._relWeightSumSingle,rhs._relWeightSumSingle);
  swap(lhs._relWeightSumSets,rhs._relWeightSumSets); 
  swap(lhs._prior,rhs._prior);
  swap(lhs._bestState,rhs._bestState);
  swap(lhs._evaluator,rhs._evaluator);
  swap(lhs._lnPr,rhs._lnPr);
  swap(lhs._params, rhs._params); 
}


void swapHeatAndProposals(Chain &lhs, Chain& rhs)
{
  std::swap(lhs._bestState,    rhs._bestState); 
  std::swap(lhs._couplingId,   rhs._couplingId    ); 
  std::swap(lhs._proposals,    rhs._proposals     ); 
  std::swap(lhs._proposalSets, rhs._proposalSets  ); 
}


std::ostream& Chain::addChainInfo(std::ostream &out) const 
{
  return out << "[run=" << _runid << ",heat=" << _couplingId << ",gen=" << getGeneration() << "]" ; 
}

 
double Chain::getChainHeat() const 
{
  double tmp = 1. + ( _deltaT * _couplingId ) ; 
  double inverseHeat = 1.f / tmp; 
  assert(_couplingId == 0 || inverseHeat < 1.); 
  return inverseHeat; 
}


void Chain::updateProposalWeights()
{
  // prepare proposals 
  _relWeightSumSingle = 0; 
  for(auto& elem : _proposals)
    _relWeightSumSingle +=  elem->getRelativeWeight();
  
  _relWeightSumSets = 0; 
  for(auto &set : _proposalSets)
    _relWeightSumSets += set.getRelativeWeight();
}


void Chain::suspend()  
{
  _traln.clearMemory(_evaluator.getArrayReservoir()); 
  _lnPr = _prior.getLnPrior();
  // tout << "SUSPEND " <<  MAX_SCI_PRECISION << _lnPr << std::endl; 

#if 0 
  // auto params =  getBranchLengthsParameterView(); 
  AbstractParameter* param = (nullptr); 
  for(auto p : _params)
    if(dynamic_cast<BranchLengthsParameter*>(p) != nullptr)
      param = p; 
  auto bs = _traln.extractBranches(param);
  tout << MAX_SCI_PRECISION << "SUSPEND " << bs << std::endl; 
#endif

  _evaluator.freeMemory(); 
}


void Chain::resume() 
{    

#if 0 
  AbstractParameter* param = (nullptr); 
  for(auto p : _params)
    if(dynamic_cast<BranchLengthsParameter*>(p) != nullptr)
      param = p; 
  auto bs = _traln.extractBranches(param);
  tout  << MAX_SCI_PRECISION << "RESUME " << bs << std::endl; 
#endif

  // auto vs = getParamView(); 
  _prior.initialize(_traln, _params);
  auto prNow = _prior.getLnPrior(); 
  
  // tout << "RESUME " <<  prNow  << std::endl; 

  if(   std::fabs(log_double(  _lnPr / prNow ).toAbs()) - 1. >= ACCEPTED_LNPR_EPS)
    {
      std::cerr << MAX_SCI_PRECISION ; 
      std::cerr << "While trying to resume chain: previous log prior could not be\n"
  		<< "reproduced. This is a programming error." << std::endl; 
      std::cerr << "Prior was " << _lnPr << "\t now we have " << prNow << 
	"\tdiff=" << std::fabs(_lnPr.getRawLog() - prNow.getRawLog()) << 
	std::endl; 
      assert(0); 
    }

  updateProposalWeights();
}


ProposalSet& Chain::drawProposalSet(Randomness &rand)
{
  double r = _relWeightSumSets * rand.drawRandDouble01(); 

  for(auto &c : _proposalSets )
    {
      double w = c.getRelativeWeight(); 
      if(r < w )
	return c; 
      else 
	r -= w ; 
    }

  assert(0); 
  return _proposalSets[0]; 
}


AbstractProposal& Chain::drawProposalFunction(Randomness &rand)
{ 
  double r = _relWeightSumSingle * rand.drawRandDouble01();   

  for(auto& c : _proposals)
    {
      double w = c->getRelativeWeight(); 
      if(r < w )
	{
	  // std::cout << "drawn " << c.get() << std::endl; 
	  return *c;
	}
      else 
	r -= w; 
    }

  assert(0); 
  return *(_proposals[0]); 
}


void Chain::serializeConditionally( std::ostream &out, CommFlag commFlags)  const 
{
  if( ( commFlags & CommFlag::PRINT_STAT )  != CommFlag::NOTHING)
    {
      cWrite<decltype(_couplingId)>(out, _couplingId);
      cWrite<decltype(_bestState)>(out, _bestState);
      
      auto lnl = getLikelihood(); 
      cWrite<decltype(lnl)>(out, lnl); 

      cWrite<decltype(_lnPr)>(out, _lnPr); 
      cWrite<decltype(_currentGeneration)>(out,_currentGeneration);
    }

  if( (commFlags & CommFlag::RAND) != CommFlag::NOTHING)
    _chainRand.serialize(out);

  if( ( commFlags & CommFlag::PROPOSALS ) != CommFlag::NOTHING )
    {
      for(auto& p : _proposals)
	p->serialize(out); 

      for(auto& p: _proposalSets)
	p.serialize(out);

      for(auto &p : _params)
	p->serialize(out); 
    }

  if( ( commFlags & CommFlag::TREE ) != CommFlag::NOTHING )
    {
       for(auto &var: _params)
	 {
           auto compo = var->extractParameter(_traln);
	  auto&&  tmp = std::stringstream{}; 
	  var->printShort(tmp); 	  	  
	  writeString(out,tmp.str()); 
	  compo.serialize(out);
	}
    }
}


void Chain::deserializeConditionally(std::istream& in, CommFlag commFlags)
{    
  if( ( commFlags & CommFlag::PRINT_STAT ) != CommFlag::NOTHING )
    {
      _couplingId = cRead<decltype(_couplingId)>(in); 
      _bestState = cRead<log_double>(in); 
      setLikelihood(cRead<log_double>(in) ); 

      _lnPr = cRead<log_double>(in); 
      _currentGeneration = cRead<decltype(_currentGeneration)>(in);
    }

  if( (commFlags & CommFlag::RAND) != CommFlag::NOTHING)
    _chainRand.deserialize(in);

  if( ( commFlags & CommFlag::PROPOSALS ) != CommFlag::NOTHING )
    {
      for(auto &p : _proposals)
	p->deserialize(in);

      for(auto &p : _proposalSets)
	p.deserialize(in);

      for(auto &p : _params)
	p->deserialize(in); 	// dubios decision: serialized version of a parameter could also be the content...
    }

  if( ( commFlags & CommFlag::TREE ) != CommFlag::NOTHING )
    {
      auto param2content = std::unordered_map<AbstractParameter*,ParameterContent>{}; 
      
      for(auto &param : _params)
	{
	  auto name = readString(in);
	  {
	    auto&& tmp = std::stringstream{}; 
	    param->printShort(tmp);
	    assert( tmp.str().compare(name) == 0 ); 
	  }
	  auto content = param->extractParameter(_traln); // initializes the object correctly. the object must "know" how many values are to be extracted 
	  content.deserialize(in);

	  param2content.insert(std::make_pair(param, content)); 
	}
      
      applyParameterContents(param2content);
    }
}




BranchPlain Chain::peekNextVirtualRoot(TreeAln &traln, Randomness rand)  
{
  // TODO go beyond next step  

  auto curGen = rand.getGeneration();

  rand.rebaseForGeneration(curGen + 1 ); 

  double sum = _relWeightSumSingle + _relWeightSumSets; 
  
  auto branch = BranchPlain(); 

  if( rand.drawRandDouble01() * sum < _relWeightSumSingle) 
    {
      auto &pfun = drawProposalFunction(rand); 
      branch = pfun.determinePrimeBranch(traln, rand); 
    }
  else 
    {
      auto pset = drawProposalSet(rand); 
      branch = pset.getProposalView()[0]->determinePrimeBranch(traln, rand); 
    }

  if(branch.getPrimNode() == 0 || branch.getSecNode() == 0)
    {
      // TODO better draw  
      branch = TreeRandomizer::drawInnerBranchUniform(traln, rand); 
#ifdef PRINT_EVAL_CHOICE
      tout << "RANDOM " << branch<< std::endl; 
#endif
      // tout << "no prediction, just returning a somewhat inner branch " << branch  << std::endl; 
    }
  else 
    {
#ifdef PRINT_EVAL_CHOICE
      tout << "PREDICT " << branch << std::endl; 
#endif
    }
  

  return branch; 
}



void Chain::stepSingleProposal()
{
  auto &traln = _traln; 
  auto prevLnl = getLikelihood(); 
  auto& pfun = drawProposalFunction(_chainRand);

  /* reset proposal ratio  */
  _hastings = log_double::fromAbs(1.); 

  pfun.applyToState(_traln, _prior, _hastings, _chainRand, _evaluator);
  
  assert(not _hastings.isNaN()) ; 

  // can be ignored
  auto suggestion = peekNextVirtualRoot(traln,_chainRand); 

  pfun.evaluateProposal(_evaluator, _traln, suggestion);

  auto priorRatio = _prior.getLnPriorRatio();
  auto lnlRatio = traln.getLikelihood() / prevLnl; 

  double testr = _chainRand.drawRandDouble01();
  double acceptance = fmin(    log_double( exponentiate( priorRatio * lnlRatio, getChainHeat() ) * _hastings).toAbs()  ,1.) ; 

  bool wasAccepted  = testr < acceptance; 

#ifdef DEBUG_SHOW_EACH_PROPOSAL 
  auto& output = tout  ; 
  // auto& output = std::cout  ; 
  addChainInfo(output); 
  output << "\t" << (wasAccepted ? "ACC" : "rej" )  << "\t"<< pfun.getName() << "\t" 
	 << MORE_FIXED_PRECISION << SHOW(prevLnl) << SHOW(lnlRatio) << SHOW(priorRatio) << SHOW( _hastings) << SHOW(acceptance)  << SHOW(testr)<< std::endl; 
#endif

  if(wasAccepted)
    {
      pfun.accept();      
      _prior.accept();
      if(_bestState < traln.getLikelihood()  )
	_bestState = traln.getLikelihood(); 
      _lnPr = _prior.getLnPrior();
    }
  else
    {
      pfun.resetState(traln);
      pfun.reject();
      _prior.reject();
      
      auto myRejected = std::vector<bool>(traln.getNumberOfPartitions(), false); 
      for(auto &elem : pfun.getAffectedPartitions())
	myRejected[elem] = true; 

      auto nodes = pfun.getInvalidatedNodes(traln); 
      _evaluator.accountForRejection(traln, myRejected, nodes); 

    }

  _evaluator.freeMemory();


  if( _tuneFrequency > 0 && _tuneFrequency <  pfun.getNumCallSinceTuning()   ) 
    pfun.autotune();
}




/** 
    whenever we apply a bunch of stored parameters, we MUST apply the
    topology parameter BEFORE the branch length parameter (otherwise,
    we may not correctly reproduce the state)
    
    the state of a chain (wrt parameter values) could be
    encapsulated...

    TODO: use this specific function also in all other occasions...
 */ 
void Chain::applyParameterContents(std::unordered_map<AbstractParameter*,ParameterContent> param2content)  
{
  auto topoParams =  std::unordered_map<AbstractParameter*,ParameterContent>{}; 
  auto otherParams = std::unordered_map<AbstractParameter*,ParameterContent>{}; 
  
  for(auto elem : param2content)
    if(dynamic_cast<TopologyParameter*>(elem.first) != nullptr) 
      topoParams.insert(elem); 

  for(auto elem : param2content)
    if(dynamic_cast<TopologyParameter*>(elem.first) == nullptr) 
      otherParams.insert(elem); 

  for(auto &elem : topoParams)
    elem.first->applyParameter(_traln, elem.second);
  
  for(auto &elem : otherParams)
    elem.first->applyParameter(_traln, elem.second); 
}



void Chain::stepSetProposal()
{
  auto &pSet = drawProposalSet(_chainRand); 

  auto oldPartitionLnls = _traln.getPartitionLnls(); 
  
  auto p2Hastings =  std::unordered_map<AbstractProposal*, log_double>{} ; 
  auto p2LnPriorRatio = std::unordered_map<AbstractProposal*, log_double>{}; 
  auto p2OldLnl = std::unordered_map<AbstractProposal*, log_double>{} ; 

  auto affectedPartitions = std::vector<nat>{}; 
  auto proposals = pSet.getProposalView(); 
  auto branches = proposals.at(_chainRand.drawIntegerOpen(proposals.size()))->prepareForSetExecution(_traln, _chainRand);

  // branch length proposals are all executed on the same branch. In
  // case of the node slider, we need two adjacent branches. One of
  // the proposals in the set determines which branch we are dealing
  // with
  for(auto &proposal: proposals)
    {
      proposal->setPreparedBranch(branches.first);
      proposal->setOtherPreparedBranch(branches.second);
    }
  
  // get all parameters and affected partitions for which we actually
  // do something in this proposal set (e.g., if this is a AA
  // proposal, we would not do anything to DNA partitions)
  auto affectedParameters = std::vector<AbstractParameter*>();  
  for(auto &proposal: proposals)
    {
      auto param = proposal->getPrimaryParameterView()[0]; 
      affectedParameters.push_back(param);
      
      auto partitions = param->getPartitions();
      affectedPartitions.insert(end(affectedPartitions), begin(partitions), end(partitions));
    }
  
  // stuff that is specific to the newton-raphson optimizing proposal
  // sets (i.e., blDistGamma)
  if( proposals[0]->isUsingOptimizedBranches() )
    {

      auto relevantBranch = branches.first;  
      _evaluator.evaluateSubtrees(_traln, relevantBranch,affectedPartitions , false);

      _evaluator.evaluatePartitionsDry(_traln, relevantBranch, affectedPartitions ); 

      auto blo = BranchLengthOptimizer(_traln, relevantBranch, 30, _evaluator.getParallelSetup().getChainComm(), affectedParameters);
      blo.optimizeBranches(_traln);
      auto optParams = blo.getOptimizedParameters();

      assert(optParams.size() == affectedParameters.size()); 
      for(nat i = 0; i < optParams.size() ; ++i)
	proposals[i]->extractProposer(_traln, optParams[i]);
    }

  // apply each proposal, reset priors and hastings after each proposal  
  for(auto &proposal : proposals)
    {
      auto  lHast = log_double::fromAbs(1.); 
      _prior.reject();
      proposal->applyToState(_traln, _prior, lHast, _chainRand, _evaluator); 
      p2LnPriorRatio[proposal] = _prior.getLnPriorRatio(); 
      p2Hastings[proposal] = lHast; 

      for(auto &p : proposal->getPrimaryParameterView())
	{
	  auto partitions = p->getPartitions(); 
	  auto lnl = log_double::fromAbs(1.0); 
	  for(auto &partition : partitions)
	    lnl *= oldPartitionLnls[partition]; 
	  p2OldLnl[proposal] = lnl; 
	}
    }

  bool fullTraversalNecessary = pSet.needsFullTraversal();
  
  // all proposals are applied, now only evaluate once 
  if( 
     // branches.first.equalsUndirected(BranchPlain(0,0))
     branches.first == BranchPlain(0,0) 
      ) // TODO another HACK
    {
      // this should be a reasonable suggestion 
      auto nextRoot = peekNextVirtualRoot(_traln, _chainRand); 
      _evaluator.evaluatePartitionsWithRoot(_traln, nextRoot, affectedPartitions, fullTraversalNecessary); 
    }
  else 
    {
      pSet.getProposalView()[0]->prepareForSetEvaluation(_traln, _evaluator);
      _evaluator.evaluatePartitionsWithRoot(_traln, branches.first , affectedPartitions, fullTraversalNecessary );
    }

  auto newPLnls = _traln.getPartitionLnls();

  _prior.reject();		// slight abuse 
  
  // decide upon acceptance, rejection of single proposals and account for it 
  auto partitionsToReset = std::vector<bool>(_traln.getNumberOfPartitions() , false); 
  nat accCtr = 0; 
  nat total = 0; 
  auto p2WasAccepted = std::unordered_map<AbstractProposal*, bool>{} ; 
  for(auto &proposal : pSet.getProposalView())
    {
      ++total; 
      auto newLnl = log_double::fromAbs(1.0) ;  
      for(auto var : proposal->getPrimaryParameterView())
	for(auto p : var->getPartitions()) 
	  newLnl *= newPLnls[p]; 
 
      auto accRatio = log_double(exponentiate(   p2LnPriorRatio[proposal] * ( newLnl / p2OldLnl[proposal]) ,  getChainHeat()) * p2Hastings[proposal]).toAbs();

      if(_chainRand.drawRandDouble01() < accRatio)
	{
	  ++accCtr;
	  proposal->accept();
	  p2WasAccepted[proposal] = true; 
	}
      else 
	{
	  // TODO prior more efficient 
	  proposal->resetState(_traln);
	  proposal->reject();	  

	  for(auto& param: proposal->getPrimaryParameterView())
	    {
	      for(auto p : param->getPartitions())
		partitionsToReset[p] = true; 
	    }
	  p2WasAccepted[proposal] = false; 
	}      
    }

  auto nodes = pSet.getProposalView()[0]->getInvalidatedNodes(_traln); 
  _evaluator.accountForRejection(_traln, partitionsToReset, nodes); 

  auto lnl = _traln.getLikelihood(); 
  _lnPr = _prior.getLnPrior();
  
  if(_bestState < lnl)
    _bestState = lnl; 

#ifdef DEBUG_SHOW_EACH_PROPOSAL
  auto &output = tout ; 

  addChainInfo(output);
  output << "\t" << accCtr << "/"  << total << "\t" << pSet << "\t" << lnl << std::endl; 
#endif

  // tune proposals, if it time 
  for(auto &proposal : pSet.getProposalView())
    if( this->_tuneFrequency > 0  && this->_tuneFrequency < proposal->getNumCallSinceTuning()) // meh
      proposal->autotune(); 

  _prior.reject();
  tout << MAX_SCI_PRECISION; 
  for(auto &proposal : pSet.getProposalView())
    {
      if(p2WasAccepted[proposal])
	{
	  auto tmp = p2LnPriorRatio.at(proposal); 
	  _prior.addToRatio(tmp); 
	}
    }

  _evaluator.freeMemory();
  _prior.accept();
}


void Chain::step()
{
  ++_currentGeneration; 

#ifdef DEBUG_VERIFY_LNPR
  _prior.verifyPrior(_traln, _params);
#endif

  _evaluator.imprint(_traln);
  // inform the rng that we produce random numbers for generation x  
  _chainRand.rebaseForGeneration(_currentGeneration);

  double sum = _relWeightSumSingle + _relWeightSumSets; 
  if(_chainRand.drawRandDouble01() * sum < _relWeightSumSingle)
    stepSingleProposal();
  else 
    stepSetProposal();

#ifdef DEBUG_LNL_VERIFY
  _evaluator.expensiveVerify(_traln, _traln.getAnyBranch() , getLikelihood()); 
#endif

#ifdef DEBUG_VERIFY_LNPR
  _prior.verifyPrior(_traln, _params );
#endif
}


const std::vector<AbstractProposal*> Chain::getProposalView() const 
{
  auto result = std::vector<AbstractProposal*>{};  
  for(auto &elem: _proposals)
    result.push_back(elem.get()); 
  return result; 
}


std::ostream& operator<<(std::ostream& out, const Chain &rhs)
{
  rhs.addChainInfo(out); 
  // out  << "\tLnl: " << rhs.tralnPtr->getTr()->likelihood << "\tLnPr: " << rhs.prior.getLnPrior();
  out  << "\tLnl: " << rhs.getLikelihood() << "\tLnPr: " << rhs._lnPr; 
  return out;  
}


void Chain::sample(  std::unordered_map<nat,TopologyFile> &paramId2TopFile ,  ParameterFile &pFile ) const
{
  auto blParamsUnfixed=  std::vector<AbstractParameter*>{} ; 
  AbstractParameter *topoParamUnfixed = nullptr; 
  
  for(auto &param : _params  ) 
    {
      if(param->getCategory() == Category::BRANCH_LENGTHS && param->getPrior()->needsIntegration() )
	blParamsUnfixed.push_back(param); 
      else if(param->getCategory() == Category::TOPOLOGY && param->getPrior()->needsIntegration() )
	topoParamUnfixed = param  ; 
    }

  if(blParamsUnfixed.size() > 0)
    {
      for(auto &param : blParamsUnfixed)
	{
	  nat myId = param->getId(); 
	  auto &f = paramId2TopFile.at(myId); 
	  f.sample(_traln, getGeneration(), param); 
	}
    }
  else if(topoParamUnfixed != nullptr)
    {
      auto &f = paramId2TopFile.at(topoParamUnfixed->getId()); 
      f.sample(_traln, getGeneration(), topoParamUnfixed); 
    }

  pFile.sample( _traln, _params, getGeneration(), _prior.getLnPrior()); 
}

     
void Chain::serialize( std::ostream &out) const
{
  serializeConditionally(out, CommFlag::PRINT_STAT | CommFlag::PROPOSALS | CommFlag::TREE | CommFlag::RAND ) ;
}  


void Chain::deserialize( std::istream &in ) 
{
  deserializeConditionally(in, CommFlag::PRINT_STAT | CommFlag::PROPOSALS | CommFlag::TREE | CommFlag::RAND ) ;
}


void Chain::resetParamPtr()
{
  for(auto &p : _proposals)
    p->setParams(&_params); 

  for(auto &p : _proposalSets)
    p.setParameterListPtr(&_params); 
} 
// c++<3
