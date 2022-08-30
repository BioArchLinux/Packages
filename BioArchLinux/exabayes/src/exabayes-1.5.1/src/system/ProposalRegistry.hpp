#ifndef _PROPOSAL_REGISTRY_H
#define _PROPOSAL_REGISTRY_H

#include <vector>
#include <memory>

#include "GlobalVariables.hpp"
#include "BlockProposalConfig.hpp"
#include "ParameterProposal.hpp"
#include "ParameterList.hpp"

class ProposalRegistry
{
public: 
  /** 
      @brief get all proposals that integrate over a single parameter   
  */ 
  std::vector<std::unique_ptr<AbstractProposal> >
  getSingleParameterProposals(Category cat, const BlockProposalConfig &config, const TreeAln &traln, ParallelSetup& pl, ParameterList &params) const ; 

  static const double initFrequencySlidingWindow ; 
  static const double initBranchLengthMultiplier; 
  static const double initRateSlidingWindow; 
  static const double initGammaSlidingWindow; 
  static const double initSecondaryBranchLengthMultiplier; 
  static const double initDirichletAlpha; 
  static const double initTreeLengthMultiplier ; 
  static const double initGammaMultiplier; 
  static const double initNodeSliderMultiplier; 
  static const int likeSprMinRadius; 
  static const int likeSprMaxRadius; 
  static const double likeSprWarp; 
  static const double initDirichletProtAlpha; 

private: 

}; 


#endif
