#ifndef _STAT_NNI_HPP_NEW
#define _STAT_NNI_HPP_NEW

#include "SprMove.hpp"

#include "TopoMoveProposer.hpp"

class StatNniProposer : public TopoMoveProposer
{
public: 
  StatNniProposer() 
    : _move{}
  {}
  
  virtual void determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params)  ; 
  virtual TopoMoveProposer* clone() const ; 
  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const ; 
  virtual void determineBackProb(TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params) ; 
  std::unique_ptr<TopoMove> getMove() const   ; 

private: 
  SprMove _move; 
}; 

#endif
