#ifndef BIPARTITIONHASHNEW_H
#define BIPARTITIONHASHNEW_H

#include <unordered_map>

#include "TreeAln.hpp"
#include "Bipartition.hpp"


class BipartitionHash
{
public: 
  BipartitionHash(nat numTax); 
  ~BipartitionHash(){}
  void addTree(const TreeAln &traln, bool withBranch, bool withTrivial); 

  std::unordered_map<Bipartition, Bipartition>::const_iterator begin() const{return _bipPresence.begin(); }
  std::unordered_map<Bipartition, Bipartition>::const_iterator end() const{return _bipPresence.end(); }

  std::vector<double> getBranchLengths(const Bipartition& bip) const;
  Bipartition getPresence(const Bipartition &bip) const; 

  nat getTreesAdded() const {return _treesAdded; }

private: 			// METHODS
  Bipartition addElement(const TreeAln &traln, nodeptr p, bool withBranch, bool withTrivial, nat &addedSoFar); 
  void addBipOccurrence(bool withBranch, Bipartition& result, nat treesAdded, nodeptr p); 

private: 			// ATTRIBUTES
  std::unordered_map<Bipartition, Bipartition> _bipPresence; 
  std::unordered_map<Bipartition, std::vector<double> > _bipBranchLengths; 
  std::vector<nat> _bipMeaning; 
  nat _treesAdded; 
}; 


#endif
