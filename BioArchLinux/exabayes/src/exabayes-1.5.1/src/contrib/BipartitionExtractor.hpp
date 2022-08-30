#ifndef BIPARTITION_EXTRACTOR_HPP
#define BIPARTITION_EXTRACTOR_HPP

#include <unordered_map>

#include "TreeProcessor.hpp"	
#include "BipartitionHash.hpp"

class BipartitionExtractor : public TreeProcessor
{
public: 
  BipartitionExtractor(std::vector<std::string> files,bool extractToOneHash, bool expensiveCheck);
  BipartitionExtractor( BipartitionExtractor&& rhs) = delete ; 
  BipartitionExtractor& operator=(BipartitionExtractor rhs) = delete ; 
  virtual ~BipartitionExtractor(){}

  template<bool readBL>
  void extractBips(nat burnin ); 
  void printBipartitions(std::string id) const ;
  void printBipartitionStatistics(std::string id) const ; 
  void printFileNames(std::string id) const ; 
  void printBranchLengths(std::string id) const ; 

  const std::vector<BipartitionHash>& getBipartitionHashes() const {return _bipHashes; } 

  std::string bipartitionsToTreeString( std::vector<Bipartition> bips, bool printSupport, bool printBranchLengths, bool phylipStyle) const ; 

  nat getNumTreesInFile(std::string file) const ; 

private: 
  void extractUniqueBipartitions() ;
  void buildTreeRecursive(nat currentId, const std::vector<std::vector<nat> >& directSubBips, const std::vector<Bipartition> &bips, std::stringstream &result, bool printSupport, bool printBranchLengths, bool phylipStyle) const; 


private: 			// ATTRIBUTES
  std::vector<BipartitionHash> _bipHashes; 
  std::unordered_map<Bipartition,nat> _uniqueBips; 
  bool _extractToOneHash; 
}; 


#endif
