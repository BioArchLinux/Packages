#include "ExtendedSprProposer.hpp"
#include "Path.hpp"
#include "TreeRandomizer.hpp"
#include "SprMove.hpp"

ExtendedSprProposer::ExtendedSprProposer(double stopProb )
  : _stopProb{stopProb}
  , _move{}
{
  assert(0. < _stopProb && _stopProb < 1.0 ); 
}


// this is bad legacy code ... 
void ExtendedSprProposer::determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params) 
{
  auto modifiedPath = Path{}; 

  assert(modifiedPath.size( ) == 0 ); 

  auto start = primeBranch; 
  auto p = traln.findNodePtr(start) ; 
  auto q = p->next->back; 
  auto r = p->next->next->back; 

  auto lengthR = traln.getBranch(r, params); 
  auto lengthQ = traln.getBranch(q, params); 
  traln.clipNode(q,r);
  // auto newBranch = lengthR; 
  // newBranch.setSecNode(q->number); 

  auto newBranch = BranchLengths(BranchPlain(  lengthR.getPrimNode(), q->number), lengthR.getLengths() ); 
  traln.setBranch(newBranch, params); 

  p->next->back = p->next->next->back = (nodeptr)NULL; 

  modifiedPath.append(start); 
  modifiedPath.append(BranchPlain(q->number, r->number)); 

  nodeptr currentNode = rand.drawRandDouble01() < 0.5  ? q : r; 
  boolean accepted = PLL_FALSE;   
  while(not accepted)
    {  
      nodeptr n = 
	rand.drawRandDouble01()  < 0.5 
	? currentNode->next->back
	: currentNode->next->next->back; 

      modifiedPath.pushToStackIfNovel(BranchPlain(currentNode->number, n->number),traln); 

      currentNode = n; 
      
      accepted = rand.drawRandDouble01() < _stopProb && modifiedPath.size() > 2 ; 	
    }

  /* undo changes to the tree  */
  traln.clipNode(p->next,q); 

  newBranch = BranchLengths(BranchPlain(lengthQ.getPrimNode(), p->number), lengthQ.getLengths()); 
  // newBranch = lengthQ ; 
  // newBranch.setSecNode(p->number); 
  traln.setBranch(newBranch, params);

  traln.clipNode(p->next->next,r);   
  newBranch = BranchLengths(BranchPlain(lengthR.getPrimNode(), p->number), lengthR.getLengths()); 
  // newBranch = lengthR; 
  // newBranch.setSecNode(p->number); 
  traln.setBranch(newBranch, params); 
  
  /* now correct  */
  if( modifiedPath.at(2).hasNode(modifiedPath.at(1).getPrimNode()))    
    modifiedPath.at(1).setSecNode(p->number); 
  else if(modifiedPath.at(2).hasNode(modifiedPath.at(1).getSecNode()  ))    
    modifiedPath.at(1).setPrimNode( p->number); 
  else 
    assert(0); 

  /* correct the incorrectly set first branch in the path */
  modifiedPath.at(0) = traln.getThirdBranch(modifiedPath.at(0), modifiedPath.at(1)); 
  
#ifdef DEBUG_ESPR
  cout << *modifiedPath << endl; 
#endif
  
  modifiedPath.debug_assertPathExists(traln); 

  // TODO in principle, we can throw away the path of this proposal and use the move proposal  
  // move.

  auto bla = modifiedPath.at(int(modifiedPath.size() - 1)); 

  _move = SprMove(traln,  BranchPlain(p->number, p->back->number), BranchPlain(bla.getPrimNode(), bla.getSecNode()) ); 

  _forwProb = log_double::fromAbs(1.); 

}
 
TopoMoveProposer* ExtendedSprProposer::clone() const
{
  return new ExtendedSprProposer(*this);  
}


BranchPlain ExtendedSprProposer::determinePrimeBranch(const TreeAln &traln, Randomness& rand) const
{
  auto  start = BranchPlain(); 
  nodeptr p,q,r; 
  do 
    {
      start = TreeRandomizer::drawBranchWithInnerNode(traln, rand); 
      
      p = traln.findNodePtr(start );
      q = p->next->back; 
      r = p->next->next->back;
    } while(traln.isTipNode(q) && traln.isTipNode(r) ); 
  return start; 
}


std::unique_ptr<TopoMove> ExtendedSprProposer::getMove() const   
{
  return std::unique_ptr<TopoMove>(_move.clone()); 
} 


void ExtendedSprProposer::determineBackProb(TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params) 
{ 
  _backProb = log_double::fromAbs(1.); 
}
