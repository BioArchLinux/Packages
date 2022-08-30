#ifndef _TREE_RANDOMIZER_H
#define _TREE_RANDOMIZER_H

class Randomness; 
class ParallelSetup; 
// class TreeAln; 

#include "TreeAln.hpp"

class TreeRandomizer
{
public: 

  /**
     @brief creates a random tree 
   */   
  static void randomizeTree( TreeAln &traln, Randomness &rand);

  /**
     @brief draws a branch with uniform probability.
     
     We have to treat inner and outer branches separatedly.
  */
  static BranchPlain drawBranchUniform(const TreeAln & traln, Randomness &rand )  ; 
  /** 
      @brief samples an inner branch (including orientation), such that
      each oriented inner branch is equally likely.
  */ 
  static BranchPlain drawInnerBranchUniform( const TreeAln& traln, Randomness &rand)  ; 
  static BranchPlain drawBranchWithInnerNode(const TreeAln& traln, Randomness &rand)  ; 
  static nat drawInnerNode(const TreeAln& traln, Randomness &rand )  ; 

  /**
     @brief creates a parsimony tree and applies it to the specfilied tree.      
     the routine does not enforce treatment of branch lengths.  
   */ 
  static void createParsimonyTree(TreeAln &traln, Randomness& rand, ParallelSetup& pl); 

private: 
  /**
     @brief currently just exists for the tree randomizer 
     
     Notice that an inner branch has 3 nodes associated with it, thus
     the probability of drawing a tip should be accordingly lower.
  */ 
  static BranchPlain drawBranchUniform_helper(const TreeAln &traln, Randomness &rand , nat curNumTax)  ; 

  static void createStepwiseAdditionParsimonyTree(TreeAln &traln, ParallelSetup& pl ); 
  static void stepwiseAddition(pllInstance *tr, partitionList *pr, nodeptr p, nodeptr q, ParallelSetup& pl); 

}; 


#endif

