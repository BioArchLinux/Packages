#ifndef _TREE_PRINTER_HPP
#define _TREE_PRINTER_HPP

#include "TreeAln.hpp"
#include <sstream>

class AbstractParameter; 


class TreePrinter
{
public: 
  TreePrinter(bool withBranchLengths , bool withInternalNodes , bool withRealNames) ; 
  /** 
      @brief prints the tree belonging to model
   */ 
  std::string printTree(const TreeAln& traln, const std::vector<AbstractParameter*> &params); 
  std::string printTree(const TreeAln& traln,  AbstractParameter* params); 
  std::string printTree(const TreeAln& traln); 
  
private: 			// METHODS
  void helper(const TreeAln &traln, std::stringstream &ss, nodeptr p, bool isFirst, const std::vector<AbstractParameter*> &params); 
  void printBranchLength(const TreeAln& traln, std::stringstream &ss, nodeptr p , const std::vector<AbstractParameter*> &params);

private: 			// ATTRIBUTES
  bool withBranchLengths; 
  bool withInternalNodes; 
  bool withRealNames; 
}; 
#endif
