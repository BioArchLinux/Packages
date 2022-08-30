#include "Category.hpp"
#include <cassert>
#include <memory>
#include <algorithm>

#include "ProtModelParameter.hpp"
#include "AbstractParameter.hpp"
#include "BranchLengthsParameter.hpp"
#include "FrequencyParameter.hpp" 
#include "ParameterContent.hpp"
#include "RateHetParameter.hpp"
#include "RevMatParameter.hpp"
#include "TopologyParameter.hpp"

#include "DivergenceTimes.hpp"
#include "DivergenceRates.hpp"


using namespace std;

namespace CategoryFuns
{
  
  std::string getShortName(Category cat)
  {
    switch(cat)
      {
      case Category::TOPOLOGY: 
	return "topo"; 
      case Category::BRANCH_LENGTHS: 
	return "brlens"; 
      case Category::FREQUENCIES: 
	return "statefreq"; 
      case Category::SUBSTITUTION_RATES : 
	return "revmat"; 
      case Category::RATE_HETEROGENEITY : 
	return "ratehet"; 
      case Category::AA_MODEL : 
	return "aamdel"; 
      case Category::DIVERGENCE_RATES : 
	return "divrates"; 
      case Category::DIVERGENCE_TIMES : 
	return "divtimes"; 
      default: 
	{
	  assert(0); 
	  return "divtimes"; 
	}
      }
  }


  std::string getComponentName(Category cat)
  {
    switch(cat)
      {
      case Category::DIVERGENCE_RATES: 
	return "dR"; 
      case Category::DIVERGENCE_TIMES: 
	return "time"; 
      case Category::TOPOLOGY:
	return "topo" ;
      case Category::BRANCH_LENGTHS:
	return "v" ;
      case Category::FREQUENCIES :
	return "pi" ;
      case Category::SUBSTITUTION_RATES:
	return "r";
      case Category::RATE_HETEROGENEITY :
	return "shape" ;
      case Category::AA_MODEL:
	return "aaModel" ;
      default: 
	assert(0); 
	return "NOTHING"; 
      }
  }
 
  std::string getPriorName(Category cat)
  {
    switch(cat)
      {
      case Category::DIVERGENCE_TIMES: 
	return "TIMEPR"; 
      case Category::DIVERGENCE_RATES:
	return "DIVRATEPR"; 
      case Category::TOPOLOGY: 
	return "TOPOPR"; 
      case Category::BRANCH_LENGTHS: 
	return "BRLENPR";
      case Category::FREQUENCIES: 
	return "STATEFREQPR";
      case Category::SUBSTITUTION_RATES: 
	return "REVMATPR";
      case Category::AA_MODEL:
	return "AAPR"; 
      case Category::RATE_HETEROGENEITY: 
	return "SHAPEPR"; 
      default: 
	assert(0); 
      }
  } 


  std::vector<Category> getAllCategories()
  {
    return {  
      Category::TOPOLOGY, 
	Category::DIVERGENCE_TIMES, 
	Category::DIVERGENCE_RATES, 
	Category::BRANCH_LENGTHS, 
	Category::FREQUENCIES, 
	Category::AA_MODEL,
	Category::SUBSTITUTION_RATES, 
	Category::RATE_HETEROGENEITY 
	} ; 
  }


  Category getCategoryByPriorName(std::string name)
  {
    // bad but this is a bit exhausting... 
    auto cats = getAllCategories();
    
    std::transform(begin(name), end(name), begin(name), ::toupper);

    for(auto c : cats)
      {
	if(getPriorName(c).compare(name) == 0)
	  return c; 
      }

    tout << "Error in config file: did not find >" << name << "<" << std::endl; 

    assert(0); 
    return cats[0]; 
  }


  Category getCategoryFromLinkLabel(std::string name)
  {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    auto cats = getAllCategories(); 

    auto result = Category::TOPOLOGY; 
    for(auto cat : cats)
      {
	if( getShortName(cat).compare(name) == 0 )
	  result = cat; 
      }
    return result; 
  }



  
  


  std::unique_ptr<AbstractParameter> getParameterFromCategory(Category cat, nat id, nat idOfMyKind, std::vector<nat> partitions, nat numTaxa)
  {
    switch(cat)
      {
      case Category::DIVERGENCE_RATES: 
	return make_unique<DivergenceRates>(id, idOfMyKind, partitions,numTaxa);
      case Category::DIVERGENCE_TIMES: 
	// dummy initialize, let's set the proper node-age object
	// later, as soon as we know the topology 
	return make_unique<DivergenceTimes>(id, idOfMyKind, partitions, NodeAge{BranchPlain(0,0),0.}); 
      case Category::TOPOLOGY :
	return  make_unique<TopologyParameter>( id, idOfMyKind,partitions );
      case Category::BRANCH_LENGTHS:
	return  make_unique<BranchLengthsParameter>( id, idOfMyKind, partitions);
      case Category::FREQUENCIES :
	return  make_unique<FrequencyParameter>( id, idOfMyKind,partitions);
      case Category::SUBSTITUTION_RATES :
	return  make_unique<RevMatParameter>( id, idOfMyKind,partitions);
      case Category::RATE_HETEROGENEITY:
	return  make_unique<RateHetParameter>( id, idOfMyKind,partitions);
      case Category::AA_MODEL :
	return make_unique<ProtModelParameter>( id, idOfMyKind,partitions);
      default : 
	{
	  assert(0); 
	  return make_unique<RateHetParameter>(id, idOfMyKind, partitions);
	}
      }    
  }  
  
  // should the catogry by default be linked into a continuous block     
  bool inUniqueByDefault(Category cat)
  {
    return
      cat == Category::TOPOLOGY
      || cat == Category::BRANCH_LENGTHS
      || cat == Category::DIVERGENCE_TIMES ; 
  }

  
  std::vector<Category> getConflictingCategories(Category cat) 
  {
    switch(cat)
      {
      case Category::TOPOLOGY:
      case Category::BRANCH_LENGTHS: 
	return {Category::DIVERGENCE_TIMES, Category::DIVERGENCE_RATES}; 
      case Category::DIVERGENCE_RATES:
      case Category::DIVERGENCE_TIMES: 
	return {Category::BRANCH_LENGTHS, Category::TOPOLOGY}; 
      case Category::SUBSTITUTION_RATES:
	return {Category::AA_MODEL}; 
      case Category::AA_MODEL:
	return {Category::SUBSTITUTION_RATES}; 
      case Category::FREQUENCIES:
      case Category::RATE_HETEROGENEITY: 
      default : 
	return {}; 
      }
  } 
}


