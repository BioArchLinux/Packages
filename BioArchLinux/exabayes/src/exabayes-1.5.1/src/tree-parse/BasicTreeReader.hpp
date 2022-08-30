#ifndef _BASIC_TREE_READER
#define _BASIC_TREE_READER

#include <string>
#include <vector>
#include <sstream>

#include "BranchLength.hpp"

#include "BranchLengthPolicy.hpp"
#include "LabelPolicy.hpp"
#include <iosfwd>
// #include "Branch.hpp"

typedef unsigned int nat; 

template<class LABEL_READER,class BL_READER>
class BasicTreeReader
{
public: 
  BasicTreeReader( nat numTax ); 
  std::vector<BranchLength> extractBranches(std::istream &iss) ; 
  void setLabelMap(std::unordered_map<std::string,nat> map) { lr.setLabelMap(map) ; }
  
private: 			// METHODS 
  double parseFloat(std::istream &iss); 
  std::tuple<nat,double> parseSubTree( std::istream &iss,std::vector<BranchLength> &branches, bool ) ; 
  std::tuple<nat,double> parseElement(std::istream &iss); 
  void expectChar(std::istream &iss,int ch); 
  void addBranch(nat label, std::tuple<nat,double> subtree, std::vector<BranchLength> &branches) const ; 
  
private: 
  nat _highestInner; 
  LABEL_READER lr; 
  BL_READER br; 
}; 

#endif
