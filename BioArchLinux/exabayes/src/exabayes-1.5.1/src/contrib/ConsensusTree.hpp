#ifndef CONSENSUSTREE_HPP
#define CONSENSUSTREE_HPP

#include "BipartitionExtractor.hpp"
#include <vector>
#include <string>

class ConsensusTree 
{
public: 
  ConsensusTree(std::vector<std::string> files, double burnin, double threshold, bool isMRE  ); 
  std::string getConsensusTreeString(bool printNames) const ;  
  std::vector<Bipartition>  getRefinedConsensusTree(const std::vector<Bipartition> &consensusBips, const std::vector<Bipartition> &minorityBips) const ; 
  std::string getTreeHeader() const; 
  std::string getType() const ; 

private:   
  BipartitionExtractor bipEx; 
  double _threshold; 
  bool _isMRE; 
}; 


#endif
