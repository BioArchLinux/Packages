#ifndef _PROPOSAL_TYPE 
#define _PROPOSAL_TYPE 

#include <unordered_map>
#include <vector>

#include "Category.hpp" 


// add your new proposal here and update accordingly in everything in
// ProposalType.cpp .  if you missed something, then exabayes usually
// complains with an > assert(0);

enum class ProposalType
{
  ST_NNI, 
    E_SPR, 
    E_TBR,
    PARSIMONY_SPR,
    TL_MULT,
    NODE_SLIDER,
    BRANCH_LENGTHS_MULTIPLIER,
    REVMAT_SLIDER,
    REVMAT_DIRICHLET,
    RATE_HET_SLIDER,
    RATE_HET_MULTI,
    FREQUENCY_SLIDER,
    FREQUENCY_DIRICHLET,
    AMINO_MODEL_JUMP,
    BRANCH_DIST_GAMMA,
    LIKE_SPR,
    DIRICH_REVMAT_PER_RATE,
    SLIDING_REVMAT_PER_RATE,
    BL_DIST_WEIBULL,
    DIV_TIME_DIRICH, 
    DIVRATE_SLIDER,
    DIVTIME_SLIDER
}; 


class ProposalTypeHash
{
public: 
  size_t operator()(const ProposalType& t) const 
  {
    std::hash<int> hasher; 
    return hasher(int(t)) ; 
  }
}; 


namespace ProposalTypeFunc
{
  /** 
      @brief gets the verbose name of the proposal 
   */ 
  std::string getLongName(ProposalType type); 
  /** 
      @brief gets the proposal type from the name in the config file   
   */ 
  ProposalType getTypeFromConfigString(std::string s); 
  /** 
      @brief gets the name of the proposal by type 
   */ 
  std::string getConfigStringFromType(ProposalType p ); 
  /** 
      @brief gets all proposals for a category that integrate only over one parameter (primarily) 
   */ 
  std::vector<ProposalType> getSingleParameterProposalsForCategory(Category c) ; 
  /** 
      @brief IMPORTANT get all relevant proposals for a category  
   */ 
  // std::vector<ProposalType> getProposalsForCategory(Category c) ; 
  /** 
      @brief gets all relevant proposals 
   */ 
  std::vector<ProposalType> getAllProposals();   
  /**
     @brief indicates, if the string specifies a valid proposal name 
   */ 
  bool isValidName(std::string name); 
  /** 
      @brief indicates, if the proposal is ready for productive use
      (if this is not the case, then the proposal can only be
      activated by explicitly specifying it in the config file)
   */ 
  bool isDefaultInstantiate(ProposalType p); 

}

#endif
