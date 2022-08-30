#include "AbstractProposal.hpp"


class TreeLengthMultiplier : public AbstractProposal
{
public: 
  TreeLengthMultiplier(  double _multiplier) ; 
  virtual ~TreeLengthMultiplier(){}

  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const{return BranchPlain(); }

  virtual void applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) ; 
  virtual void evaluateProposal(  LikelihoodEvaluator &evaluator, TreeAln &traln, const BranchPlain &branchSuggestion) ; 
  virtual void resetState(TreeAln &traln)  ; 
  virtual void autotune() ;

  virtual AbstractProposal* clone() const;  

  virtual void readFromCheckpointCore(std::istream &in);
  virtual void writeToCheckpointCore(std::ostream &out) const;

  virtual std::pair<BranchPlain,BranchPlain> prepareForSetExecution(TreeAln &traln, Randomness &rand)  { return std::make_pair (BranchPlain(0,0),BranchPlain(0,0) );}

  virtual std::vector<nat> getInvalidatedNodes(const TreeAln& traln) const; 

private: 			// METHODS
  void multiplyBranchLengthsRecursively(TreeAln& traln, nodeptr p, double multiHere); 

private: 			// ATTRIBUTES
  double multiplier; 		// the tuning variable  
  double initTreeLength; 
  std::vector<BranchLength> storedBranches; 
} ; 
