#include "TopoMoveProposer.hpp"

#include "SprMove.hpp"

class ExtendedSprProposer : public TopoMoveProposer
{
public: 			// inherited methods 
  virtual void determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params) ; 
  virtual TopoMoveProposer* clone() const; 
  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const; 
  virtual void determineBackProb (TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params) ;
  virtual std::unique_ptr<TopoMove> getMove() const   ; 

  virtual void printParams(std::ostream &out)  const { out << ";stopProb=" << _stopProb  ;} 
public: 			// methods 
  ExtendedSprProposer(double stopProb );

private: 
  double _stopProb; 
  SprMove _move; 
}; 
