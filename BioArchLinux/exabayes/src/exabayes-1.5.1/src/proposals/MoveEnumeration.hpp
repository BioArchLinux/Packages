#ifndef _MOVE_ENUMERATOR_HPP
#define _MOVE_ENUMERATOR_HPP

#include <vector> 

#include "common.h"


#include "TreeAln.hpp"

class SprMove; 

class MoveEnumeration
{
public: 
  static std::vector<SprMove> getAllUniqueMoves(const TreeAln& traln, nat dist) ; 
  static std::vector<SprMove> getMovesForBranch(const TreeAln& traln, BranchPlain pruneBranch, nat dist) ; 
  static std::vector<SprMove> getMovesFromDepthFirstSearch(const TreeAln& traln, const BranchPlain & pruneBranch, int dist); 

private: 
  static std::vector<SprMove> getMovesFromDepthFirstSearch_helper(const TreeAln& traln, const BranchPlain & pruneBranch, const BranchPlain& insert, int depth, bool isFirst ) ; 
}; 


#endif
