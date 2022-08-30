#ifndef _GENERIC_TOPO_PROPOSAL_HPP
#define _GENERIC_TOPO_PROPOSAL_HPP

#include "AbstractProposal.hpp"
#include "TopoMoveProposer.hpp"
#include "BranchBackup.hpp"


class GenericTopoProposal : public AbstractProposal
{
public: 			// INHERITED METHODS 
  virtual void applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) ; 
  virtual void evaluateProposal(  LikelihoodEvaluator &evaluator, TreeAln &traln, const BranchPlain &branchSuggestion) ; 
  virtual void resetState(TreeAln &traln) ; 
  virtual std::pair<BranchPlain,BranchPlain> prepareForSetExecution(TreeAln &traln, Randomness &rand)  { return std::make_pair(BranchPlain(0,0),BranchPlain(0,0) ); }
  virtual void autotune() {}	// disabled 
  virtual AbstractProposal* clone() const;  
  virtual void readFromCheckpointCore(std::istream &in) {   } // disabled
  virtual void writeToCheckpointCore(std::ostream &out) const { } //disabled
  
  virtual void printParams(std::ostream &out)  const { _moveProposer->printParams(out); } 
  
public: 			// METHODS 
  GenericTopoProposal( std::unique_ptr<TopoMoveProposer> moveDet , std::string name , double relWeight, MoveOptMode toOpt); 
  GenericTopoProposal(const GenericTopoProposal& rhs); 
  GenericTopoProposal(GenericTopoProposal&& rhs) = delete; 
  GenericTopoProposal& operator=( GenericTopoProposal rhs) ; 
  friend void swap(GenericTopoProposal& lhs, GenericTopoProposal& rhs); 

  void enableUseMultiplier() { _useMultiplier = true; } 
 
  log_double assessOldBranches(TreeAln& traln, LikelihoodEvaluator& eval, const std::vector<BranchPlain> &bs, const std::vector<AbstractParameter*> params) ; 
  BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const; 
  std::vector<nat> getInvalidatedNodes(const TreeAln& traln) const;  
  
private: 			// METHODS 
  log_double proposeAndApplyNewBranches(TreeAln &traln, const std::vector<BranchPlain> &bs, PriorBelief &prior,  Randomness &rand, LikelihoodEvaluator& eval) const ; 

private:			// ATTRIBUTES
  std::unique_ptr<TopoMove> _move; // abstract product  
  std::unique_ptr<TopoMoveProposer> _moveProposer; // abstract factory 
  MoveOptMode _moveOptMode;	
  BranchBackup _backup; 
  bool _useMultiplier ; 

  static double multiplier; 

};

#endif
