#include <cassert>

#include "AbstractParameter.hpp"

// #include "Branch.hpp"
#include "TreePrinter.hpp"

#include "BranchLength.hpp"


TreePrinter::TreePrinter(bool withBranchLengths_I , bool withInternalNodes_I , bool withRealNames_I) 
  : withBranchLengths(withBranchLengths_I)
  , withInternalNodes(withInternalNodes_I)
  , withRealNames(withRealNames_I)
{
}


std::string TreePrinter::printTree(const TreeAln& traln, AbstractParameter* param)
{
  auto vect = std::vector<AbstractParameter*>{};
  vect.push_back(param); 
  return printTree(traln, vect);   
}


std::string TreePrinter::printTree(const TreeAln& traln, const std::vector<AbstractParameter*> &params)
{
  auto &&ss = std::stringstream{}; 
  ss << SOME_FIXED_PRECISION; 

  ss << "("; 
  helper(traln, ss, traln.getNode(1)->back, true, params );   
  ss << ")" ; 
  if(withBranchLengths)
    ss << ":0.0"; 
  ss << ";"; 
  return ss.str();
} 



void TreePrinter::printBranchLength(const TreeAln& traln, std::stringstream &ss, nodeptr p , const std::vector<AbstractParameter*> &params)
{
  ss << MAX_SCI_PRECISION; 

  if(params.size() == 1 )
    {
      auto param = params[0]; 
      // ss << ":" << traln.getBranch(p,param).getInterpretedLength( param); 
      ss << ":" << traln.getBranch(p,param).toMeanSubstitutions(param->getMeanSubstitutionRate()); 
    }
  else 
    {
      ss << ":["; 
      bool isFirst = true ; 
      for(auto param : params)
	{
	  // ss << (isFirst ? "" : ",")  << traln.getBranch(p,param).getInterpretedLength( param); 
	  ss << (isFirst ? "" : ",") << traln.getBranch(p,param).toMeanSubstitutions(param->getMeanSubstitutionRate()); 
	  isFirst = false; 
	}
      ss << "]"; 
    }
}


void TreePrinter::helper(const TreeAln &traln, std::stringstream &ss, 
			 nodeptr p, bool isFirst, const std::vector<AbstractParameter*> &params)
{  
  if(traln.isTipNode(p))
    {
      if(withRealNames)
	{
	  auto &taxa = traln.getTaxa(); 
	  ss << taxa.at(p->number-1) ; 
	// ss << traln.getTrHandle().nameList[p->number]; 
	}
      else 
	ss << p->number; 
    }
  else 
    {
      if (not isFirst )
	ss << "("; 
      helper(traln, ss, p->next->back, false, params); 
      ss << "," ; 
      helper(traln, ss, p->next->next->back, false, params ); 
      if(not isFirst)
	ss << ")"; 
    }

  if(not isFirst && withInternalNodes && not traln.isTipNode(p))
    ss << p->number ; 

  if(not isFirst && withBranchLengths && params.size() != 0 )
    printBranchLength(traln, ss, p, params);

  if(isFirst)
    {
      ss << "," << p->back->number; 
      if(withBranchLengths && params.size() !=  0)
	printBranchLength(traln, ss, p, params);
    }
}

std::string TreePrinter::printTree(const TreeAln& traln)
{
  return printTree(traln, {});
} 
