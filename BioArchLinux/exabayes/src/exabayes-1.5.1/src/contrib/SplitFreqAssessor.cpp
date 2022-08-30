#include <fstream>
#include <algorithm>
#include <sstream> 
#include <cassert>

#include <ncl/ncl.h>

#include "TreeAln.hpp"
#include "Arithmetics.hpp"
#include "SplitFreqAssessor.hpp"
#include "BipartitionHash.hpp"


SplitFreqAssessor::SplitFreqAssessor(std::vector<string> fileNames, bool expensiveCheck)
  : TreeProcessor(fileNames, expensiveCheck)
  , newBipHashes{}
  , file2numTree{}
{
  for(auto file : fileNames)
    {
      file2numTree[file] = getNumTreeAvailable(file);
      newBipHashes.emplace_back(_taxa.size());
    }
}


// void SplitFreqAssessor::extractBipsNew(nat start, nat end, bool takeAll)
void SplitFreqAssessor::extractBips( const std::vector<nat> &start, const std::vector<nat> &end) 
{
  int ctr = 0; 
  
  for (auto filename : _fns)
    {
      nat endhere = end[ctr]; 
      // nat endhere = end; 
      // if(takeAll)
      // 	endhere = file2numTree[filename]; 

      auto&& ifh = std::ifstream{filename}; 

      for(nat i = 0; i < start[ctr]; ++i)
	skipTree(ifh);

      auto &bipHash  = newBipHashes.at(ctr);
      for(nat i = start[ctr] ; i < endhere; ++i)
	{
	  nextTree<false>(ifh);
	  bipHash.addTree(*_tralnPtr,false, false);
	}
      ++ctr; 
    }
}


auto SplitFreqAssessor::computeAsdsfNew(double ignoreFreq)
 -> std::pair<double,double> 
{
  auto allBips = std::unordered_set<Bipartition>{}; 

  for(auto &h : newBipHashes)
    for(auto &elem : h)
      allBips.insert(elem.first);

  auto numTrees = std::vector<double>{}; 
  for(auto &elem : newBipHashes)
    {
      nat tn = elem.getTreesAdded() ; 
      numTrees.push_back(tn);
    }

  auto numOccs = std::unordered_map<Bipartition,std::vector<double> >{}; 
  for(auto elem : allBips)
    {
      auto tmp = std::vector<double>{};
      for(auto &h : newBipHashes)
	tmp.push_back( double(h.getPresence(elem).count()) );
      numOccs.insert(std::make_pair(elem, tmp)); 
    }

  auto asdsfPart = std::vector<double>{};
  for(auto &elem : allBips)
    {
      nat ctr = 0; 
      bool isRelevant = false;  
      for(auto &v : numOccs[elem])
	{
	  v /= numTrees.at(ctr) ; 
	  isRelevant |= ( ignoreFreq < v); 
	  ++ctr; 
	}
      
      auto sd = sqrt ( Arithmetics::getVariance(numOccs[elem]) ) ; 

      if(isRelevant)
	asdsfPart.push_back( sd );
    }

  auto asdsfMax = std::max_element(begin(asdsfPart), end(asdsfPart)); 
  auto mean = Arithmetics::getMean(asdsfPart); 

  return std::make_pair(mean, *asdsfMax); 
}

nat SplitFreqAssessor::getNumTreeAvailable(string fileName)
{  
  string whitespace = " \t"; 
  ifstream infile(fileName); 
  string line ; 
  bool foundTaxaStart = false,
    foundTreeStart = false; 
  int numTrees = 0; 

  while(getline(infile, line))
    {
      auto cleanline = trim(line); 

      if(cleanline.compare("end;") == 0 )
	{
	  break; 
	}
      else if(foundTreeStart)	// check number of trees 
	{
	  if(cleanline[cleanline.size()-1] == ';') 
	    ++numTrees; 
	  else 
	    break; 		// DONE  
	}
      else if(foundTaxaStart) 	// check if all taxa are there and have the appropriate number 
	{
	  if(cleanline[cleanline.size()-1] == ';')
	    foundTreeStart = true ; 

	  string cleanerString =  trim(cleanline,  ",;"); 
	  auto pos = cleanerString.find_first_of(whitespace, 0); 
	  string num = cleanerString.substr(0, pos),
	    name = cleanerString.substr(pos+1, cleanerString.size()); 

	  int index; 
	  istringstream(num) >> index ; 
	  index--; 

	  if(name.compare(_taxa.at(index)) != 0)
	    {
	      std::cout << "expected " << name << " but got " << _taxa.at(index) << std::endl; 
	      assert(0); // assert same taxa names 
	    }
	} 
      else if(cleanline.compare("translate") == 0)
	foundTaxaStart = true; 
    }

  assert(foundTaxaStart && foundTreeStart); 
  return numTrees; 
}


int SplitFreqAssessor::getMinNumTrees()
{
  nat minimum = std::numeric_limits<nat>::max(); 
  for(auto elem : file2numTree)
    {
      nat avHere = std::get<1>(elem); 
      if(avHere < minimum)
	minimum = avHere;       
    }
  return minimum; 
} 
