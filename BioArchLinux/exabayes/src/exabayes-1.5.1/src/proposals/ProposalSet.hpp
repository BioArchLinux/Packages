/** 
    @file ProposalSet.hpp
    
    @brief represents a set of proposals 

    This class should be used to execute component-wise
    Metropolis-Hastings (a.k.a. component-in-sequence MH).

    The main motivation to do so is efficiency.

    The justification to do is e.g., 
    http://theclevermachine.wordpress.com/2012/11/04/mcmc-multivariate-distributions-block-wise-component-wise-updates/  
    
    and more specifically 
    The Metropolitan-Hastings Algorithm and Extensions.S. Sawyer — Washington University — Vs. August 22, 2010
 */ 


#ifndef PROPOSAL_SET
#define PROPOSAL_SET

#include <iostream>
#include "AbstractProposal.hpp"

class ProposalSet : public Serializable
{
public: 
  // life cycle 
  ProposalSet(double relWeight, std::vector<std::unique_ptr<AbstractProposal>> proposals); 
  ProposalSet(const ProposalSet &rhs);
  ProposalSet& operator=(const ProposalSet &rhs); 
  /** 
      @brief prints the proposal set    
   */ 
  void printVerboseAbbreviated(std::ostream &out, double sum) const; 
  /** 
      @brief indicates whether for this proposal set a full tree
      traversal is necessary
   */ 
  bool needsFullTraversal(); 
  
  nat numerateProposals(nat ctr); 

  void setParameterListPtr(ParameterList* pPtr);  

  double getRelativeWeight() const {return relativeWeight; }
  std::vector<AbstractProposal*> getProposalView() const;  

  virtual void deserialize( std::istream &in ); 
  virtual void serialize( std::ostream &out) const;   

  friend std::ostream& operator<<(std::ostream& out, const ProposalSet &rhs); 

private: 
  double relativeWeight; 
  std::vector<std::unique_ptr<AbstractProposal> > proposals;   
}; 


#endif
