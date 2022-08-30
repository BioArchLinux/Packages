/** 
    @file Path.hpp represents a  path in a tree
    
    Currently we are assuming, the path always is connected. 

    This is an ancient class; not entirely happy with it.

 */ 


#ifndef _PATH_H
#define _PATH_H

#include <vector>

#include "TreeAln.hpp"
#include "Randomness.hpp"
#include "PriorBelief.hpp"
// #include "Branch.hpp"


class Path
{
public:   
  Path()
    : stack(std::vector<BranchPlain>{} ) {} 
  virtual ~Path(){}

/** @brief returns true, if the node with a given id is part of this branch */ 
  bool nodeIsOnPath(int node) const;  

  /** @brief asserts that this path exists in a given tree */ 
  void debug_assertPathExists(TreeAln& traln); 

  /** @brief only add a branch to the path, if it is novel. If the new
      branch cancels out an existing branch, the path is shortened again */ 
  void pushToStackIfNovel(BranchPlain b, const TreeAln &traln ); 

  // straight-forward container methods 
  void append(BranchPlain value); 
  void clear(); 
  /** @brief number of branches in the path */ 
  size_t size() const {return stack.size(); }

  /** @brief yields the branch */  
  BranchPlain& at(size_t num){return stack[num]; }
  BranchPlain at(size_t num) const{return stack[num];}

  /** @brief reverse the path */ 
  void reverse(); 

  /** @brief removes the last element */ 
  void pop(); 
  /** @brief removes the first element */ 
  void popFront(); 

  /** @brief returns the id of the nth node in the path. nodes 0 and n+1 are the outer nodes in this path that do not have a neighbor. */ 
  nat getNthNodeInPath(size_t num) const ; 
  
  /** @brief gets the number of nodes represented by the path (assuming it is connected)  */
  size_t getNumberOfNodes() const {return stack.size()  + 1 ;   }

  void findPath(const TreeAln& traln, nodeptr p, nodeptr q);
  friend std::ostream& operator<<(std::ostream &out, const Path &rhs)  ;

private: 			// METHODS 
  bool findPathHelper(const TreeAln &traln, nodeptr p, const BranchPlain &target);

private: 			// ATTRIBUTES 
  std::vector<BranchPlain> stack; 

}; 

#endif
