#ifndef _ABSTRACT_TOPO_MOVE_DRAWER_HPP 
#define _ABSTRACT_TOPO_MOVE_DRAWER_HPP 

#include <memory> 

#include "TreeAln.hpp"
#include "log_double.hpp"
#include "TopoMove.hpp"
#include "LikelihoodEvaluator.hpp"
#include "OptimizedParameter.hpp"


class Communicator; 

class TopoMoveProposer
{
public: 			// INHERITED METHODS  
  /** 
      determines the move and a probability of drawing this move     
   */ 
  virtual void determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params)  = 0; 
  /** 
      only determines the proposal density of the backward move (NOT including any further transformations like branch lengths) 
   */ 
  virtual void determineBackProb(TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params) = 0; 
  /**
     determines the first branch (e.g., subtree that is pruned)
   */ 
  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const = 0; 
  virtual TopoMoveProposer* clone() const = 0; 

  virtual std::unique_ptr<TopoMove> getMove() const  = 0; 

  virtual void printParams(std::ostream &out)  const {} 
  
public: 			// METHODS 
  TopoMoveProposer()
    : _forwProb{log_double::fromAbs(1.)}
    , _backProb{log_double::fromAbs(1.)}
    , _withOptimizedBranches{false}
    , _optBranches{}
  {
  }
  virtual ~TopoMoveProposer(){}
  TopoMoveProposer(const TopoMoveProposer& rhs) = default; 
  TopoMoveProposer(TopoMoveProposer&& rhs) = default ; 
  TopoMoveProposer& operator=(const TopoMoveProposer &rhs) = default; 
  TopoMoveProposer& operator=( TopoMoveProposer &&rhs) = default; 

  log_double getProposalDensity() const {return _backProb / _forwProb; }
  bool hasOptimizedBranches()  const  { return _withOptimizedBranches; } 
  std::vector<OptimizedParameter> getOptimizedBranches() const { return _optBranches;  } 

protected: 
  log_double _forwProb; 
  log_double _backProb; 
  bool _withOptimizedBranches; 
  std::vector<OptimizedParameter> _optBranches; 
}; 


#endif
