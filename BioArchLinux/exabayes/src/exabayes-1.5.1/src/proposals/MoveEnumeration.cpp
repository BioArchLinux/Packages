#include "MoveEnumeration.hpp"

#include <cassert>

#include "SprMove.hpp"



std::vector<SprMove> MoveEnumeration::getMovesFromDepthFirstSearch_helper(const TreeAln& traln, const BranchPlain & pruneBranch, const BranchPlain& insert, int depth, bool isFirst ) 
{
  auto result = std::vector<SprMove>{}; 
  
  if(depth < 0 )
    return result; 

  if(not isFirst )
    result.emplace_back(traln, pruneBranch, insert);

  if(not traln.isTipNode(insert.getSecNode())) 
    {
      auto desc = traln.getDescendents(insert.getInverted()); 

      auto res1 = getMovesFromDepthFirstSearch_helper(traln, pruneBranch, desc.first, depth - 1, false);
      auto res2 = getMovesFromDepthFirstSearch_helper(traln, pruneBranch, desc.second, depth - 1, false);
      
      result.insert(end(result), begin(res1), end(res1)); 
      result.insert(end(result), begin(res2), end(res2)); 
    }

  return result; 
}

std::vector<SprMove> MoveEnumeration::getMovesFromDepthFirstSearch(const TreeAln& traln, const BranchPlain & pruneBranch, int dist)
{
  auto result = std::vector<SprMove>{}; 

  assert(not traln.isTipNode(pruneBranch.getPrimNode())); 

  auto desc = traln.getDescendents(pruneBranch);

  auto res1 = getMovesFromDepthFirstSearch_helper(traln, pruneBranch, desc.first, dist, true ) ; 
  auto res2 = getMovesFromDepthFirstSearch_helper(traln, pruneBranch, desc.second, dist, true ) ; 

  result.insert(end(result), begin(res1), end(res1));
  result.insert(end(result), begin(res2), end(res2)); 
  
  return result; 
}



std::vector<SprMove> MoveEnumeration::getMovesForBranch(const TreeAln& traln, BranchPlain pruneBranch, nat dist) 
{
  auto result = std::vector<SprMove>{};
  
  auto pP = pruneBranch.getPrimNode();  

  if(not traln.isTipNode( pruneBranch.getPrimNode()))
    {
      auto distBranches = traln.getBranchesByDistance(pruneBranch, dist + 1, false ); 

      for(auto insertBranch : distBranches)
	{
	  auto iP = insertBranch.getPrimNode(),
	    iS = insertBranch.getSecNode(); 

	  if(dist == 1 
	     &&   
	     ( ( traln.exists(BranchPlain(pP, iP )) && pP > iP  ) 
	       || ( traln.exists(BranchPlain(pP, iS )) && pP > iS  )) )
	    {
	      // tout << "skipping " << pruneBranch << "," << insertBranch << std::endl; 
	      continue; 
	    }

	  result.emplace_back( traln, pruneBranch, insertBranch);
	}
    }

  return result; 
}



std::vector<SprMove> MoveEnumeration::getAllUniqueMoves(const TreeAln& traln, nat dist)
{
  auto result = std::vector<SprMove>{}; 
  
  assert(dist > 0); 

  for(auto pruneBranch : traln.extractBranches())  
    {
      auto res1 = getMovesForBranch(traln, pruneBranch, dist+1); 
      auto res2 = getMovesForBranch(traln, pruneBranch.getInverted(), dist+1); 

      result.insert(end(result), begin(res1),end(res1)); 
      result.insert(end(result), begin(res2),end(res2)); 
    }
  
  return result; 
}




