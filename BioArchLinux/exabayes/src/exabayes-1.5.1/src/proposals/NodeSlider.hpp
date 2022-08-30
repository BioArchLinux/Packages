#ifndef _NODESLIDER_H
#define _NODESLIDER_H

#include "AbstractProposal.hpp" 
#include "Path.hpp"


class NodeSlider : public AbstractProposal
{
public:   
  NodeSlider(  double multiplier);
  virtual ~NodeSlider(){}

  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const; 

  virtual void applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) ; 
  virtual void evaluateProposal(  LikelihoodEvaluator &evaluator, TreeAln &traln, const BranchPlain &branchSuggestion) ; 
  virtual void resetState(TreeAln &traln) ; 
  
  BranchPlain proposeBranch(const TreeAln &traln, Randomness &rand) const ; 
  BranchPlain proposeOtherBranch(const BranchPlain &firstBranch, const TreeAln& traln, Randomness& rand) const ; 

  virtual void prepareForSetEvaluation( TreeAln &traln, LikelihoodEvaluator& eval) const ; 
  std::pair<BranchPlain,BranchPlain> prepareForSetExecution(TreeAln& traln, Randomness &rand) ; 
  virtual void autotune() {}	// disabled 

  virtual AbstractProposal* clone() const;  

  virtual std::vector<nat> getInvalidatedNodes(const TreeAln &traln ) const; 

  virtual void readFromCheckpointCore(std::istream &in) {   } // disabled
  virtual void writeToCheckpointCore(std::ostream &out) const { } //disabled

protected: 
  double multiplier; 
  BranchLength oneBranch; 
  BranchLength otherBranch; 
};  

#endif
