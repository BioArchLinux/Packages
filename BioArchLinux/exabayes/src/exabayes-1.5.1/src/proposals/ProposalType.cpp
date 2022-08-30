#include <iostream>		// 
#include <cassert>

#include "ProposalType.hpp"


namespace ProposalTypeFunc
{
  std::string getLongName(ProposalType type)
  {
    std::unordered_map<ProposalType, std::string, ProposalTypeHash> map = 
      {
	{ ProposalType::ST_NNI , "statistical NNI"  } , 
	{ ProposalType::E_SPR , "extended SPR"}, 
	{ ProposalType::E_TBR , "extended TBR"},
	{ ProposalType::PARSIMONY_SPR, "parsimony-guided SPR"},
	{ ProposalType::TL_MULT , "tree length multiplier"},
	{ ProposalType::NODE_SLIDER , "node slider"},
	{ ProposalType::BRANCH_LENGTHS_MULTIPLIER , "branch length multiplier"},
	{ ProposalType::REVMAT_SLIDER , "substition matrix slider"},
	{ ProposalType::REVMAT_DIRICHLET , "substitution matrix dirchlet"},
	{ ProposalType::RATE_HET_SLIDER , "rate heterogeneity slider"},
	{ ProposalType::RATE_HET_MULTI , "rate heterogeneity multiplier"},
	{ ProposalType::FREQUENCY_SLIDER , "frequency slider"},
	{ ProposalType::FREQUENCY_DIRICHLET , "frequency dirichlet"},
	{ ProposalType::AMINO_MODEL_JUMP , "amino acid model jump"},
	{ ProposalType::BRANCH_DIST_GAMMA , "branch length from opt-dist"} ,
	{ ProposalType::LIKE_SPR, "likelihood-guided SPR move"}, 
	{ ProposalType::DIRICH_REVMAT_PER_RATE, "rate orientated dirichlet proposal on RevMat" } ,
	{ ProposalType::SLIDING_REVMAT_PER_RATE, "rate orientated sliding proposal on RevMat"} , 
	{ ProposalType::BL_DIST_WEIBULL , "a weibull based branch length proposal" } ,
	{ ProposalType::DIV_TIME_DIRICH , "dirichlet proposal on divergence times" } ,
      };
    
    if(map.find(type) == map.end())
      {
	std::cerr << "you requested a proposal type for which no long name has been set. Correct that in ProposalType.hpp  " << std::endl; 
	exitFunction(-1, true); 
      }
    return map[type]; 
  }

  std::string getConfigStringFromType(ProposalType p )
  {
    // notice: MUST be upper case!
    std::unordered_map<ProposalType, std::string, ProposalTypeHash> proposal2name = 
      {
	{ ProposalType::ST_NNI,  "STNNI" } ,
	{ ProposalType::E_SPR,  "ESPR" } ,
	{ ProposalType::E_TBR,  "ETBR" } ,
	{ ProposalType::PARSIMONY_SPR,  "PARSIMONYSPR" } ,
	{ ProposalType::TL_MULT,  "TREELENGTHMULT" } ,
	{ ProposalType::BRANCH_LENGTHS_MULTIPLIER,  "BRANCHMULTI" } ,
	{ ProposalType::NODE_SLIDER,  "NODESLIDER" } ,
	{ ProposalType::REVMAT_SLIDER,  "REVMATSLIDER" }  , 
	{ ProposalType::REVMAT_DIRICHLET,  "REVMATDIRICHLET" } ,
	{ ProposalType::RATE_HET_SLIDER,  "RATEHETSLIDER" } ,
	{ ProposalType::RATE_HET_MULTI,  "RATEHETMULTI" } ,
	{ ProposalType::FREQUENCY_SLIDER,  "FREQUENCYSLIDER" } ,
	{ ProposalType::FREQUENCY_DIRICHLET,  "FREQUENCYDIRICHLET" } ,
	{ ProposalType::AMINO_MODEL_JUMP,  "AAMODELJUMP" } ,
	{ ProposalType::BRANCH_DIST_GAMMA , "BLDISTGAMMA"} , 
	{ ProposalType::LIKE_SPR, "LIKESPR"},
	{ ProposalType::DIRICH_REVMAT_PER_RATE, "REVMATRATEDIRICH" } ,
	{ ProposalType::SLIDING_REVMAT_PER_RATE, "REVMATRATESLIDER"} ,
	{ ProposalType::BL_DIST_WEIBULL , "BLDISTWEIBULL" } ,
	{ ProposalType::DIV_TIME_DIRICH , "DIVTIMEDIRICH" },
      };

    return proposal2name[p];     
  }

  ProposalType getTypeFromConfigString(std::string name )
  {
    auto ps = getAllProposals();
    for(auto p : ps)
      {
	if(getConfigStringFromType(p).compare(name) == 0)
	  return p; 
      }
    
    std::cerr << "something went wrong while trying to identify what kind of proposal >"  << name  << "< is. See ProposalType.hpp" << std::endl; 
    exitFunction(-1, true); 
    return ProposalType::ST_NNI; 
  }


  std::vector<ProposalType> getSingleParameterProposalsForCategory(Category c) 
  {
    switch(c)
      {
      case Category::DIVERGENCE_TIMES: 
	return
	  {
	    ProposalType::DIVTIME_SLIDER 
	  }; 
      case Category::DIVERGENCE_RATES: 
	return 
	  {
	    ProposalType::DIVRATE_SLIDER
	  }; 
      case Category::TOPOLOGY: 
	return { 
	  ProposalType::ST_NNI, 
	    ProposalType::E_SPR, 
	    ProposalType::E_TBR, 
	    ProposalType::PARSIMONY_SPR, 
	    ProposalType::LIKE_SPR
	    }; 
      case Category::BRANCH_LENGTHS: 
	return { 
	  ProposalType::BRANCH_LENGTHS_MULTIPLIER, 
	    ProposalType::TL_MULT, 
	    ProposalType::BRANCH_DIST_GAMMA , 
	    ProposalType::NODE_SLIDER, 
	    ProposalType::BL_DIST_WEIBULL, 
	    ProposalType::DIV_TIME_DIRICH, 
            };
      case Category::FREQUENCIES: 
	return { 
	  ProposalType::FREQUENCY_SLIDER, 
	    ProposalType::FREQUENCY_DIRICHLET
	    } ; 
      case Category::SUBSTITUTION_RATES: 
	return { 
	  ProposalType::REVMAT_SLIDER, 
	    ProposalType::REVMAT_DIRICHLET, 
	    ProposalType::DIRICH_REVMAT_PER_RATE,
	    ProposalType::SLIDING_REVMAT_PER_RATE
	    }; 	
      case Category::RATE_HETEROGENEITY: 
	return { 
	  ProposalType::RATE_HET_MULTI, 
	    ProposalType::RATE_HET_SLIDER
	    }; 
      case Category::AA_MODEL: 
	return { 
	  ProposalType::AMINO_MODEL_JUMP
	    }; 
      default: 
	{
	  std::cerr << "no proposals for caterogy " << int(c) << std::endl; 	  
	  assert(0); 
	}
      }
  }


  std::vector<ProposalType> getAllProposals()
  {
    auto result = std::vector<ProposalType>{}; 
    auto cs =   CategoryFuns::getAllCategories() ; 
    for(auto c : cs)
      {
	auto someProposals =  getSingleParameterProposalsForCategory( c) ; 
	result.insert(result.end(), someProposals.begin(), someProposals.end()); 
      } 

    return result; 
  }

  bool isValidName(std::string name)
  {
    auto ps = getAllProposals();     
    for(auto &p : ps)
      {
	if(getConfigStringFromType(p).compare(name) == 0)
	  return true; 
      }
    return false; 
  } 


  bool isDefaultInstantiate(ProposalType p)
  {
    std::unordered_map<ProposalType, bool, ProposalTypeHash> map  = 
    {
	{ ProposalType::ST_NNI,  true } ,
	{ ProposalType::E_SPR,  true } ,
	{ ProposalType::E_TBR,  false } , // dont like this one: mostly works, when degenerated to spr 
	{ ProposalType::LIKE_SPR , true}, 
	{ ProposalType::PARSIMONY_SPR,  true } ,

	{ ProposalType::REVMAT_SLIDER,  true }  , 
	{ ProposalType::REVMAT_DIRICHLET,  true } ,
	{ ProposalType::DIRICH_REVMAT_PER_RATE, true } ,
	{ ProposalType::SLIDING_REVMAT_PER_RATE, false} , 

	{ ProposalType::RATE_HET_SLIDER,  false } ,
	{ ProposalType::RATE_HET_MULTI,  true } ,

	{ ProposalType::FREQUENCY_SLIDER,  true } ,
	{ ProposalType::FREQUENCY_DIRICHLET,  true } ,

	{ ProposalType::AMINO_MODEL_JUMP,  true } ,

	{ ProposalType::TL_MULT,  true } ,
        { ProposalType::BRANCH_DIST_GAMMA , true } ,
        { ProposalType::BRANCH_LENGTHS_MULTIPLIER,  true } ,
	{ ProposalType::NODE_SLIDER,  false } ,
	{ ProposalType::BL_DIST_WEIBULL, false }, 
	{ ProposalType::DIV_TIME_DIRICH, false }, 
	{ ProposalType::DIVTIME_SLIDER , true } , 
	{ ProposalType::DIVRATE_SLIDER , true }
    };

    if(map.find(p) == map.end())
      {
	std::cerr << "Error, could not find type >" << int(p) << 
	  "< when trying to determine, if proposal is ready for productive use. Check ProposalType.cpp" << std::endl; 
	assert(0);
	exitFunction(-1, true); 
      }

    return map[p]; 
  }
}
