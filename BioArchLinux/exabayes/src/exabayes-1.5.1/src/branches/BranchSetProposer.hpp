#ifndef BRANCH_SET_PROPOSER_HPP
#define BRANCH_SET_PROPOSER_HPP

#include <unordered_map>

#include "OptimizedParameter.hpp" 

class LikelihoodEvaluator; 


class BranchSetProposer
{
public: 
  ~BranchSetProposer(){}
  BranchSetProposer(TreeAln &traln, std::vector<BranchPlain> branches, std::vector<AbstractParameter*> params); 
  void findJointOptimum(LikelihoodEvaluator& eval, int maxIter, bool computeLikelihood); 
  std::unordered_map<BranchPlain, std::vector<OptimizedParameter> > getResult() const {return _result; }
  log_double getOptimalLikelihood() const {return _likelihood; }

private: 
  void reorderToConnectedComponent(); 

private: 

  std::reference_wrapper<TreeAln> _traln; 
  std::vector<BranchPlain> _branches; 
  const std::vector<AbstractParameter*> _params; 
  std::unordered_map<BranchPlain, std::vector<OptimizedParameter> > _result; 
  log_double _likelihood; 
}; 

#endif
