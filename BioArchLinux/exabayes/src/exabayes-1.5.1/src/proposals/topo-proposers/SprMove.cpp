#include <array> 
#include <algorithm>
#include <cmath>

// #include "AdHocIntegrator.hpp"
#include "SprMove.hpp"
#include "Arithmetics.hpp"
#include "BoundsChecker.hpp"
#include "ParallelSetup.hpp"
#include "BranchLengthOptimizer.hpp"




SprMove::SprMove(const TreeAln &traln, const BranchPlain &prunedTree,  const BranchPlain &insertBranch) 
  : _path{}
{
  nodeptr p = traln.findNodePtr(prunedTree);
  
  _path.clear();   
  _path.findPath(traln, p, traln.findNodePtr(insertBranch));
  _path.reverse();   

  auto Tmp = _path.at(0); 

  nodeptr aNode = traln.findNodePtr(Tmp); 
  if(traln.isTipNode(aNode))
    aNode = traln.findNodePtr(Tmp.getInverted());
  assert(not traln.isTipNode(aNode)); 
  auto b = _path.at(0); 
  while(  (b == _path.at(0) || b.getInverted() == _path.at(0)) 
 // b.equalsUndirected( )
	  || // b.equalsUndirected(BranchPlain(p->number, p->back->number))
	  (b == BranchPlain(p->number, p->back->number) || b.getInverted() == BranchPlain(p->number, p->back->number))
	  ) 
    {
      aNode = aNode->next; 
      b = BranchPlain(aNode->number, aNode->back->number) ;
    }
  _path.reverse();
  _path.append(b);  
  _path.reverse();
}




void SprMove::apply(TreeAln &traln,const std::vector<AbstractParameter*> &params) const
{

  assert(_path.size() > 2 ); 

  auto third =  traln.getThirdBranch(_path.at(0), _path.at(1)); 
  nodeptr sTPtr = traln.findNodePtr(third); 
  assert(_path.at(1).hasNode(sTPtr->number )); 
  
  // finds the two nodeptrs adjacent to the subtree  
  auto prOuterPtr = traln.findNodePtr(BranchPlain(_path.getNthNodeInPath(0) ,_path.getNthNodeInPath(1)) ),
    prInnerPtr = traln.findNodePtr(BranchPlain(_path.getNthNodeInPath(2), _path.getNthNodeInPath(1)) ); 

  int lastNode = _path.getNthNodeInPath(  int(_path.getNumberOfNodes()-1 ) ),
    s2LastNode = _path.getNthNodeInPath( int(_path.getNumberOfNodes()-2 ) ); 
  nodeptr s2lPtr = traln.findNodePtr(BranchPlain(s2LastNode, lastNode) ), // s2l 
    lPtr = traln.findNodePtr(BranchPlain(lastNode, s2LastNode) ); // l

  nodeptr toBeInserted = prInnerPtr->back;   
  nodeptr toBeInsertedPath = prOuterPtr->back;  

  assert(toBeInserted->number == sTPtr->number && sTPtr->number == toBeInsertedPath->number); 

  /* prune sTPtr */
  traln.clipNode(prInnerPtr, prOuterPtr); 		 
  traln.setBranch(traln.getBranch(prOuterPtr, params), params); 
  sTPtr->next->back = sTPtr->next->next->back = (nodeptr) NULL; 
  
  // and hook up at the reattachment point, mapping the branches 
  traln.clipNode(toBeInsertedPath, lPtr);
  traln.setBranch(traln.getBranch(lPtr, params), params);

  traln.clipNode(toBeInserted, s2lPtr); 
  traln.setBranch(traln.getBranch(toBeInserted, params), params); 

  assert(traln.exists(BranchPlain(sTPtr->number, s2lPtr->number))); 
  assert(traln.exists(BranchPlain(sTPtr->number, lPtr->number) )); 
}




/* 
   this does not assume that the move already has been applied 
 */ 
BranchPlain SprMove::getMovingSubtree(const TreeAln &traln) const 
{
  return traln.getThirdBranch(_path.at(0), _path.at(1)); 
}


/*
    we assume that the move already has been applied 
 */ 
BranchPlain SprMove::getEvalBranch(const TreeAln &traln) const
{    
  auto b = traln.getThirdBranch(_path.at(0), _path.at(1)); 
  auto lastBranch = _path.at( int(_path.size()-1 )); 

  auto bA = BranchPlain(b.getPrimNode(), lastBranch.getPrimNode()), 
    bB = BranchPlain(b.getPrimNode(),lastBranch.getSecNode()); 

  assert(traln.exists(bA) && traln.exists(bB)); 

  auto futureRoot = traln.getThirdBranch(bA, bB ); 
  
  return futureRoot; 
}





std::ostream& operator<<(std::ostream &out, const SprMove& rhs)
{
  return out << rhs._path;   
} 





SprMove SprMove::getInverseMove() const
{
  if( _path.size() > 0 )
    { 
      auto result = *this; 
      auto newPath = Path{}; 

      newPath.append(BranchPlain(result._path.getNthNodeInPath(0) , result._path.getNthNodeInPath(2) ));
      for(nat i = 2; i < result._path.size()-1 ; ++i)
	newPath.append(result._path.at(i));

      newPath.append(BranchPlain(result._path.getNthNodeInPath(result._path.getNumberOfNodes()-2), 
				    result._path.getNthNodeInPath(1))); 
      newPath.append(BranchPlain(result._path.getNthNodeInPath(result._path.getNumberOfNodes()-1), 
				    result._path.getNthNodeInPath(1))); 
      newPath.reverse();
      
      result._path = newPath; 
      
      return result; 
    }
  else 
    return SprMove{}; 
}


std::vector<nat> SprMove::getDirtyNodes() const 
{
  auto result = std::vector<nat>{}; 
  for(auto i = 1u; i < _path.getNumberOfNodes() -1 ; ++i)
    result.push_back(_path.getNthNodeInPath(i)) ;

  return result; 
} 


template<typename T1,typename T2>
std::ostream& operator<<(std::ostream& out, const std::pair<T1,T2> &elem)
{
  out << elem.first << "," << elem.second ; 
  return out; 
}


std::unique_ptr<TopoMove> SprMove::getInverse() const 
{
  // auto result = new SprMove();
  // *result = getInverseMove();
  return std::move(std::unique_ptr<TopoMove>(new SprMove(getInverseMove()))); 
} 


TopoMove* SprMove::clone() const
{
  return new SprMove(*this);
}


void SprMove::print(std::ostream& out)  const 
{
  out << *this; 
} 



// 0: only the "inner branches of the move"(1 for NNI, 2 for SPR-2 )
std::unordered_set<BranchPlain> SprMove::getBranchesByDistance(const TreeAln &traln, nat dist) const 
{
  auto result = std::unordered_set<BranchPlain>{}; 
  if(dist == 0)
    {
      for(nat i = 1; i < _path.size() -1 ; ++i)
	result.insert(_path.at(i));
    }
  else 
    {
      auto prevBranches = getBranchesByDistance(traln, dist-1); 
      
      auto additionalBranches = std::unordered_set<BranchPlain>{}; 

      for(auto &v : prevBranches)
	{
	  if(not traln.isTipNode( v.getPrimNode()))
	    {
	      auto desc = traln.getDescendents(v); 
	      additionalBranches.insert(desc.first);
	      additionalBranches.insert(desc.second);
	    }
	  
	  auto vI = v.getInverted();
	  if( not traln.isTipNode( vI.getPrimNode()) )
	    {
	      auto desc =  traln.getDescendents(vI);
	      additionalBranches.insert(desc.first); 
	      additionalBranches.insert(desc.second); 
	    }
	}

      
      result = prevBranches; 
      result.insert(begin(additionalBranches), end(additionalBranches)); 
    }
  
  return result; 
}






std::vector<BranchPlain> SprMove::getBranchesToPropose(const TreeAln& traln, MoveOptMode mode )
{
  auto result = std::vector<BranchPlain>{}; 

  if( _path.size() == 0)
    return result; 

  switch(mode)
    {
    case MoveOptMode::ALL_SURROUNDING:
      {
	result.push_back(_path.at(0)); 
	for(auto i = 1u; i < _path.size(); ++i)
	  {
	    result.push_back(_path.at(i)); 
	    result.push_back(traln.getThirdBranch(_path.at(i-1), _path.at(i))); 
	  }
      }
      break; 
    case MoveOptMode::ALL_INTERNAL: 
      for(nat i = 1; i < _path.size()-1 ; ++i)
	result.push_back( _path.at(i));
      break; 
    case MoveOptMode::ONLY_SWITCHING: 
      result.push_back(_path.at(1)); 
      break; 
    case MoveOptMode::NONE: 
      break; 
    case MoveOptMode::ALL_IN_MOVE: 
      for (nat i = 0; i < _path.size(); ++i) 
	result.push_back(_path.at(i)); 
      break; 
    default: 
      assert(0); 
    }

  for(auto &elem : result )
    assert(traln.exists(elem)); 
    
  assert(std::unordered_set<BranchPlain>(begin(result), end(result)).size() == result.size() ); 
  return result; 
}




std::vector<nat> SprMove::getNodeList() const 
{
  auto result = std::vector<nat>{}; 
  for(nat i = 0; i <_path.size() + 1 ; ++i )
    result.push_back(_path.getNthNodeInPath(i));
  return result; 
}


void SprMove::invalidateArrays(LikelihoodEvaluator& eval,  TreeAln& traln, MoveOptMode mode)  const 
{
  for(auto n : getDirtyNodes())
    eval.invalidateArray(traln, n); 
} 




static bool isSubset(const std::vector<nat> &listA, const std::vector<nat> &listB)
{
  auto len = std::min(listB.size(), listA.size()); 
  for(nat i = 0; i < len; ++i)
    {
      auto res = (listA[i] == listB[i]) ; 
      if (not res  )
	return false; 
    }

  return true; 
}



std::vector<nat> SprMove::getDirtyWrtPrevious(const SprMove &rhs) const 
{
  // damn, this is inefficient here....dont have more time ...

  auto result = std::vector<nat>{}; 
  auto hisDirty =  rhs.getDirtyNodes(); 
  auto myDirty =  getDirtyNodes();

  auto aIter = begin(hisDirty); 
  auto bIter = begin(myDirty); 

  // tout << SHOW(hisDirty)<< SHOW(myDirty) << std::endl; 
  
  auto lastCommon = -1; 
  while(aIter != end(hisDirty) && bIter != end(myDirty) && *aIter == *bIter)  
    {
      lastCommon = *aIter; 
      ++aIter;
      ++bIter; 
    }

  while(aIter != end(hisDirty))
    {
      result.push_back(*aIter);
      ++aIter; 
    }
  while(bIter != end(myDirty))
    {
      result.push_back(*bIter); 
      ++bIter; 
    }

  // tout << SHOW(lastCommon)<< std::endl; 

  if(lastCommon != -1 && not isSubset(rhs.getNodeList() , getNodeList()) ) // that would be a descent 
    result.push_back(lastCommon ) ; 
  
  return result; 
}
