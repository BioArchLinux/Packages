/**
   @brief represents a tree and the associated alignment 

 */

#ifndef _TREEALN_H
#define _TREEALN_H

#include "extensions.hpp"
#include "ArrayReservoir.hpp"
#include "GlobalVariables.hpp"

#include "ProtModel.hpp"
#include "BranchPlain.hpp"
#include "FlagType.hpp"

#include "BranchLengthResource.hpp"
#include "Partition.hpp"

#include <vector>
#include <iostream>
#include <array>
#include <memory>

class BranchLength; 
class BranchLengths; 
class AbstractPrior; 
class Randomness; 
class TreePrinter; 
class AbstractParameter; 
class ByteFile; 

class TreeAln
{
  friend class BranchLengthResource;  

public: 
   /////////////////
   // life cycle  //
   /////////////////
  TreeAln(size_t numTax, bool isSaveMemorySEV);
  TreeAln(const TreeAln& rhs);
  TreeAln(TreeAln&& rhs) = default; 
  ~TreeAln(){}
  TreeAln& operator=(TreeAln rhs);

  friend void swap(TreeAln& lhs, TreeAln& rhs ); 

  friend std::ostream& operator<< (std::ostream& out,  const TreeAln&  traln);

  /////////////////////////////////////
  //           OBSERVERS             //
  /////////////////////////////////////
  Partition& getPartition(nat model) ;
  const Partition& getPartition(nat model)  const ; 
  /** 
      @brief get the internal raxml tree representation 
   */ 
  pllInstance& getTrHandle() {return _tr;}
  const pllInstance& getTrHandle() const {return _tr; }
  /** 
      @brief frees all likelihood arrays 
   */   
  void clearMemory(ArrayReservoir &arrayReservoir);
  void clearMemory(); 
  /** 
      @brief get the number of branches in the tree (not counting per-partition branch lengths)
   */ 
  nat getNumberOfBranches() const {return getNumberOfNodes() - 1 ; }
  /** 
      @brief get the number of partitions in the alignment
   */ 
  size_t getNumberOfPartitions() const;   
  /** 
      @brief get the number of taxa in the tree 
   */ 
  nat getNumberOfTaxa() const {return getTrHandle().mxtips; }
  /** 
      @brief get the number of nodes in the unrooted tree 
   */ 

  nat getNumberOfNodes() const { nat numTax = getNumberOfTaxa() ;  return 2 * numTax - 2 ;  } // excluding the virtual root 
  /** 
      @brief get the substitution matrix for partition "model"
   */ 
  std::vector<double> getRevMat(nat model, bool isRaw) const ;   
  /** 
      @brief gets the state frequencies for partition "model" 
   */ 
  std::vector<double> getFrequencies(nat model) const; 
  /**
     @brief gets the alpha parameter of the gamma distribution  
   */ 
  double getAlpha(nat model) const 
  {  
    auto &p = getPartition(model);
    return p.getAlpha();
  } 

  BranchPlain getThirdBranch(const BranchPlain& oneBranch, const BranchPlain& otherBranch) const ; 

  // not so happy with that...
  log_double getLikelihood() const 
  {
    return log_double::fromLog(getTrHandle().likelihood); 
  }

  void setLikelihood(log_double val)
  {
    getTrHandle().likelihood = val.getRawLog();
  }


  /** 
      @brief indicates whether a nodepointer is a tip 
   */ 
  bool isTipNode(nodeptr p) const {return p->number <=  getTrHandle().mxtips ;}
  bool isTipNode(nat aNode) const { return int(aNode) <=  getTrHandle().mxtips; }
  /** 
      @brief gets the branch from a node pointer (including branch length)
      
      @notice if only one parameter is provided, the resulting branch
      will ONLY contain the branch length for this one parameter
      (bogus parameters for other branch lengths, if you have
      per-partition branch lengths). For setting this branch, the same
      parameter must be used (and only this one).
      
      If you call this function with an existing Branch b, this
      function is usefull to get the actual branch length.
   */ 
  BranchLength getBranch(const BranchPlain &branch,  const AbstractParameter *param) const; 
  BranchLengths getBranch(const BranchPlain& branch, const std::vector<AbstractParameter*> &params) const ; 

  BranchLength getBranch(const nodeptr &branch,  const AbstractParameter *param) const; 
  BranchLengths getBranch(const nodeptr &branch, const std::vector<AbstractParameter*> &params) const ; 

  // improved interface for topological moves  
  void insertSubtree(const BranchPlain &subtree, const BranchPlain& insertBranch, const BranchLengths &branchToCreate, const std::vector<AbstractParameter*> &params); 
  auto  pruneSubtree(const BranchPlain &subtree, const BranchPlain &prunedBranch, const std::vector<AbstractParameter*> &params)
    -> std::tuple<BranchLengths, BranchPlain>; 

  /** 
      @brief gets a nodepointer with specified id 
   */ 
  nodeptr getNode(nat elem) const ; 
  
  std::vector<BranchPlain> getBranchesFromNode(nat aNode) const; 
  /** 
      @brief extract all branches from the tree (including branch lengths)
   */ 
  // template<typename RESULT>
  std::vector<BranchLength> extractBranches( const AbstractParameter* param ) const; 
  std::vector<BranchLengths> extractBranches( const std::vector<AbstractParameter*> &param ) const;  
  std::vector<BranchPlain> extractBranches() const; 
  /** 
      @brief gets the number of inner nodes in the tree 
   */ 
  nat getNumberOfInnerNodes(bool rooted) const; 

  /** 
      @brief gets the mean substitution rate overall specified partitions
   */ 
  // double getMeanSubstitutionRate(const std::vector<nat> &partitions) const ;
  /** 
      @brief unlinks a node 
   */ 
  void unlinkNode(nodeptr p); 
  /** 
      @brief gets the three nodes adjacent to the given node  
   */ 
  std::vector<nat> getNeighborsOfNode( nat node ) const ; 
  /** 
      @brief prunes the node from the tree 
   */ 
  void detachNode(nodeptr p); 
  ///////////////
  // MODIFIERS //
  ///////////////
  partitionList& getPartitionsHandle() ; 
  const partitionList& getPartitionsHandle() const ;

  void setPartitions(const std::vector<Partition> &p, bool initial);

  BranchPlain getAnyBranch() const ;
  
  /**
     @brief sets the frequencies. Format is important, frequencies must add up to 1.0 
  */ 
  void setFrequencies(const std::vector<double> &values, nat model);
  /** 
      @brief sets the parameters. Format is important, last rate must be 1.0  
  */ 
  void setRevMat(const std::vector<double> &values, nat model, bool isRaw);
  /** 
      @brief sets the alpha for partition "model"
   */ 
  void setAlpha(double alpha,  nat model);   
  /** 
      @brief sets a branch. Topology is NOT modified! 
   */ 
  void setBranch(const BranchLengths& b, const std::vector<AbstractParameter*> params);   
  void setBranch(const BranchLength& branch, const AbstractParameter* param); 
  /** 
      @brief hooks up two nodes. Branch lengths must be set
      separately.
   */ 
  void clipNode(nodeptr p, nodeptr q, double *z = nullptr);   

  bool exists(const BranchPlain &branch )const ; 
  
  /** 
      @brief hooks up two nodes with default branch length
   */ 
  void clipNodeDefault(nodeptr p, nodeptr q); 
  /**
     @brief resets/destroys the topology in the tree 
   */ 
  void unlinkTree();
  /** 
      @brief gets the maximum length of paths below this branch 
   */ 
  nat getDepth(const BranchPlain  &b) const ; 
  /** 
      @brief gets the longest path 
   */ 
  std::vector<nat> getLongestPathBelowBranch(const BranchPlain &b) const ; 


  bool isTipBranch(const BranchPlain &branch ) const ; 

  nodeptr findNodePtr(const BranchPlain &branch ) const ; 

  /**
     @brief gets a node with given id that is not connected to the tree right now 
   */ 
  nodeptr getUnhookedNode(int number);
  ///////////////
  // observers //
  ///////////////
  /** 
      @brief gets the branches that are below one branch 

      This function is handy for traversing the tree and relying less
      on raw pointers. For traversing, it is often necessary to invert
      the resulting branch.      
   */
  std::pair<BranchPlain,BranchPlain> getDescendents(const BranchPlain &b) const; 

  std::vector<bool> getExecModel() const ; 
  std::vector<log_double> getPartitionLnls() const; 
  void setPartitionLnls(const std::vector<log_double> partitionLnls) ; 
  void setExecModel(const std::vector<bool>  &modelInfo); 

  nat getNumberOfAssignedSites(nat model) const ; 

  const std::vector<std::string>& getTaxa() const {return _taxa; }
  void setTaxa(std::vector<std::string> taxa){_taxa = taxa; }

  std::vector<BranchPlain> getBranchesByDistance(const BranchPlain& branch, nat distance, bool bothSides ) const;   

  void setProteinModel(int part, ProtModel model) ; 
  ProtModel getProteinModel(int part) const; 
  void setBranchUnchecked(const BranchLength &bl); 
  void copyTopologyAndBl(const TreeAln &rhs); 
  void setBranchLengthResource(BranchLengthResource bls); 
  
  bool isSaveMemorySEV() const { return _isSaveMemorySEV; } 

  void createCaterpillar(); 

  void initRevMat(nat model); 	// these functions are not needed any more: directly use the respective setter function     
  void discretizeGamma(nat model); 
  void initialize(size_t numTax);
  nat addNodeToPartialTree(nat id, nat curRoot, nat outerCtr); 

    /////////////////////
   // rooted topology //
  /////////////////////
  bool isRooted(void) const;
  void setRootBranch(const BranchPlain &rb);
  BranchPlain getRootBranch() const;
  bool isRootChild(const nat nodeId) const;
  bool isRootBranch(const BranchPlain &rb) const;

private: 			// ATTRIBUTES 
  std::vector<Partition> _partitions;
  partitionList _partitionListResource; 
  std::vector<pInfo*> _partitionPtrs;	

  pllInstance _tr;
  std::vector<std::string> _taxa; 
  BranchLengthResource _bls; 

  // tree resources 
  std::vector<traversalInfo>  _traversalInfo; 
  std::vector<node> _nodes; 
  std::vector<nodeptr> _nodeptrs;

  std::vector<int> _ti; 
  std::vector<boolean> _execModel; // for the traveral descriptor 
  std::vector<nat> _parsimonyScore; 

  bool _isSaveMemorySEV;
  // in case we are working with a rooted tree
  BranchPlain _root;

};  

#endif
