#include "ExtendedTbrProposer.hpp" 

#include "Path.hpp"
#include "TreeRandomizer.hpp"

#include "TbrMove.hpp"


void ExtendedTbrProposer::buildPath(Path &path, BranchPlain bisectedBranch, TreeAln &traln, Randomness &rand, std::vector<AbstractParameter*> params ) const 
{
  double stopProb = _stopProb; 

  nodeptr p= traln.findNodePtr(bisectedBranch ); 
  path.clear();

  nodeptr pn = p->next->back,
    pnn = p->next->next->back; 
  
  
  auto pnBranch = traln.getBranch(pn, params),
    pnnBranch = traln.getBranch(pnn, params);

  // prune  
  traln.clipNodeDefault(pn, pnn); 
  p->next->next->back = p->next->back = NULL; 

  path.append(bisectedBranch);
  path.append(BranchPlain(pn->number, pnn->number)); 
  nodeptr currentNode = rand.drawRandDouble01() < 0.5  ? pn : pnn;
  bool accepted = false; 
  while(not accepted )
    {
      nodeptr n = 
	rand.drawRandDouble01()  < 0.5 
	? currentNode->next->back
	: currentNode->next->next->back; 

      path.pushToStackIfNovel(BranchPlain(currentNode->number, n->number),traln); 
      currentNode = n;       
      accepted = rand.drawRandDouble01() < stopProb && path.size() > 2 ; 
    }

  // reset
  traln.clipNode(p->next, pn); 
  traln.setBranch(pnBranch, params);
  traln.clipNode(p->next->next, pnn); 
  traln.setBranch(pnnBranch, params);

  // a correction is necessary 
  if(path.at(2).hasNode(path.at(1).getPrimNode() ))
    path.at(1).setSecNode(p->number); 
  else if (path.at(2).hasNode(path.at(1).getSecNode() ))
    path.at(1).setPrimNode(p->number); 
  else 
    assert(0); 

  // for reasons of resetting the first branch in the path must be
  // identifyable later
  path.at(0) = traln.getThirdBranch(bisectedBranch, path.at(1)); 
}


void ExtendedTbrProposer::determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params) 
{
  auto modifiedPath1 = Path{}; 
  auto modifiedPath2 = Path{}; 

  auto bisectedBranch = primeBranch; 

  // determine, if a true TBR move can be executed
  auto canMove = [&](const BranchPlain &b) -> bool 
    { 
      if(traln.isTipNode(b.getPrimNode()))
	return false; 
      else 
	{
	  auto desc = traln.getDescendents(b) ; 
	  return not(traln.isTipBranch(desc.first) && traln.isTipBranch(desc.second)) ; 
	}
    }; 
  auto oneMovable = canMove(bisectedBranch);
  auto otherMovable = canMove(bisectedBranch.getInverted()); 

  auto descOne = BranchPlain{}; 
  if(oneMovable)
    {
      buildPath(modifiedPath1, bisectedBranch, traln, rand, params); 
      descOne = modifiedPath1.at( int(modifiedPath1.size() -1) ); 
    }

  auto descOther = BranchPlain{}; 
  if(otherMovable)
    {
      
      buildPath(modifiedPath2, bisectedBranch.getInverted(), traln, rand, params); 
      descOther = modifiedPath2.at(int(modifiedPath2.size()-1)); 
    }

  _forwProb = log_double::fromAbs(1.);
  _move = TbrMove(traln, bisectedBranch, descOne, descOther) ; 
}


TopoMoveProposer* ExtendedTbrProposer::clone() const 
{
  return new ExtendedTbrProposer(*this);
}

BranchPlain ExtendedTbrProposer::determinePrimeBranch(const TreeAln &traln, Randomness& rand) const
{
  auto canMove = [&](const BranchPlain &b) -> bool 
    { 
      auto result = traln.isTipBranch(b); 
      if( not result)
  	{
  	  auto desc = traln.getDescendents(b) ; 
  	  result = not(traln.isTipBranch(desc.first) && traln.isTipBranch(desc.second) ); 
  	  // tout << b << " with children " << std::get<0>(desc) << "," << std::get<1>(desc)<< std::endl ; 
  	}
      return result; 
    }; 

  auto bisectedBranch = BranchPlain{}; 
  auto movableA = bool{false}; 
  auto movableB = bool{false}; 

  while(not ( movableA || movableB ) )
    {
      bisectedBranch = TreeRandomizer::drawBranchWithInnerNode(traln,rand); 
      movableA = canMove(bisectedBranch); 
      movableB = canMove(bisectedBranch.getInverted()); 
    }

  return bisectedBranch; 
}



std::unique_ptr<TopoMove> ExtendedTbrProposer::getMove() const   
{
  return std::unique_ptr<TopoMove>(_move.clone()); 
} 


void ExtendedTbrProposer::determineBackProb(TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params)
{
  _backProb = log_double::fromAbs(1.); 
} 
