#ifndef DIVTIMESLIDER_H
#define DIVTIMESLIDER_H


#include "AbstractProposal.hpp"

class DivTimeSlider : public AbstractProposal
{ 
public:                         // STATIC 
  const static double defaultWeight;  

public:				// INHERITED 

  virtual void applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) ; 
  virtual void evaluateProposal(LikelihoodEvaluator &evaluator, TreeAln &traln, const BranchPlain &branchSuggestion) ; 
  virtual void resetState(TreeAln &traln)  ; 
  virtual void autotune()   ;
  virtual AbstractProposal* clone() const ;  
  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const ; 
  virtual std::vector<nat> getInvalidatedNodes(const TreeAln &traln) const ;  
  virtual std::pair<BranchPlain,BranchPlain> prepareForSetExecution(TreeAln &traln, Randomness &rand)  ; 
  virtual void writeToCheckpointCore(std::ostream &out)const   ;  
  virtual void readFromCheckpointCore(std::istream &in) ; 

  DivTimeSlider(double weight);
  virtual ~DivTimeSlider(){}

protected:
  double getNewProposal(double height, double oldHeight, double youngHeight, Randomness &rand);

  NodeAge getNodeAge(TreeAln & traln, BranchPlain & bp) const;
  NodeAge getNodeAge(TreeAln & traln, nat primaryNode) const;
  ParameterContent _savedContent;
//  NodeAge _savedAge;
//  double _savedTime;
//  double _savedBranchLength;
//  std::pair<double, double> _savedChildrenBranchLength;
};




#endif /* DIVTIMESLIDER_H */
