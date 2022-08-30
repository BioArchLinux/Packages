#ifndef DIVRATESLIDER_H
#define DIVRATESLIDER_H


#include "AbstractProposal.hpp"

  // AbstractProposal( Category cat, std::string  _name, double weight, double minTuning, double maxTuning, bool needsFullTraversal )  ; 


class DivRateSlider : public AbstractProposal
{

public: 
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

  DivRateSlider();
  virtual ~DivRateSlider();

protected:

  enum ProposalMode {
		  PROP_MODE_CATEGORIES,
		  PROP_MODE_RATES
  };

  enum AssignmentsOperation {
		  ASSING_OP_MERGE,  /* merge 2 categories */
		  ASSING_OP_SPLIT,  /* split 1 category in 2 or several */
		  ASSING_OP_MOVE    /* move an item from one category to another */
  };

  double getNewProposal(double oldRate, Randomness &rand);

  ParameterContent _savedContent;
  
};




#endif /* DIVRATESLIDER_H */
