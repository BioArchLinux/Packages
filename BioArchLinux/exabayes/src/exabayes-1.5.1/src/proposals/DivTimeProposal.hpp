#ifndef _DIVTIME_PROPOSAL_H
#define _DIVTIME_PROPOSAL_H


#include "AbstractProposal.hpp"

class DivTimeProposal : public AbstractProposal
{
public: 
  DivTimeProposal(double weight ) ; 

  /**
     @brief determines the proposal, applies it to the tree / model, updates prior and hastings ratio
   */ 
  virtual void applyToState(TreeAln &traln, PriorBelief &prior, log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval) ; 
  /**
     @brief evaluates the proposal 
     @todo remove the prior, we should not need it here 
   */ 
  virtual void evaluateProposal(LikelihoodEvaluator &evaluator, TreeAln &traln, const BranchPlain &branchSuggestion) ; 
  /** 
      @brief resets the tree to its previous state; corrects the prior, if necessary (@todo is this the case?)
   */ 
  virtual void resetState(TreeAln &traln)  ; 
  /** 
      @brief tunes proposal parameters, if available  
   */ 
  virtual void autotune()   ;
  
  virtual AbstractProposal* clone() const ;  

  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const ; 
  /** 
      @brief gets nodes that are invalid by executed the proposal 
   */ 
  virtual std::vector<nat> getInvalidatedNodes(const TreeAln &traln) const ;  

  virtual std::pair<BranchPlain,BranchPlain> prepareForSetExecution(TreeAln &traln, Randomness &rand)  ; 

  // we need to implement these 
  virtual void serialize( std::ostream &out)  const{}  
  virtual void deserialize( std::istream &in ) {} 

  /** 
      @brief writes proposal specific (tuned) parameters
   */ 
  virtual void writeToCheckpointCore(std::ostream &out)const   ;  
  /** 
      @brief reads proposal specific (tuned) parameters 
   */ 
  virtual void readFromCheckpointCore(std::istream &in) ; 

  virtual void prepareForSetEvaluation( TreeAln &traln, LikelihoodEvaluator& eval) const  {} 


  virtual void printParams(std::ostream &out)  const {} 


public:                         // STATIC 
  static double defaultWeight; 
  
protected:   
  // probably needs something like 
  // std::set<TimeBranchLength>  _backup
  // for the reset

}; 

#endif

