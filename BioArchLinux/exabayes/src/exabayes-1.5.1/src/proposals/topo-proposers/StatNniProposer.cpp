#include "StatNniProposer.hpp"
#include "TreeRandomizer.hpp"
#include "SprMove.hpp"

void StatNniProposer::determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params) 
{
  auto b = traln.getBranch( primeBranch, params); 
  nodeptr p = traln.findNodePtr(b); 
  auto switchingBranch = BranchPlain( rand.drawRandDouble01() < 0.5  
				      ? p->back->next->back->number
				      : p->back->next->next->back->number, 
				      p->back->number ); 
  
  _move = SprMove(traln, b, switchingBranch); 
  _forwProb = log_double::fromAbs(1.); 
} 


TopoMoveProposer* StatNniProposer::clone() const  
{
  return new StatNniProposer(*this);
}


BranchPlain StatNniProposer::determinePrimeBranch(const TreeAln &traln, Randomness& rand) const 
{
  return TreeRandomizer::drawInnerBranchUniform(traln, rand);   
} 


std::unique_ptr<TopoMove> StatNniProposer::getMove() const   
{
  return std::unique_ptr<TopoMove>(_move.clone()); 
} 


void StatNniProposer::determineBackProb(TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params) 
{
  _backProb = log_double::fromAbs(1.);
} 
