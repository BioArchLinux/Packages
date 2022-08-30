#ifndef _TBR_MOVE_H 
#define _TBR_MOVE_H 

#include "SprMove.hpp"

#include "TopoMove.hpp"

class TbrMove : public TopoMove
{
public: 			// INHERITED methods  
  virtual void apply(TreeAln &traln, const std::vector<AbstractParameter*> &blParams) const ; 
  virtual ~TbrMove(){}
  virtual BranchPlain getEvalBranch(const TreeAln &traln) const ; 
  virtual std::vector<nat> getDirtyNodes() const  ; 
  virtual std::unique_ptr<TopoMove> getInverse() const  ; 
  virtual TopoMove* clone() const  ; 
  virtual void print(std::ostream& out) const ; 

  virtual std::vector<BranchPlain> getBranchesToPropose(const TreeAln& traln, MoveOptMode mode ) ; 
  
  virtual int getNniDistance() const { return _moveA.getNniDistance() + _moveB.getNniDistance()  ; }
  virtual void invalidateArrays(LikelihoodEvaluator& eval, TreeAln& traln, MoveOptMode mode) const ; 

public: 			// methods 
  explicit TbrMove()  
    : _moveA{}
    , _moveB{}
  {}
  TbrMove(const TreeAln &traln, const BranchPlain &bisected, const BranchPlain& insertA, const BranchPlain& insertB)  ; 

  TbrMove getInverseMove() const ; 

  friend std::ostream& operator<<(std::ostream &out, const TbrMove &rhs) ;  

private: 			// ATTRIBUTES
  SprMove _moveA; 
  SprMove _moveB; 
}; 

#endif
