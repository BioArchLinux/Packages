#ifndef _EXTENDED_TBR_DETERMINER_HPP
#define _EXTENDED_TBR_DETERMINER_HPP

#include "TbrMove.hpp"
#include "TopoMoveProposer.hpp"

class Path;

class ExtendedTbrProposer : public TopoMoveProposer
{

public:				// INHERITED METHODS 
  virtual void determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params)  ; 
  virtual TopoMoveProposer* clone() const ; 
  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const ; 
  virtual void determineBackProb(TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params); 
  std::unique_ptr<TopoMove> getMove() const   ; 

  virtual void printParams(std::ostream &out)  const { out << ";stopProb=" << _stopProb  ;} 

public: 			// METHODS 
  ExtendedTbrProposer(double stopProb )
    : _stopProb{stopProb}
    , _move{}
  {}
  
private: 			// METHODS
  void buildPath(Path &path, BranchPlain bisectedBranch, TreeAln &traln, Randomness &rand, std::vector<AbstractParameter*> params ) const; 

private: 			// ATTRIBUTES
  double _stopProb; 
  TbrMove _move; 
}; 



#endif
