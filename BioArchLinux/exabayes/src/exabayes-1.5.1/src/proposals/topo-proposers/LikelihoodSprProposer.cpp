#include "LikelihoodSprProposer.hpp"
#include "TreeRandomizer.hpp"
#include "MoveEnumeration.hpp"
#include "BranchSetProposer.hpp"

using std::get; 

double LikehoodSprProposer::weightEps = 1e-100; 

#define MAX_ITER 10 


LikehoodSprProposer::LikehoodSprProposer(nat maxStep, double likeWarp, MoveOptMode toOpt)
  : _maxStep{maxStep}
  , _likeWarp{likeWarp}
  , _move{}
  , _moveOptMode{toOpt}
{
  this->_withOptimizedBranches = true; 
}

  
void LikehoodSprProposer::determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params)  
{
  auto prunedTree = primeBranch; 

  // tout << SHOW(prunedTree) << std::endl; 
  auto insertions = computeLikelihoodsOfInsertions(traln,eval, prunedTree, params);
  insertions = transformLikelihoods(insertions);

  auto myR = rand.drawRandDouble01(); 

  auto choice = []( const std::vector<InsertionResult>& ins, double r ) -> InsertionResult
    {
      for(auto &v : ins)
	{
	  r -= v.getInsertionProb().toAbs(); 
	  if(r <= 0 )
	    return v; 
	}

      assert(0); 
      return ins.at(0); 
    }; 
  
  auto chosen = choice(insertions, myR); 
  
  _move = SprMove(traln, prunedTree, chosen.getBranch() ); 
  _forwProb = chosen.getInsertionProb();
  _optBranches = chosen.getOptParams();
}


void LikehoodSprProposer::determineBackProb(TreeAln &traln, LikelihoodEvaluator& eval, const std::vector<AbstractParameter*> &params) 
{
  // tout << "\nBACK\n" << std::endl; 

  auto prunedTree = _move.getEvalBranch(traln); 

  auto insertsForRevMove = computeLikelihoodsOfInsertions(traln, eval, prunedTree, params); 
  insertsForRevMove = transformLikelihoods(insertsForRevMove); 

  auto invMove = _move.getInverseMove(); 
  auto backBranch =  invMove.getInsertBranch(); 
  
  // find the correct element for the reverse move
  auto backInsertIter = std::find_if(begin(insertsForRevMove), end(insertsForRevMove),
				     [&]( const InsertionResult& elem) 
				     { 
				       // return elem.getBranch().equalsUndirected(backBranch); 
				       return elem.getBranch() == backBranch 
				       ||  elem.getBranch() == backBranch.getInverted(); 
				     }); 
  assert(backInsertIter != end(insertsForRevMove)); 
  
  _backProb = backInsertIter->getInsertionProb(); 
}


BranchPlain LikehoodSprProposer::determinePrimeBranch(const TreeAln &traln, Randomness& rand) const 
{
  auto prunedTree = BranchPlain();
  nodeptr p, pn, pnn;  
  do 
    {
      prunedTree  = TreeRandomizer::drawBranchWithInnerNode(traln,rand); 
      p = traln.findNodePtr(prunedTree);
      pn = p->next->back; 
      pnn = p->next->next->back;         

    } while( (traln.isTipNode(pn) &&  traln.isTipNode(pnn)) ); 

  return prunedTree; 
}
 

TopoMoveProposer* LikehoodSprProposer::clone() const 
{
  return new LikehoodSprProposer(*this); 
}


std::unique_ptr<TopoMove> LikehoodSprProposer::getMove() const  
{
  return std::unique_ptr<TopoMove>(_move.clone()); 
}

 
std::vector<InsertionResult>  
LikehoodSprProposer::computeLikelihoodsOfInsertions(TreeAln &traln, LikelihoodEvaluator &eval, const BranchPlain& prunedTree, const std::vector<AbstractParameter*> &params)  
{
  auto result = std::vector<InsertionResult>  {}; 

  auto moves = MoveEnumeration::getMovesFromDepthFirstSearch(traln, prunedTree, int(_maxStep)); 

  auto prevMove = SprMove();

  for(auto move : moves )
    {
      move.apply(traln, params);
      auto branches = move.getInverse()->getBranchesToPropose(traln, _moveOptMode); 

      if(_moveOptMode == MoveOptMode::NONE || _moveOptMode == MoveOptMode::ONLY_SWITCHING) 
	{
	  auto setdiff = move.getDirtyWrtPrevious(prevMove)	  ; 
	  // tout << "SET-DIFF: " << SHOW(move) << SHOW(prevMove) << SHOW(setdiff)<< std::endl; 
	  for(auto n : setdiff ) 
	    eval.invalidateArray(traln,n);
	  eval.invalidateArray(traln, prunedTree.getPrimNode());
	}
      else 
	{
	  move.invalidateArrays(eval, traln, _moveOptMode); 
	}

      if(branches.size() == 0 )
	{
	  eval.evaluate(traln, prunedTree, false);
	  auto lnl = traln.getLikelihood(); 
	  result.emplace_back(move.getInsertBranch(), lnl);
	  // tout <<  SHOW(lnl) << std::endl; 
	}
      else 
	{
	  auto backup = BranchBackup(traln, branches, params);
	  auto bsp = BranchSetProposer(traln, branches, params);
	  bsp.findJointOptimum(eval, MAX_ITER, true);

	  auto mappedOptParams = bsp.getResult(); 
	  auto optParams = std::vector<OptimizedParameter>{}; 
	  for(auto elem : mappedOptParams)
	    optParams.insert(end(optParams), begin(elem.second), end(elem.second)); 

	  result.emplace_back(move.getInsertBranch(), bsp.getOptimalLikelihood(), optParams );
	  backup.resetFromBackup(traln); 
	}

      auto inv = move.getInverse(); 
      inv->apply(traln,params); 

      if(not (_moveOptMode == MoveOptMode::NONE || _moveOptMode == MoveOptMode::ONLY_SWITCHING))
	inv->invalidateArrays(eval, traln, _moveOptMode);       

      prevMove = move; 
    }

  prevMove.invalidateArrays(eval,traln, _moveOptMode); 

  return result; 
}



std::vector<InsertionResult>  
LikehoodSprProposer::transformLikelihoods(std::vector< InsertionResult > result ) const 
{
  auto maxElem = std::max_element(begin(result), end(result), [](const InsertionResult& a, const InsertionResult &b){ return  a.getInsertionProb() < b.getInsertionProb()  ;   });
  auto maximum = maxElem->getInsertionProb();
  // tout << SHOW(maximum) << std::endl; 

  auto sum = 0.;
  for(auto &v : result)
    {
      auto val = v.getInsertionProb();
      val /= maximum; 
      val = exponentiate(val, _likeWarp);
      auto abs = val.toAbs() + weightEps; 
      v.setInsertionProb(log_double::fromAbs(abs));
      sum += abs; 
    }

  auto lsum = log_double::fromAbs(sum);
  for(auto &v : result)
    v.setInsertionProb(v.getInsertionProb()  / lsum); 

#ifdef PRINT_LIKESPR_INFO
  tout << "================================================================" << std::endl; 
  tout << "likelihoods: " << std::endl; 
  for(auto &pair : result)
    tout << "DIFF " << pair.getBranch() << "\t" <<  MAX_SCI_PRECISION << pair.getInsertionProb()  << "\t" << SOME_FIXED_PRECISION << pair.getInsertionProb() << std::endl; 
#endif
  
  return result; 
}


