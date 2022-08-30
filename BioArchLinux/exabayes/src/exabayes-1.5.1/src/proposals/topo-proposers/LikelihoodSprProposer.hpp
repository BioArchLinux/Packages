#ifndef LIKELIHOODSPRPROPOSER_H
#define LIKELIHOODSPRPROPOSER_H

#include "TopoMoveProposer.hpp"
#include "SprMove.hpp"
#include "BranchBackup.hpp"


class LikehoodSprProposer  : public  TopoMoveProposer
{
public: 			// INHERITED 
  virtual void determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params)  ; 
  virtual void determineBackProb(TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params) ; 
  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const ; 
  virtual TopoMoveProposer* clone() const ; 
  virtual std::unique_ptr<TopoMove> getMove() const  ; 

  virtual void printParams(std::ostream &out)  const { out << ";radius=" << _maxStep << ",warp=" << _likeWarp ;} 

public:
  LikehoodSprProposer(nat maxStep, double likeWarp, MoveOptMode toOpt); 
  virtual ~LikehoodSprProposer(){}

private:			// METHODS 
  std::vector<InsertionResult> computeLikelihoodsOfInsertions(TreeAln &traln, LikelihoodEvaluator &eval, const BranchPlain& prunedTree, const std::vector<AbstractParameter*> &params)  ; 
  std::vector<InsertionResult> transformLikelihoods(std::vector< InsertionResult > result ) const ; 


private: 			// ATTRIBUTES 
  nat _maxStep; 
  double _likeWarp; 
  SprMove _move; 
  MoveOptMode _moveOptMode; 
  
  static double weightEps; 
};


#endif /* LIKELIHOODSPRPROPOSER_H */
