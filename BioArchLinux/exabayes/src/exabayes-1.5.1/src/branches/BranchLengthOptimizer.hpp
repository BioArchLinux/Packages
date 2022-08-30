#ifndef _BRANCH_LENGTH_OPT_HPP 
#define  _BRANCH_LENGTH_OPT_HPP 

#include <vector> 
#include <functional>

#include "AbstractParameter.hpp"
#include "Communicator.hpp"

class TreeAln; 

#include "OptimizedParameter.hpp"


class BranchLengthOptimizer
{
public: 
  BranchLengthOptimizer(TreeAln& traln, const BranchPlain& branch, int maxIter, Communicator &comm, const std::vector<AbstractParameter*> &blParams ); 
  void applyToTraversalDescriptor(std::vector<bool> &execModel, TreeAln& traln) const ; 
  bool hasConvergedAll()  const ; 
  /** 
      @brief this function is essentially makenewzGeneric
  */ 
  void  optimizeBranches(TreeAln& traln)  ; 
  std::vector<OptimizedParameter> getOptimizedParameters() const { return _optParams; } 

private: 
  std::reference_wrapper<Communicator> _comm; 
  const std::vector<AbstractParameter*> _blParams; 
  std::vector<OptimizedParameter> _optParams; 
  BranchPlain _branch; 
  std::vector<BranchLength> _origBranches; 
};  

#endif
