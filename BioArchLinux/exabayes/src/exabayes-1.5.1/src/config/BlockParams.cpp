#include "BlockParams.hpp"
#include "GlobalVariables.hpp"

#include <algorithm>

extern void genericExit(int code); 


void BlockParams::partitionError(nat partition, size_t totalPart) const
{
  std::cerr << "In the parameter block of the configuration file you specified partition " << partition << ". However, there are only " << totalPart << " partitions in total in your alignment." << std::endl; 
  std::cerr << "NOTICE that the first partition has id 0 and the last of n partitions has the id (n-1) . " << std::endl; 
  exitFunction(-1, true); 
}


void BlockParams::parseScheme(NxsToken& token, Category cat, nat &idCtr)
{
  auto numPart = tralnPtr->getNumberOfPartitions();
  auto partAppeared = std::vector<bool>(numPart, false); 

  token.GetNextToken();
  assert(token.GetToken().EqualsCaseInsensitive("=")); 
  token.GetNextToken();
  assert(token.GetToken().EqualsCaseInsensitive("(")); 

  auto scheme = std::vector<std::vector<nat>>{}; 
  while(not token.GetToken().EqualsCaseInsensitive(")") )
    {
      auto schemePart = std::vector<nat>{}; 
      bool startingNext = true; 
      
      // parse one partition part 
      while( not token.GetToken().EqualsCaseInsensitive(")") 	     
	     && (startingNext || not token.GetToken().EqualsCaseInsensitive(",")) )
	{
	  startingNext = false; 
	  // check if the separation item is an expasion item (-)
	  auto isLinkedRange =  token.GetToken().EqualsCaseInsensitive("-") ; 
	  auto isUnlinkedRange = token.GetToken().EqualsCaseInsensitive(":") ; 

	  token.GetNextToken();
	  nat part = token.GetToken().ConvertToInt();
	  nat start = ( isLinkedRange || isUnlinkedRange )  ? schemePart.back() +1  : part; 
	  nat end = part + 1 ;  

	  for(nat i = start ;  i < end ; ++i )
	    {
	      if(not (i < numPart))
		partitionError(i, numPart); 
	      if(partAppeared.at(i))
		{
		  tout << "error: partition " << i << " occurring twice in the same scheme. Check your parameter-block!" << std::endl; 
		  exitFunction(-1, true); 
		}
	      partAppeared.at(i) = true; 

	      if(isUnlinkedRange)
		{
		  scheme.push_back(schemePart);
		  schemePart = {i}; 
		}
	      else 
		schemePart.push_back(i); 
	    }

	  token.GetNextToken(); 
	}

      scheme.push_back(schemePart); 
    }

  // instantiate the parameters 
  for(auto schemePart : scheme )
    {
      assert(schemePart.size()  > 0 ); 
      parameters.push_back(CategoryFuns::getParameterFromCategory(cat,idCtr,getNumSeen(cat), schemePart, tralnPtr->getNumberOfTaxa()));
      ++idCtr; 
    }
}


void BlockParams::Read(NxsToken &token)
{
  DemandEndSemicolon(token, "PARAMS");
  nat idCtr = 0; 

  auto catsFound = std::unordered_set<Category>{}; 

  while(true)
    {
      token.GetNextToken();
      NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token); 

      if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
	return;

      if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
	{	  
	  auto str = token.GetToken(false); 

	  auto cat = CategoryFuns::getCategoryFromLinkLabel(str); 	  
	  parseScheme(token, cat, idCtr); 

	  if(catsFound.find(cat) != catsFound.end())
	    {
	      cerr << "parsing error: found a linking scheme for category  " <<  CategoryFuns::getShortName(cat) << " twice. Aborting." ; 
	      exitFunction(-1, true); 
	    }

	  if( cat == Category::TOPOLOGY)
	    {
	      cerr <<  "not implemented"; 
	      assert(0); 
	    }	    
	}
    }
}


std::vector<std::unique_ptr<AbstractParameter> > BlockParams::getParameters() const
{
  std::vector<std::unique_ptr<AbstractParameter> > result; 
  for(auto &p : parameters )
    result.push_back(std::unique_ptr<AbstractParameter>(p->clone() )); 
  return result; 
}



nat BlockParams::getNumSeen(Category cat) 
{
  nat result = 0; 
  for(auto &p : parameters)
    if(p->getCategory() == cat )
      ++result;
  return result; 
} 
