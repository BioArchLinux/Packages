#include <sstream>
#include "extensions.hpp" 
#include <string.h>
#include <cassert>
#include <iostream>
#include <cmath>
// #include "Branch.hpp"
#include "BasicTreeReader.hpp"
#include "TreeProcessor.hpp"
#include "BranchLengthsParameter.hpp"

using namespace std;

TreeProcessor::TreeProcessor(std::vector<std::string> fileNames, bool expensiveCheck)
  : _tralnPtr{nullptr}
  , _fns{}
  , _taxa(fillTaxaInfo(fileNames.at(0)))
{
  assert(fileNames.size() > 0); 

  if(expensiveCheck && fileNames.size() > 1)
    {
      for(auto f : fileNames )
	{
	  auto taxa = fillTaxaInfo(f); 
	  bool okay = true;  
	  for(nat i = 0; i < taxa.size() ; ++i)
	    okay &= (_taxa[i].compare(taxa[i]) == 0 ); 
	  if(not okay)
	    {
	      std::cout << "Error: file " << f << " contains a different numbering of taxa than " << fileNames[0] << ". At the moment, " << PROGRAM_NAME << " does not support this. Write us an e-mail, if this feature is important for you. " << std::endl; 
	      exitFunction(-1, false);
	    }
	}
    }

  _tralnPtr = make_unique<TreeAln>( int(_taxa.size()), false);
  
  // only necessary, if we actually have branch lengths (so resources
  // are wasted, if we only read the topology)
  auto blRes = BranchLengthResource(); 
  blRes.initialize(nat(_taxa.size()), 1 ); 
  _tralnPtr->setBranchLengthResource(blRes); 
  
  _fns = fileNames; 
}


TreeProcessor::TreeProcessor(TreeProcessor&& tp) 
  : _tralnPtr(std::move(tp._tralnPtr))
  , _fns(tp._fns)
  , _taxa(tp._taxa)
{
}  


TreeProcessor& TreeProcessor::operator=(TreeProcessor &&rhs)
{
  if(this == &rhs)
    return *this; 
  else 
    assert(0); 
} 


template<bool readBl>
void TreeProcessor::nextTree(std::istream &treefile) 
{
  auto paramPtr = make_unique<BranchLengthsParameter>(0,0, std::vector<nat>{0});   
  paramPtr->addPartition(0);

  while( treefile.get() != '('); 
  treefile.unget();

  auto bt = BasicTreeReader<IntegerLabelReader,
			    typename std::conditional<readBl,
						      ReadBranchLength,
						      IgnoreBranchLength>::type>( nat( _taxa.size()) );
  auto branches = bt.extractBranches(treefile);

  _tralnPtr->unlinkTree();
  for(auto b :branches)
    {
      _tralnPtr->clipNode(_tralnPtr->getUnhookedNode(b.getPrimNode()), _tralnPtr->getUnhookedNode(b.getSecNode()) );
      if(readBl)
	{
	  _tralnPtr->setBranchUnchecked(b);
	}
    }
}

void TreeProcessor::skipTree(std::istream &iss)
{
  while( iss &&  iss.get() != ';'); 
}


std::string TreeProcessor::trim(const std::string& str, const std::string& whitespace) 
{
  const auto strBegin = str.find_first_not_of(whitespace);
  if (strBegin == std::string::npos)
    return ""; 
  const auto strEnd = str.find_last_not_of(whitespace);
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}



std::vector<std::string>
TreeProcessor::fillTaxaInfo(std::string fileName)
{
  auto result = std::vector<std::string>{}; 
  auto whiteSpace = " \t";

  auto &&infile =  std::ifstream(fileName);
  
  if (!infile)
    {
      tout << "Warning: could not find file " << fileName << std::endl;
      assert(0);
    }
  
  
  auto line = std::string{} ; 
  bool foundStart = false; 
  bool abort = false;

  while(not abort && getline(infile, line))
    {      
      std::string cleanLine = trim(line); 

      if(foundStart)
	{
	  if(cleanLine[cleanLine.size()-1] == ';') // we are done 
	    abort = true; 

	  std::string cleanerString = trim(cleanLine, ",;"); 

	  auto pos = cleanerString.find_first_of(whiteSpace, 0);
	  std::string num =  cleanerString.substr(0, pos),
	    name = cleanerString.substr(pos+1, cleanerString.size()); 	  

	  // _taxa.push_back(name); 
	  result.push_back(name);
	}
      else if(cleanLine.compare("translate") == 0  )
	foundStart = true; 
    }  

  assert(foundStart); 
  return result; 
}


template void TreeProcessor::nextTree<true>(std::istream &treefile) ; 
template void TreeProcessor::nextTree<false>(std::istream &treefile) ;
