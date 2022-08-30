#include "ParsSprProposer.hpp"

#include "Randomness.hpp"
#include "TreeRandomizer.hpp"

#include "SprMove.hpp"

double ParsSprProposer::weightEps = 1e-100; 


ParsSprProposer::ParsSprProposer(double parsWarp, int depth, Communicator& comm )
  : _parsWarp{parsWarp}
  , _depth{depth}
  , _pEval{}
  , _comm(comm)
  , _computedInsertions{}
  , _move{}
{
}



// this is legacy code, could be implemented in a nicer way...

static double state2factor(nat states)
{
  static double typicalBL = 0.05; 
  return log((1.0/states) - exp(-(states/(states-1) * typicalBL)) / states);
}


std::array<double,2> ParsSprProposer::factors = 
  {
    {
      state2factor(4),		// DNA
      state2factor(20)		// AA
    }
  }; 


ParsSprProposer::branch2parsScore
ParsSprProposer::determineScoresOfInsertions(TreeAln& traln, BranchPlain prunedTree  )  
{
  auto result = branch2parsScore{}; 

  nodeptr
    p = traln.findNodePtr(prunedTree), 
    pn = p->next->back , 
    pnn = p->next->next->back ; 

  // prune the subtree 
  traln.clipNode( pn, pnn); 
  p->next->back = p->next->next->back = NULL; 

  // fetch all parsimony scores   
  if(not traln.isTipNode(pn)) 
    {
      testInsertParsimony(traln, pn->next->back, p, result, _depth);
      testInsertParsimony(traln, pn->next->next->back, p, result, _depth); 
    }
  if(not traln.isTipNode(pnn))
    {
      testInsertParsimony(traln, pnn->next->back,p, result, _depth); 
      testInsertParsimony(traln, pnn->next->next->back,p, result, _depth); 
    }

  traln.clipNode( p->next, pn ); 
  traln.clipNode( p->next->next, pnn); 

  return result; 
}



void ParsSprProposer::testInsertParsimony(TreeAln &traln, nodeptr insertPos, nodeptr prunedTree, branch2parsScore &result, int curDepth)  
{
  if(curDepth == 0 )
    return; 
  --curDepth; 

  nodeptr insertBack =  insertPos->back;   
  traln.clipNode(insertPos, prunedTree->next);
  traln.clipNode( insertBack, prunedTree->next->next); 
  
  auto b = BranchPlain(insertPos->number, insertBack->number); 

  if(_computedInsertions.size() > 0 && _computedInsertions.find(b) != _computedInsertions.end()) 
    {
      result[b] = _computedInsertions.at(b); 
    }
  else 
    {
      ParsimonyEvaluator::disorientNode(prunedTree); 
      result[b] =  _pEval.evaluate(traln, prunedTree, false  ); 
    }

  traln.clipNode(insertPos, insertBack); 
  prunedTree->next->back = prunedTree->next->next->back = NULL; 

  // recursively descend 
  if(not traln.isTipNode(insertPos))
    {
      testInsertParsimony(traln, insertPos->next->back, prunedTree, result, curDepth); 
      testInsertParsimony(traln, insertPos->next->next->back, prunedTree, result, curDepth); 
    }
}

void ParsSprProposer::determineMove(TreeAln &traln, LikelihoodEvaluator &eval, Randomness& rand, BranchPlain primeBranch, const std::vector<AbstractParameter*> &params)  
{
  // auto result = new SprMove{}; 
  auto blParams = params; 
  auto prunedTree = primeBranch; 

  _pEval = ParsimonyEvaluator{} ; 

  _pEval.evaluate(traln, traln.findNodePtr(prunedTree), true );

  // decide upon an spr move 
  _computedInsertions = branch2parsScore {}; 
  auto forwInsertions  = determineScoresOfInsertions(traln, prunedTree);
  auto weightedInsertions = getWeights(traln, forwInsertions); 

  auto r = rand.drawRandDouble01(); 
  auto chosen = std::pair<BranchPlain,double>{}; 
  for(auto &v : weightedInsertions)
    {
      if(r < v.second)
	{
	  chosen = v; 
	  break; 
	}
      else 
	r -= v.second; 
    }

  // determine the branch, we pruned from 
  auto desc = traln.getDescendents(prunedTree);
  auto prunedFromBranch = BranchPlain{std::get<0>(desc).getSecNode() , std::get<1>(desc).getSecNode()}; 

  // important: save the move 
  _move = SprMove(traln, prunedTree,chosen.first );
  _forwProb = log_double::fromAbs(std::get<1>(chosen));
} 


void ParsSprProposer::determineBackProb( TreeAln &traln, LikelihoodEvaluator &eval, const std::vector<AbstractParameter*> &params )
{
  // assumes the move has alread been applied!!! 

  auto prunedTree = _move.getEvalBranch(traln); 
  auto prunedFromBranch = _move.getInverseMove().getInsertBranch();

  ParsimonyEvaluator::disorientNode( traln.findNodePtr(prunedTree)); 
  _pEval.evaluateSubtree(traln, traln.findNodePtr(prunedTree));

  auto backScores = determineScoresOfInsertions(traln, prunedTree); 
  auto weightedInsertionsBack = getWeights(traln, backScores); 
  
  assert(weightedInsertionsBack.find(prunedFromBranch) != end(weightedInsertionsBack)); 

  // RESULT 
  _backProb = log_double::fromAbs(weightedInsertionsBack[prunedFromBranch]); 
} 

TopoMoveProposer* ParsSprProposer::clone() const  
{
  return new ParsSprProposer(*this); 
}


BranchPlain ParsSprProposer::determinePrimeBranch(const TreeAln &traln, Randomness& rand) const 
{
  auto prunedTree = BranchPlain();
  nodeptr p, pn, pnn;  
  do 
    {
      prunedTree  = TreeRandomizer::drawBranchWithInnerNode(traln,rand); 
      p = traln.findNodePtr(prunedTree);
      pn = p->next->back; 
      pnn = p->next->next->back;         

    } while( (traln.isTipNode(pn) &&  traln.isTipNode(pnn))     ); 

  return prunedTree; 
} 



ParsSprProposer::weightMap ParsSprProposer::getWeights(const TreeAln& traln, branch2parsScore insertions)   
{
  auto result = weightMap{}; 
  double minWeight = std::numeric_limits<double>::max(); 

  auto data = std::vector<parsimonyNumber>{}; 
  data.reserve(insertions.size()  * 2 );
  for(auto &pair : insertions)
    {
      auto &values = std::get<1>(pair); 
      for(auto& v : values)
	data.push_back(v);
    }

  data = _comm.get().allReduce<>(data);

  nat ctr = 0; 
  for(auto &pair :insertions)
    {
      auto &values = std::get<1>(pair); 
      for(auto &v : values)
	v = data[ctr++]; 
    }

  for(auto &elem : insertions)
    {
      double score = 0; 

      auto& scoreArray= std::get<1>(elem); 
      for(nat i = 0 ; i < scoreArray.size() ; ++i)
	score += - ( factors[i]  * _parsWarp) * scoreArray[i]; 

      if(score < minWeight)
	minWeight = score; 

      result[std::get<0>(elem)] = score; 
    }

  // converting back to non-log
  double sum = 0; 
  for(auto & elem : result)
    {
      double normalizedWeight =exp(minWeight - elem.second)  + weightEps; 
      sum += normalizedWeight; 
      elem.second = normalizedWeight; 
    }

  for(auto &elem : result)
    std::get<1>(elem) /= sum; 

  return result; 
}



std::unique_ptr<TopoMove> ParsSprProposer::getMove() const   
{
  return std::unique_ptr<TopoMove>(_move.clone()); 
} 
