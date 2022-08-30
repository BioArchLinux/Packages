/** 
    @brief a helper object making the code concerning a SPR move
    re-usable.
 */ 


#ifndef _SPR_MOVE_H
#define _SPR_MOVE_H

#include "Path.hpp"

#include "TreeTraverser.hpp"
#include <unordered_set> 

#include "TopoMove.hpp"

class PriorBelief; 
class LikelihoodEvaluator; 

class SprMove : public TopoMove
{
public : 			// INHERITED 
  virtual void apply(TreeAln &traln, const std::vector<AbstractParameter*> &blParams) const; 
  virtual BranchPlain getEvalBranch(const TreeAln &traln) const; 
  virtual std::unique_ptr<TopoMove> getInverse() const ; 
  virtual TopoMove* clone() const ; 
  virtual void print(std::ostream& out) const ; 
  virtual std::vector<BranchPlain> getBranchesToPropose(const TreeAln& traln, MoveOptMode mode ); 
  virtual int getNniDistance() const  { return std::max(int(_path.size() ) - 2,0) ; }
  virtual std::vector<nat> getDirtyNodes() const ; 

  virtual void invalidateArrays(LikelihoodEvaluator& eval,  TreeAln& traln, MoveOptMode mode)  const ; 

public: 
  explicit SprMove()
    : _path{}
  {}
  virtual ~SprMove(){}
  SprMove(const TreeAln &traln, const BranchPlain &prunedTree,  const BranchPlain &insertBranch);

  std::vector<nat> getDirtyWrtPrevious(const SprMove &rhs) const ; 

  BranchPlain getMovingSubtree(const TreeAln &traln) const ; 
  
  std::vector<nat> getNodeList() const ; 

  SprMove getInverseMove() const ; 
  BranchPlain getInsertBranch() const { return _path.at(int(_path.size())-1); }

  std::unordered_set<BranchPlain> getBranchesByDistance(const TreeAln &traln, nat dist) const ; 

  friend std::ostream& operator<<(std::ostream &out, const SprMove& rhs); 

private: 			// ATTRIBUTES
  Path _path; 
}; 

#endif
