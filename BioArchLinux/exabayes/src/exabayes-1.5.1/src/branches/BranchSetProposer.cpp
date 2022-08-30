#include "BranchSetProposer.hpp"
#include "BranchLengthOptimizer.hpp"
#include "ParallelSetup.hpp"
#include "BoundsChecker.hpp"
#include "LikelihoodEvaluator.hpp"
#include "BranchBackup.hpp"

#include <map>

#define THRESH 1.e-2
#define B_MAX_IT 8 

/*
  branches : order is important! 
 */ 
BranchSetProposer::BranchSetProposer(TreeAln &traln, std::vector<BranchPlain> branches, std::vector<AbstractParameter*> params)
  : _traln{traln}
  , _branches{branches}
  , _params{params}
  , _result{}
  , _likelihood{log_double::fromAbs(1.)}
  {
    reorderToConnectedComponent();
  }


void BranchSetProposer::findJointOptimum(LikelihoodEvaluator& eval, int maxIter, bool computeLikelihood)
{
  assert(_branches.size() > 0 ); 
  
  // create backup 
  auto backup = BranchBackup(_traln, _branches, _params); 

  auto &comm = eval.getParallelSetup().getChainComm();

  auto conv = false; 
  auto ctr = 1; 
  auto prevBranch = BranchPlain(0,0);
  while(not conv)
    {
      auto branchesConverged = true ;

      for(auto b : _branches)
 	{
	  eval.evaluate(_traln, b, false);

	  auto blo = BranchLengthOptimizer(_traln, b, B_MAX_IT, comm, _params ); 
	  blo.optimizeBranches(_traln);

	  auto optParams = blo.getOptimizedParameters(); 
	  for(auto optParam : optParams)
	    {
	      auto optBranch = optParam.getOptimizedBranch(); 
	      auto p = optParam.getParam(); 
	      auto lenNow = optBranch.toMeanSubstitutions( p->getMeanSubstitutionRate());
	      auto lenOld = _traln.get().getBranch(b,p).toMeanSubstitutions( p->getMeanSubstitutionRate()); 
	      // tout << ctr << "\t" << b << "\t" << MAX_SCI_PRECISION << SHOW(lenNow) << SHOW(lenOld) << std::endl; 
	      
	      if(not BoundsChecker::checkBranch(optBranch))
		BoundsChecker::correctBranch(optBranch); 

	      _traln.get().setBranch(optBranch, p); 
	      
	      branchesConverged &=   std::fabs( log(lenNow) - log(lenOld) ) < THRESH; 
	    }

	  _result[b] = optParams;
	  
	  prevBranch = b; 
	}

      conv = _branches.size() == 1 ||  ( ctr == maxIter ) ||  branchesConverged; 
      ++ctr ; 
    }

  if(computeLikelihood)
    {
      eval.evaluate(_traln, _branches.back(), false);
      _likelihood = _traln.get().getLikelihood(); 
    }

  backup.resetFromBackup(_traln);
} 


using std::get; 

/** 
    @brief reorders the branches, s.t. they form a connected
    component.
 */ 
void BranchSetProposer::reorderToConnectedComponent()
{
  auto node2branch = std::multimap<nat, BranchPlain>{}; 
  for(auto b : _branches)
    {
      node2branch.insert(std::make_pair(b.getPrimNode(), b)); 
      node2branch.insert(std::make_pair(b.getSecNode(),  b)); 
    }
  
  auto isOuter = [&](const BranchPlain &b)
    {
      return node2branch.count(b.getPrimNode() )==  1
      || node2branch.count(b.getSecNode() )==  1; 
    }; 

  auto getNeighbors = [&](const BranchPlain&b ) ->std::vector<BranchPlain>
    {
      auto result = std::vector<BranchPlain>{}; 
      
      auto res1 = node2branch.equal_range(b.getPrimNode()); 
      auto res2 = node2branch.equal_range(b.getSecNode()); 
      
      for(auto iter = res1.first ; iter != res1.second; ++iter)
	{

   // ->second.equalsUndirected(b)
	  if(not (iter->second == b || b.getInverted() == iter->second) )
	    result.push_back( iter->second);
	}
      
      for(auto iter = res2.first; iter != res2.second; ++iter)
	{
	  // if(not (iter->second.equalsUndirected(b))
	  if(not (iter->second == b || iter->second == b.getInverted()))
	    result.push_back(iter->second); 
	}

      return result; 
    }; 

  auto cur = std::find_if(begin(_branches), end(_branches), 
			    [&](const BranchPlain &b)  -> bool 
			    {
			      if(isOuter(b))
				{
				  auto neighbors = getNeighbors(b); 
				  auto numNeighborOuter = std::count_if(begin(neighbors),end(neighbors), [&](const BranchPlain& bb){ return isOuter(bb) ;  }) ; 
				  assert(0 <= numNeighborOuter ); 
				  
				  return neighbors.size() <= 1 || numNeighborOuter == 1 ; 
				}
			      else 
				return false; 
			    } ) ; 


  assert(cur != end(_branches) || _branches.size() == 0 ); 

  auto neigh =std::unordered_set<BranchPlain>{  }; 
  if(_branches.size ()  != 0 )
    neigh.insert( *cur); 
  auto result = std::vector<BranchPlain>{}; 
  
  auto seen = std::unordered_set<BranchPlain>();
  while(result.size() < _branches.size() )
    {
      auto newNeigh = std::unordered_set<BranchPlain>{}; 
      for(auto n : neigh)
	{
	  if(seen.find(n) == end(seen))
	    {
	      result.push_back(n);
	      seen.insert(n); 
	      
	      auto moreNeigh = getNeighbors(n);
	      newNeigh.insert(begin(moreNeigh), end(moreNeigh)); 
	    }
	}
      neigh = newNeigh; 
    }
  
  _branches = result; 
}
