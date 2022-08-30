#include <set>
#include <iostream>

#include "extensions.hpp"
#include "RunFactory.hpp"
#include "ProposalRegistry.hpp"	

#include "AbstractProposer.hpp"
#include "MultiplierProposer.hpp"
#include "DirichletProposer.hpp" 
#include "SlidingProposer.hpp"

#include "TopologyParameter.hpp"
#include "BranchLengthsParameter.hpp"
#include "FrequencyParameter.hpp"
#include "RevMatParameter.hpp"
#include "RateHetParameter.hpp"

#include "DiscreteModelPrior.hpp"
#include "AbstractPrior.hpp"
#include "ExponentialPrior.hpp"
#include "UniformPrior.hpp"
#include "DirichletPrior.hpp"

void RunFactory::addStandardParameters(std::vector<std::unique_ptr<AbstractParameter> > &params, const TreeAln &traln ) const 
{
  int highestParamId = -1; 
  if(params.size() > 0 )
    {
      auto ids = std::vector<nat>{};
      for(auto &param : params )
	ids.push_back(param->getId() ); 
      highestParamId = * (std::max_element(ids.begin(), ids.end())) ; 
    } 

  auto allCats = CategoryFuns::getAllCategories(); 
  auto cat2partsUsed = std::unordered_map<Category, std::vector<bool> >(); 
  auto cat2idOfItsKind = std::unordered_map<Category,int>(); 
  for( auto c : allCats)
    {
      cat2partsUsed[c] = std::vector<bool>(traln.getNumberOfPartitions(), false);
      cat2idOfItsKind[c] = -1; 
    }

  // register which partitions are used by which parameter 
  for(const auto &param : params)
    {
      auto cat =param->getCategory(); 
      int id = param->getIdOfMyKind(); 
      if( cat2idOfItsKind[cat] < id )
	cat2idOfItsKind[cat] = id; 
      for(auto &p : param->getPartitions())
	cat2partsUsed[cat].at(p) = true; 
    }

  // add frequency parameters to aa-revmats, if not already there 
  auto relCat = Category::FREQUENCIES; 
  auto&& paramsToAdd= std::vector<std::unique_ptr<AbstractParameter>> {};
  for( const auto &p : params)
    {
      assert(p->getPartitions().size() > 0 );
      if ( p->getCategory() == Category::SUBSTITUTION_RATES
	   &&  traln.getPartition(p->getPartitions()[0]).getDataType() == PLL_AA_DATA ) 
	{
	  for(auto part : p->getPartitions())
	    {
	      if(not cat2partsUsed.at(relCat).at(part) )
		{
		  ++highestParamId;   
		  ++cat2idOfItsKind[relCat]; 
  		  paramsToAdd.push_back(CategoryFuns::getParameterFromCategory(relCat, highestParamId, cat2idOfItsKind[relCat], {part}, traln.getNumberOfTaxa()));
		  cat2partsUsed.at(relCat).at(part) = true; 
		}
	    }
	}
    }
  
  for(auto &p : paramsToAdd)
    params.push_back(std::move(p)); 

  auto divTimeMode = std::any_of(begin(params), end(params), 
				 [](const std::unique_ptr< AbstractParameter> &param){ return param->getCategory() == Category::DIVERGENCE_TIMES
										       || param->getCategory() == Category::DIVERGENCE_RATES ; }); 
  
  // add standard stuff, if not defined yet
  for(auto cat : CategoryFuns::getAllCategories())
    {
      // determine unused partitions 
      auto partsUnused = std::vector<nat>{}; 
      switch(cat)
	{	  
	  // force to have everything linked with those 
	case Category::BRANCH_LENGTHS: 
	case Category::RATE_HETEROGENEITY:
	case Category::TOPOLOGY: 
          if(not divTimeMode)
            {
              for(nat i = 0; i < traln.getNumberOfPartitions() ; ++ i)
                {
                  if(not cat2partsUsed.at(cat).at(i))
                    {
                      partsUnused.push_back(i);
                    }
                }
            }
          else 
            continue; 
          break; 
	case Category::DIVERGENCE_RATES: 
	case Category::DIVERGENCE_TIMES:
	  {
	    // but do not instantiate these parameters, if we are doing
	    // divtime (use the conflicting categories later, to rerduce
	    // the complexity of the code)
	    if(divTimeMode)
	      {
		for(nat i = 0; i < traln.getNumberOfPartitions() ; ++ i)
		  {
		    if(not cat2partsUsed.at(cat).at(i))
		      partsUnused.push_back(i);
		  }
	      }
	    else 
	      continue ; 
	  }
	  break; 
	case Category::AA_MODEL:	
	  {
	    for(nat i = 0; i < traln.getNumberOfPartitions(); ++i)
	      {
		if( traln.getPartition(i).getDataType() == PLL_AA_DATA
		    && not cat2partsUsed[Category::SUBSTITUTION_RATES][i]
		    && not cat2partsUsed[Category::AA_MODEL][i] )
		  partsUnused.push_back(i);
	      }
	  } 
	  break; 
	case Category::FREQUENCIES:
          for(nat i = 0; i < traln.getNumberOfPartitions() ; ++i )
            {
              auto dataType = traln.getPartition(i).getDataType(); 
              if(not cat2partsUsed.at(cat).at(i) // not already there 
                 && dataType != PLL_AA_DATA ) // we use an AA_MODEL as default for proteins 
                partsUnused.push_back(i); 
            }
          break ;
	case Category::SUBSTITUTION_RATES: 
	  {
	    for(nat i = 0; i < traln.getNumberOfPartitions() ; ++i )
	      {
		auto dataType = traln.getPartition(i).getDataType(); 
		if(not cat2partsUsed.at(cat).at(i) // not already there 
		   && dataType != PLL_AA_DATA
		   && dataType != PLL_BINARY_DATA) // we use an AA_MODEL as default for proteins 
		  partsUnused.push_back(i); 
	      }
	  }
	  break; 
	default: 
	  assert(0); 
	}
      
      if(partsUnused.size() == 0 )
        continue; 



      if(CategoryFuns::inUniqueByDefault(cat))
	{
	  auto numNeeded =  ( cat == Category::DIVERGENCE_TIMES ) ? (traln.getNumberOfInnerNodes(true))  : 1;
	  for(int i = 0; i < numNeeded; ++i )
            {
              ++highestParamId; 
              ++cat2idOfItsKind[cat]; 
	      params.push_back(CategoryFuns::getParameterFromCategory(cat, highestParamId, cat2idOfItsKind[cat] , 
								      partsUnused, traln.getNumberOfTaxa())); 
            }
	}
      else 
        {
	  // add a parameter per partition 
          for(auto p : partsUnused)
            {
              ++highestParamId;
              ++cat2idOfItsKind[cat];
	      params.push_back(CategoryFuns::getParameterFromCategory(cat, highestParamId, cat2idOfItsKind[cat], 
								      {p}, traln.getNumberOfTaxa())); 
            }
        }
    }


  // TODO sanity check, whehther conflicting parameters are there...
}


void RunFactory::addStandardPrior(AbstractParameter* var, const TreeAln& traln )
{
  switch(var->getCategory())			// TODO such switches should be part of an object
    {
    case Category::TOPOLOGY:  
      var->setPrior( make_unique<UniformPrior>(0,0) ); // TODO : proper topology prior? 
      break; 
      
    case Category::DIVERGENCE_TIMES:
      var->setPrior(make_unique<UniformPrior>(0,10)); 
      break; 
    case Category::DIVERGENCE_RATES: 
    {
      std::vector<double> cats(2*traln.getNumberOfTaxa() - 2, 1.0);
      var->setPrior(make_unique<DirichletPrior>(cats));
      break; 
    }
    case Category::BRANCH_LENGTHS: 
      var->setPrior( make_unique<ExponentialPrior>(10.0 ));
      break; 
    case Category::FREQUENCIES: 
      {
	auto& partition = traln.getPartition(var->getPartitions()[0]);
	assert(partition.getDataType() == PLL_DNA_DATA 
	       || partition.getDataType() == PLL_AA_DATA 
	       || partition.getDataType() == PLL_BINARY_DATA
	       ); 
	var->setPrior(std::unique_ptr<AbstractPrior>(new DirichletPrior(std::vector<double>(partition.getStates() , 1.)))); 
      }
      break; 
    case Category::SUBSTITUTION_RATES: 
      {
	auto& partition = traln.getPartition(var->getPartitions()[0]);	;
	var->setPrior( make_unique<DirichletPrior>( std::vector<double>(RateHelper::numStateToNumInTriangleMatrix(partition.getStates()), 1.) )); 
      }
      break; 
    case Category::RATE_HETEROGENEITY: 
      var->setPrior(make_unique<UniformPrior>(1e-6, 200)); 
      break; 
    case Category::AA_MODEL : 
      {
	auto modelProbs = std::unordered_map<ProtModel,double>{}; 
	for(auto model : ProtModelFun::getAllModels())
	  modelProbs[model] = 1.; 
	auto&& prior =  make_unique<DiscreteModelPrior>(modelProbs);
	var->setPrior(std::move(prior));
      }
      break; 
    default: 
      assert(0); 
    }
}


void RunFactory::addPriorsToParameters(const TreeAln &traln,  const BlockPrior &priorInfo, ParameterList &variables)
{
  auto& priors = priorInfo.getPriors();

  for(auto &v : variables)
    {
      auto partitionIds = v->getPartitions(); 
      
      auto cat = v->getCategory();
      auto foundPrior = bool{false}; 

      // try to find a partition specific prior 
      for(auto iter = priors.find(cat) ; iter != priors.end() ; ++iter)
	{
	  auto &partitionsOfPrior = std::get<0>(iter->second); 
	  foundPrior = std::any_of(begin(partitionIds), end(partitionIds), [&]( nat tmp ){  return partitionsOfPrior.find(tmp) != partitionsOfPrior.end(); } ); 
	  
	  auto& prior = *(std::get<1>(iter->second).get()); 

	  if(foundPrior)
	    {
	      if(not v->priorIsFitting(prior, traln))
		{
		  tout  << "You forced prior " << &prior << " to be applied to parameter  "  << v << ". This is not possible. "  << std::endl; 
		  exitFunction(-1, true);
		}

	      // tout << "setting prior " << &prior << 
              // " for param "<< v.get() << std::endl; 

	      v->setPrior( std::unique_ptr<AbstractPrior>(prior.clone()) ) ; 
	      break; 
	    }
	}

      // try to find a general prior 
      if(not foundPrior)
	{
	  for(auto iter = priors.find(cat) ; iter != priors.end() ; ++iter)
	    {
	      auto &prior = *(std::get<1>(iter->second).get()); 

	      if(not v->priorIsFitting(prior, traln))
		continue; 

	      auto &partitionsOfPrior = std::get<0>(iter->second); 
	      foundPrior = partitionsOfPrior.size() ==  0; 
	      if(foundPrior)
		{
		  // tout << "setting prior " << &prior << " for param "<< v.get() << std::endl; 
		  v->setPrior( std::unique_ptr<AbstractPrior>(prior.clone()) ) ; 
		  break; 
		}
	    }
	}
      
      if(not foundPrior)
	addStandardPrior(v,traln); 
    }
}


void RunFactory::addSecondaryParameters(AbstractProposal* proposal,  ParameterList &allParameters, nat numTaxa)
{
  // TODO reduce code complexity 

  auto primParams = proposal->getPrimaryParameterView(); 
  
  auto doingDivRates = std::any_of(begin(primParams), end(primParams), [](const AbstractParameter* param){return param->getCategory() == Category::DIVERGENCE_RATES; }); 
  auto doingDivTimes = std::any_of(begin(primParams), end(primParams), [](const AbstractParameter* param){return param->getCategory() == Category::DIVERGENCE_TIMES; }); 

  if(doingDivRates)
    {
//      auto tp = std::find_if(begin(allParameters), end(allParameters), [](const AbstractParameter* param){return param->getCategory() == Category::DIVERGENCE_TIMES; });
//      proposal->addSecondaryParameter((*tp)->getId());
      for(auto &p : allParameters)
	{
	  if(p->getCategory() == Category::DIVERGENCE_TIMES)
	    {
	      // now add all *other* div time parameters as secondary parameters
	      proposal->addSecondaryParameter(p->getId());
	    }
	}
    }
  else if(doingDivTimes)
    {
      // what is our current primary parameter id?
      assert(primParams.size() == 1 ); 
      auto primId = primParams[0]->getId();

      for(auto &p : allParameters)
	{
	  if(p->getCategory() == Category::DIVERGENCE_RATES)
	    {
	      // add the div-rates as a secondary paramater
	      proposal->addSecondaryParameter(p->getId()); 
	    }
	  else if(p->getCategory() == Category::DIVERGENCE_TIMES && p->getId() != primId)
	    {
	      // now add all *other* div time parameters as secondary parameters 
	      proposal->addSecondaryParameter(p->getId()); 
	    }
	}
    }
  else 
    { 
      // get all branch length pararemeters   
      auto blParameters = allParameters.getViewByCategory(Category::BRANCH_LENGTHS);
      bool needsBl = false; 
  
      auto myPartitions = std::unordered_set<nat> {}; 
  
      for(auto &p: proposal->getPrimaryParameterView())
        {
	  needsBl |=  ( p->getCategory() == Category::FREQUENCIES 
			|| p->getCategory() == Category::SUBSTITUTION_RATES
			|| p->getCategory() == Category::TOPOLOGY
			|| p->getCategory() == Category::AA_MODEL
                        ); 
	  auto ps = p->getPartitions();
          myPartitions.insert(ps.begin(), ps.end()); 
        }

      if(needsBl)
        {
          for(auto &blRandVar : blParameters)
            {
              auto partitions =  blRandVar->getPartitions();
              if(std::any_of(partitions.begin(),partitions.end(), 
                             [&](nat i){ return myPartitions.find(i) != myPartitions.end(); }))
                proposal->addSecondaryParameter( blRandVar->getId() ); 
	    }
	}
    }
}



std::tuple<std::vector<std::unique_ptr<AbstractProposal>>,std::vector<ProposalSet> > 
RunFactory::produceProposals(const BlockProposalConfig &propConfig, const BlockPrior &priorInfo, 
			     ParameterList  &params, const TreeAln &traln, bool componentWiseMH, ParallelSetup& pl)
{
  auto proposals = std::vector<std::unique_ptr<AbstractProposal> >{} ; 
  auto resultPropSet = std::vector<ProposalSet >{} ; 
  addPriorsToParameters(traln, priorInfo, params);

  auto reg = ProposalRegistry{}; 

  // instantiate all proposals that integrate over one parameter 
  for(auto &v : params)
    {
      if(not v->getPrior()->needsIntegration() )
	continue;

      if( componentWiseMH && v->getCategory() != Category::TOPOLOGY )
	continue; 
      
      auto tmpResult = reg.getSingleParameterProposals(v->getCategory(), propConfig,  traln, pl, params); 

      // remove proposals that are not meant for DNA/AA
      if(v->getCategory() == Category::SUBSTITUTION_RATES)
	{
	  // bool isProtPartition = traln.getPartition(v->getPartitions().at(0)).dataType == AA_DATA; 
	  bool isDNAPartition = traln.getPartition(v->getPartitions().at(0)).getDataType() == PLL_DNA_DATA; 
	  auto tmp = decltype(tmpResult){}; 
	  for(auto &elem : tmpResult )
	    {
	      if( not ( elem->isForProteinOnly() && isDNAPartition ) ) // TODO more generic!  
		tmp.push_back(std::move(elem)); 
	    }
	  tmpResult.clear(); 
	  tmpResult = std::move( tmp ) ; 
	}
      
      for(auto &p : tmpResult )
	{
	  p->addPrimaryParameter(v->getId());
	  addSecondaryParameters(p.get(), params, traln.getNumberOfTaxa()); 
	}

      // dammit...
      for(auto it = tmpResult.begin(); it != tmpResult.end(); )
	{
	  proposals.emplace_back(std::move(*it));
	  it = tmpResult.erase(it);
	}
    }

  // instantiate proposals that integrate over multiple over an entire
  // category gather all parameters that we can integrate over
  // together in a partitioned manner and output a good set of
  // proposals
  nat blCtr = 0; 
  auto mashableParameters = std::vector<AbstractParameter*>{}; 
  for(auto &v : params)
    {
      if(not v->getPrior()->needsIntegration() ) 
	continue;

      // those are prototypes! non-owning pointers 
      switch(v->getCategory())
	{
	case Category::DIVERGENCE_RATES: 
	case Category::DIVERGENCE_TIMES: 
	case Category::SUBSTITUTION_RATES: 
	case Category::FREQUENCIES:
	case Category::RATE_HETEROGENEITY: 
	case Category::AA_MODEL:	
	  mashableParameters.push_back(v); 
	  break; 
	case Category::BRANCH_LENGTHS:
	  {
	    mashableParameters.push_back(v); 
	    ++blCtr; 
	  }
	case Category::TOPOLOGY:
	  break; 
	default: 
	  ;
	}
    }  

  // TODO that's all a bit cumbersome, has to be re-designed a bit 
  if(componentWiseMH)
    {
      auto cat2param = std::unordered_map<Category, std::vector<AbstractParameter*> >{}; 
      for(auto &p:  mashableParameters)
	cat2param[p->getCategory()].push_back(p); 

      for(auto &elem : cat2param)
	{
	  auto proposalsForSet = reg.getSingleParameterProposals(elem.first, propConfig, traln, pl,params );

	  // filter out proposals that do not fit the current data
	  // type of
	  // TODO improve the setup here 
	  if(std::get<0>(elem) == Category::SUBSTITUTION_RATES)
	    {
	      // bool isProtPartition = traln.getPartition(std::get<1>(elem).at(0)->getPartitions().at(0)).dataType == AA_DATA; 
	      bool isDNAPartition = traln.getPartition(std::get<1>(elem).at(0)->getPartitions().at(0)).getDataType() == PLL_DNA_DATA; 
	      auto tmp = decltype(proposalsForSet){}; 
	      for(auto &elem2 : proposalsForSet )
		{
		  if( not ( elem2->isForProteinOnly() &&  isDNAPartition ) ) // TODO more generic! 
		    tmp.push_back(std::move(elem2)); 
		}
	      proposalsForSet.clear(); 
	      proposalsForSet = std::move( tmp ) ; 
	    }	  
	  
	  // this and the above for-loop essentially produce all
	  // proposals, that we'd also obtain in the default case =>
	  // but we need a set of it
	  for(auto &proposalType : proposalsForSet)
	    {
	      auto lP = std::vector<std::unique_ptr<AbstractProposal> >{}; 	      
	      for(auto &p : elem.second)
		{
		  auto proposalClone = std::unique_ptr<AbstractProposal>(proposalType->clone()); 
		  proposalClone->addPrimaryParameter(p->getId());
		  addSecondaryParameters(proposalClone.get(), params, traln.getNumberOfTaxa()); 
		  lP.push_back(std::move(proposalClone));		  
		} 
	      proposalType->setInSetExecution(true);

	      resultPropSet.emplace_back(lP[0]->getRelativeWeight(), std::move(lP)); 
	    }
	}
    }

  return std::make_tuple(std::move(proposals), resultPropSet); 
}

