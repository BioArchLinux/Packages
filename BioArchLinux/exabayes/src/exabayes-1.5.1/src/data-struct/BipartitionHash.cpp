#include "BipartitionHash.hpp"
// #include "Branch.hpp"

#include "BranchPlain.hpp"

#include <algorithm>
#include <iostream>

#include "Randomness.hpp"

BipartitionHash::BipartitionHash(nat numTax)   
  :_bipPresence{}
  , _bipBranchLengths{}
  , _bipMeaning(numTax)
  , _treesAdded(0)
{
  // must be deterministic!
  auto seed = randCtr_t{{0xdeadbeef,0xbadcafee}}; 
  Randomness rand(seed);

  // we do nat allow a zero 
  for(auto &r : _bipMeaning)
    while(r == 0)
      r = rand();
}


void BipartitionHash::addTree(const TreeAln &traln, bool withBranch, bool withTrivial)
{
  auto pStart = traln.getTrHandle().nodep[1]->back; 
  auto desc = traln.getDescendents(BranchPlain(pStart->number, pStart->back->number)); 

  nat bipsAdded = 0; 

  // explicitly add the first trivial bipartition 
  if(withTrivial)
    {
      auto result = Bipartition{}; 

      auto p = traln.getTrHandle().nodep[1] ; 

      nat num = p->number-1; 

      result.initializeWithTaxon(num, _bipMeaning.at(num));     
      
      addBipOccurrence(withBranch, result, _treesAdded, pStart); 
      ++bipsAdded; 
    }

  // recursive descent
  addElement(traln, traln.findNodePtr(desc.first.getInverted()), withBranch, withTrivial, bipsAdded); 
  addElement(traln, traln.findNodePtr(desc.second.getInverted()), withBranch, withTrivial, bipsAdded); 

  // check 
  auto numTax = traln.getNumberOfTaxa(); 
  if(withTrivial)
    assert(bipsAdded == 2 * numTax - 3 );
  else 
    assert(bipsAdded == numTax -3 ); 

  ++_treesAdded;
}


void BipartitionHash::addBipOccurrence(bool withBranch, Bipartition& result, nat treesAdded, nodeptr p )
{
  if(withBranch)
    {
      auto bl = p->z[0]; 
      _bipBranchLengths[result].push_back(bl); 
    }

  auto &precBip = _bipPresence[result]; 
  precBip.reserve(treesAdded); 
  assert(not precBip.isSet(treesAdded)); 
  precBip.set(treesAdded); 
}


Bipartition BipartitionHash::addElement(const TreeAln &traln, nodeptr p, bool withBranch, bool withTrivial, nat &addedSoFar)
{
  auto curBranch = BranchPlain(p->number, p->back->number);

  if(traln.isTipBranch(curBranch))
    {
      auto result = Bipartition{}; 
      // TODO maybe insert trivial bipartitions as well 

      result.initializeWithTaxon(p->number-1, _bipMeaning.at(p->number-1)); 

      if(withTrivial)
	{
	  addBipOccurrence(withBranch, result, _treesAdded, p);
	  ++addedSoFar;
	}

      return result;  
    }
  else 
    {
      auto desc = traln.getDescendents(curBranch);

      auto&& bipA = addElement(traln, traln.findNodePtr(desc.first.getInverted()), withBranch, withTrivial, addedSoFar); 
      auto&& bipB = addElement(traln, traln.findNodePtr(desc.second.getInverted()), withBranch, withTrivial, addedSoFar) ; 

      auto result = bipA | bipB; 

      addBipOccurrence(withBranch, result, _treesAdded,p ); 
      ++addedSoFar; 

      return result; 
    }
} 


std::vector<double>
BipartitionHash::getBranchLengths(const Bipartition& bip) const
{
  auto result = std::vector<double>{}; 
  auto iter = _bipBranchLengths.find(bip); 
  if(iter != _bipBranchLengths.end())
    result = iter->second; 
  return result;
}


Bipartition
BipartitionHash::getPresence(const Bipartition &bip) const 
{
  auto result = Bipartition{}; 
  auto iter = _bipPresence.find(bip); 
  if(iter != _bipPresence.end())
    result = iter->second; 
  return result;   
}

