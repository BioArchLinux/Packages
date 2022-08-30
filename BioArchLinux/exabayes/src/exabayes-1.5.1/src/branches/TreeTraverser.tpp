#include "TreeTraverser.hpp"

#include "BranchLengthOptimizer.hpp"
#include "ParallelSetup.hpp"


template<class T> 
const int TreeTraverser<T>::_maxIter = 8; 



template<class T>
TreeTraverser<T>::TreeTraverser( int depth, TreeAln &traln, LikelihoodEvaluator& eval, std::vector<AbstractParameter*>  params, const BranchPlain &rootOfTraversed, const BranchPlain &prunedSubtree)
  : _doFirst{false}
  , _depth(depth)
  , _traln(traln)
  , _eval(eval)
  , _params{params}
  , _rootOfTraversed{rootOfTraversed}
  , _prunedSubtree{prunedSubtree}
{
#ifdef PRINT_LIKESPR_INFO
  tout << "created tree traverser for " << SHOW(prunedSubtree) << SHOW(rootOfTraversed) << std::endl; 
#endif
}


template<class T>
void TreeTraverser<T>::traverse()
{
  auto floatingBranch = BranchLengths{}; 
  auto branchAfterPrune = BranchPlain{};

  std::tie(floatingBranch, branchAfterPrune) = _traln.get().pruneSubtree(_prunedSubtree, _rootOfTraversed, _params); 

  testInsert(branchAfterPrune, floatingBranch, true, 0 ); 

  _traln.get().insertSubtree( _prunedSubtree, branchAfterPrune, floatingBranch, _params );
}


/** 
    insert, do stuff and remove again 
 */ 
template<class T>
void TreeTraverser<T>::testInsert( const BranchPlain &insertBranch, BranchLengths floatingBranch, bool isFirst, int curDepth)
{
  assert( isFirst || not _traln.get().isTipNode(insertBranch.getPrimNode())); 


  if(_depth < curDepth )
    return; 

  if(not isFirst || _doFirst )
    {
      // modify the floating branch, s.t it tells where to insert.  We
      // assume that primNode of insertBranch is the direction we are
      // coming from (while descending), so this is the point, where
      // we have to insert the branch length

      // floatingBranch.setPrimNode(_prunedSubtree.getPrimNode()); 
      // floatingBranch.setSecNode( insertBranch.getPrimNode() ); 
      
      floatingBranch = BranchLengths(BranchPlain(_prunedSubtree.getPrimNode(),insertBranch.getPrimNode() ), floatingBranch.getLengths()); 
      
      _traln.get().insertSubtree( _prunedSubtree,  insertBranch, floatingBranch, _params); 

      auto dirtyNodes = std::vector<nat>{ _prunedSubtree.getPrimNode() 
					  ,  insertBranch.getPrimNode()
      }; 

      for(auto v : dirtyNodes)
      	_eval.get().invalidateArray(_traln, v); 

      auto insert = _insertEval.getResult(_eval, _traln, insertBranch, _prunedSubtree, floatingBranch, _params);
      _result.push_back(insert);	

      auto floatAfterPrune = BranchLengths{}; 
      auto branchAfterPrune = BranchPlain{};

      std::tie( floatAfterPrune, branchAfterPrune) = _traln.get().pruneSubtree(_prunedSubtree, floatingBranch, _params); 
      
      // assure correct pruning 
      // not good anyway
      // auto lenA = floatAfterPrune.getLengths(); 
      // auto lenB = floatingBranch.getLengths(); 
      // for(auto i = 0u; i < lenA.size() ; ++i)
      // 	assert(lenA.at(i) == lenB.at(i)); 
    }

 if(not  _traln.get().isTipNode(insertBranch.getSecNode()) )
    {
      auto desc = _traln.get().getDescendents(insertBranch.getInverted()); 

      testInsert(desc.first, floatingBranch, false, curDepth+1 ); 
      testInsert(desc.second, floatingBranch, false, curDepth+1 ); 
    }
 
 if( not _traln.get().isTipNode(insertBranch.getPrimNode())  ) // isFirst
   {
     _eval.get().invalidateArray(_traln.get(), insertBranch.getPrimNode()); 
   }
}
