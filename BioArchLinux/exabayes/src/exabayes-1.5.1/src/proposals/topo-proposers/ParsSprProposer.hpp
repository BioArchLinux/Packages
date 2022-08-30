#ifndef _PARS_SPR_DETERMINER_HPP
#define _PARS_SPR_DETERMINER_HPP

#include "TopoMoveProposer.hpp"
#include "SprMove.hpp"
#include "ParsimonyEvaluator.hpp"
#include "Communicator.hpp"

class ParsSprProposer : public  TopoMoveProposer
{
  typedef std::unordered_map<BranchPlain, double> weightMap; 
  typedef std::unordered_map<BranchPlain,std::array<parsimonyNumber,3> > branch2parsScore;

public: 			// INHERITED METHODS
  virtual void determineMove(TreeAln &traln, LikelihoodEvaluator& eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params)  ; 
  virtual TopoMoveProposer* clone() const ; 
  virtual BranchPlain determinePrimeBranch(const TreeAln &traln, Randomness& rand) const ; 
  virtual void determineBackProb( TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params ); 

  virtual std::unique_ptr<TopoMove> getMove() const ; 

  virtual void printParams(std::ostream &out)  const { out << ";radius=" << _depth << ",warp=" << _parsWarp ;} 

public: 			// METHODS
  ParsSprProposer(double parsWarp, int depth, Communicator &comm );

  virtual  ~ParsSprProposer() {} 
  ParsSprProposer(const ParsSprProposer& rhs)
    : TopoMoveProposer(rhs)
    , _parsWarp{rhs._parsWarp}
    , _depth{rhs._depth}
    , _pEval{rhs._pEval}
    , _comm{rhs._comm}
    , _computedInsertions{rhs._computedInsertions}
    , _move{rhs._move}
  {
  }


  ParsSprProposer(ParsSprProposer&& rhs) = default; 
  ParsSprProposer& operator=(const ParsSprProposer &rhs)  = default; 
  ParsSprProposer& operator=( ParsSprProposer &&rhs)  = default ; 
  
private: 			// METHODS 
  branch2parsScore determineScoresOfInsertions(TreeAln& traln, BranchPlain prunedTree  )  ;
  void testInsertParsimony(TreeAln &traln, nodeptr insertPos, nodeptr prunedTree, branch2parsScore &result, int curDepth) ; 
  auto  getWeights(const TreeAln& traln, branch2parsScore insertions)  -> weightMap; 

private: 			// ATTRIBUTES
  double _parsWarp; 
  int _depth; 
  ParsimonyEvaluator _pEval; 
  std::reference_wrapper<Communicator> _comm; 
  branch2parsScore _computedInsertions; 
  SprMove _move; 

  static std::array<double,2> factors; 
  static double weightEps; 	// mostly a numerical addition
}; 


#endif
