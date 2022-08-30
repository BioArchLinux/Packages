#ifndef _TREE_PROCESSOR_HPP
#define _TREE_PROCESSOR_HPP

#include <string>
#include "TreeAln.hpp"

class TreeProcessor
{
public: 
  TreeProcessor(std::vector<std::string> fileNames, bool expensiveCheck ); 
  TreeProcessor(TreeProcessor&& tp) ; 
  virtual ~TreeProcessor(){}
  TreeProcessor& operator=(TreeProcessor &&tp); 
  
  const std::vector<std::string> getTaxa() const {return _taxa; }

protected: 			// METHODS
  auto fillTaxaInfo(std::string fileName) -> std::vector<std::string>; 
  template<bool readBl>
  void nextTree(std::istream &treefile); 
  void skipTree(std::istream &iss); 
  static std::string trim(const std::string& str, const std::string& whitespace  = " \t"); 

protected: 			// ATTRIBUTES
  std::unique_ptr<TreeAln> _tralnPtr;
  std::vector<std::string> _fns; 
  std::vector<std::string> _taxa; 
}; 


#endif
