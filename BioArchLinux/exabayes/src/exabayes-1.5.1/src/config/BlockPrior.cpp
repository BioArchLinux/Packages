#include "BlockPrior.hpp"

#include <sstream>
#include <cmath>
#include <limits>


#include "extensions.hpp"

#include "DiscreteModelPrior.hpp"
#include "UniformPrior.hpp"
#include "ExponentialPrior.hpp"
#include "DirichletPrior.hpp"
#include "FixedPrior.hpp"

static void expectString( std::string expectation, NxsToken& token)
{
  bool okay = token.GetToken().EqualsCaseInsensitive(expectation.c_str()); 
  if(not okay)
    {
      std::cerr << "error while parsing the config file: expected " << expectation << " but got " << token.GetToken() << std::endl; 
      exitFunction(-1, false); 
    }
}


static std::vector<double> parseValues(NxsToken &token)
{
  auto result = std::vector<double>{}; 

  // assumption: we have already seen the ')'

  while(token.GetToken().compare(")") != 0)
    {
      auto &&iss = std::istringstream{token.GetToken()}; 
      auto value = double{0.};
      iss >> value; 
      result.push_back(value);
      token.GetNextToken();
      if(token.GetToken().compare(",") == 0)
	token.GetNextToken();
    }
  
  return result; 
}


std::unique_ptr<AbstractPrior> BlockPrior::parsePrior(NxsToken &token)  
{
  auto value = token.GetToken(false); 
  token.GetNextToken();

  assert(token.GetToken(false).compare("(") == 0); 

  if(value.EqualsCaseInsensitive("uniform"))
    {
      token.GetNextToken();

      // for non-continuous variables (e.g., topology)
      if(token.GetToken().compare(")") == 0) 
	return make_unique<UniformPrior>(0,0); 

      double n1 = parseScientificDouble(token); 

      assert(token.GetToken().compare(",") == 0);
      token.GetNextToken();

      double n2 = parseScientificDouble(token); 

      assert(token.GetToken().compare(")") == 0);
      return make_unique<UniformPrior>(n1,n2);  
    }
  else if(value.EqualsCaseInsensitive("disc"))
    {
      expectString("(", token);
      
      auto modelsProbs = std::unordered_map<ProtModel, double>{};

      auto remainder = std::numeric_limits<double>::infinity();

      while(token.GetToken().compare(")") != 0)
	{
	  token.GetNextToken();

	  auto foundRemainder = token.GetToken().EqualsCaseInsensitive("remainder"); 
	  if(foundRemainder)	// keep track of the weight 
	    {
	      if( not ( std::isinf( remainder) && remainder > 0 )  )
		{
		  std::cerr << "Encountered 'remainder' twice while defining aaPr" << std::endl; 
		  exitFunction(-1, true); 
		}

	      token.GetNextToken();
	      expectString("=", token); 
	      token.GetNextToken();

	      auto &&iss = std::istringstream{token.GetToken()};
	      iss >> remainder ; 
	      token.GetNextToken();
	    } 
	  else 			// simply parse that model 
	    {
	      auto modelRes = ProtModelFun::getModelFromStringIfPossible(token.GetToken()); 
	      if(not std::get<0>(modelRes) )
		{
		  std::cerr << "Error: expected " << token.GetToken() << " to be a valid protein model name" << std::endl; 
		  exitFunction(-1, true); 
		}
	      auto model = std::get<1>(modelRes);

	      token.GetNextToken();
	      expectString("=", token); 

	      token.GetNextToken(); 
	      auto &&iss = std::istringstream{token.GetToken()}; 
	      auto weight = double{0.}; 
	      iss >> weight; 

	      if(modelsProbs.find(model) != modelsProbs.end())
		{
		  std::cerr << "Error: model " <<  model << "occurred more than once in your specification of a discrete amino acid model prior."  << std::endl; 
		  exitFunction(-1, true); 
		}

	      modelsProbs[model] = weight; 

	      token.GetNextToken();
	    }
	}

      if(   not std::isinf(remainder) )
	{
	  for(auto model : ProtModelFun::getAllModels())
	    {
	      if( modelsProbs.find(model) == modelsProbs.end()  )
		modelsProbs[model] = remainder; 
	    }
	}

      return make_unique<DiscreteModelPrior>(modelsProbs);
    }
  else if(value.EqualsCaseInsensitive("dirichlet"))
    {
      expectString("(",token);
      token.GetNextToken();
      
      auto alphas = parseValues(token); 
      auto pr = new DirichletPrior(alphas);

      return std::unique_ptr<AbstractPrior>(pr); 
    }
  else if(value.EqualsCaseInsensitive("fixed"))
    { 
      token.GetNextToken();

      auto res = ProtModelFun::getModelFromStringIfPossible(token.GetToken()); 
      auto foundProt = std::get<0>(res); 

      if( foundProt )
	{
	  auto model = std::get<1>(res);

	  token.GetNextToken();
	  assert(token.GetToken().compare(")" ) == 0 ); 

	  std::unordered_map<ProtModel,double> tmp = {{model,1.}}; 
	  return make_unique<DiscreteModelPrior>( tmp  );
	}
      else 
	{
	  auto fixedValues = parseValues(token);
	  auto &&result = make_unique<FixedPrior>(fixedValues); 
	  return std::move(result);
	}
    }
  else if(value.EqualsCaseInsensitive("exponential"))
    {
      token.GetNextToken();

      double n1 = parseScientificDouble(token);

      assert(token.GetToken().compare(")") == 0);
      return make_unique<ExponentialPrior>(n1);
    }
  else 
    {
      cerr << "attempted to parse prior. Did not recognize keyword " <<  value << endl; 
      exitFunction(-1, true); 
      return make_unique<ExponentialPrior>(0);
    }
}


void BlockPrior::Read(NxsToken &token) 
{
  DemandEndSemicolon(token, "PRIOR");

  while(true)
    {
      token.GetNextToken();
      auto  res = HandleBasicBlockCommands(token); 

      if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
	return;
      if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
	{
	  auto cat = CategoryFuns::getCategoryByPriorName(token.GetToken()); 
	  token.GetNextToken();

	  auto partitions  = std::unordered_set<nat> {}; 
	  if(token.GetToken().compare("{") == 0)
	    {
	      while(not token.GetToken().EqualsCaseInsensitive("}"))
		{
		  token.GetNextToken();
		  auto val = token.GetToken().ConvertToInt(); 
	      
		  if( _numPart <= nat(val)  )
		    {
		      tout << "Error while parsing priors: you specified partition id " << val << " while ExaBayes assumes, that you only have " << _numPart << " partitions" << std::endl; 
		      exitFunction(-1, true); 
		    }
	      
		  partitions.insert(val);
		  token.GetNextToken();
		}

	      assert(token.GetToken().compare("}") == 0) ;
	      token.GetNextToken();
	    }

	  auto prior = parsePrior(token);
	  // tout << "parsed prior " << prior.get() << " for category " << cat  << std::endl; 

	  // account for fixed bl prior that may not have any value at all 

	  if(cat == Category::BRANCH_LENGTHS && prior->getInitialValue().values.size() == 0)
	    prior->setKeepInitData(); 

	  _parsedPriors.insert(std::make_pair( cat, std::make_tuple(partitions,std::move(prior))  ));
	}
    }  
} 


void BlockPrior::verify() const
{
  // TODO actually it seems the things below are not really necessary... 
  auto range =  _parsedPriors.equal_range(Category::BRANCH_LENGTHS); 
  for(auto iter = std::get<0>(range) ; iter != std::get<1>(range) ; ++iter )
    {
      auto &elem = std::get<1>(*iter); 
      auto &setOfParts = std::get<0>(elem); 
      auto priorPtr = std::get<1>(elem).get(); 
      if( setOfParts.size() > 0 && dynamic_cast<FixedPrior*>(priorPtr)  != nullptr) 
	  {
	    tout << "You attempted to set a fixed branch lengths prior to one or more\n"
		 << "partitions (but not all of them). Currently, this option is not\n"
		 << "supported. If you urgently need this feature, please contact us." << std::endl; 
	    exitFunction(-1, true);
	  }
    }
} 

