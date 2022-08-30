// #include "Branch.hpp"
#include "TbrMove.hpp"
#include "Path.hpp"

#include <set>


TbrMove::TbrMove(const TreeAln &traln, const BranchPlain &bisected, const BranchPlain& insertA, const BranchPlain& insertB)  
  : _moveA{}
  , _moveB{}
{
  auto empty = BranchPlain{}; 

  _moveA = 
    // not insertA.equalsUndirected(empty) 
    not (insertA == empty || insertA.getInverted() == empty)
    ? SprMove(traln, bisected,insertA)
    : SprMove(); 
  
  _moveB = 
    not 
    // insertB.equalsUndirected(empty) 
    (insertB == empty || insertB.getInverted() == empty ) 
    ? SprMove(traln, bisected.getInverted(),insertB)
    : SprMove();
}


void TbrMove::apply(TreeAln &traln, const std::vector<AbstractParameter*> &blParams) const 
{
  assert(_moveA.getNniDistance() > 0 || _moveB.getNniDistance() > 0 ); 

  if( _moveA.getNniDistance() > 0)
    _moveA.apply(traln, blParams);
  if( _moveB.getNniDistance() > 0)
    _moveB.apply(traln,blParams); 
} 



TbrMove TbrMove::getInverseMove() const 
{
  auto result = *this; 

  result._moveA = result._moveA.getInverseMove();
  result._moveB = result._moveB.getInverseMove();

  return result; 
}

BranchPlain TbrMove::getEvalBranch(const TreeAln &traln) const 
{   
  auto evalBranch = BranchPlain{}; 

  if( _moveA.getNniDistance() >  0 )
    evalBranch = _moveA.getEvalBranch(traln);
  else if( _moveB.getNniDistance() > 0 )
    evalBranch = _moveB.getEvalBranch(traln);
  else 
    assert(0); 

  return evalBranch; 
}


std::ostream& operator<<(std::ostream &out, const TbrMove &rhs) 
{
  return out <<  "path1:"  << rhs._moveA << std::endl
	     << "path2:" << rhs._moveB << std::endl; // 
}  


std::vector<nat> TbrMove::getDirtyNodes() const
{
  auto tmp = std::unordered_set<nat>{}; 
  for(auto val : _moveA.getDirtyNodes())
    tmp.insert(val); 
  for(auto val : _moveB.getDirtyNodes())
    tmp.insert(val); 

  return std::vector<nat>(begin(tmp),end(tmp)); 
} 



std::unique_ptr<TopoMove> TbrMove::getInverse() const  
{
  return std::unique_ptr<TopoMove>(new TbrMove(getInverseMove()));
} 


TopoMove* TbrMove::clone() const  
{
  return new TbrMove(*this); 
}

 
void TbrMove::print(std::ostream& out) const 
{
  out << *this; 
} 


std::vector<BranchPlain> TbrMove::getBranchesToPropose(const TreeAln& traln, MoveOptMode mode) 
{
  auto result = std::vector<BranchPlain>{}; 

  auto nniA = _moveA.getNniDistance(); 
  auto nniB = _moveB.getNniDistance(); 
  auto trueTbr = nniA > 0 && nniB > 0; 

  auto res1 = _moveA.getBranchesToPropose(traln,mode); 
  auto res2 = _moveB.getBranchesToPropose(traln,mode); 
  
  result.insert(end(result), begin(res1), end(res1));
  result.insert(end(result), begin(res2), end(res2));



  if(trueTbr)
    {
      switch(mode)
	{
	case MoveOptMode::ALL_SURROUNDING: 
	  // the bisected branch has been inserted twice
	  {
	    auto tmp = std::unordered_set<BranchPlain>(begin(result), end(result)); 
	    result = std::vector<BranchPlain>(begin(tmp),end(tmp));
	  }
	  break; 
	case MoveOptMode::ALL_IN_MOVE:
	case MoveOptMode::ONLY_SWITCHING: 
	case MoveOptMode::ALL_INTERNAL: 
	  // also add the bisected branch 
	  {
	    auto a = _moveA.getMovingSubtree(traln); 
	    auto b = _moveB.getMovingSubtree(traln); 
	    assert( a == b || a.getInverted() == b ); 
	    result.push_back(b);
	  }
	  break; 
	case MoveOptMode::NONE: 
	  break; 
	default :
	  assert(0); 
	}
    }

  for(auto &elem : result )
    assert(traln.exists(elem)); 
    
  assert(std::unordered_set<BranchPlain>(begin(result), end(result)).size() == result.size() ); 

  return result; 
} 



void TbrMove::invalidateArrays(LikelihoodEvaluator& eval, TreeAln& traln, MoveOptMode mode)  const 
{
  _moveA.invalidateArrays(eval, traln, mode); 
  _moveB.invalidateArrays(eval, traln, mode); 
}
