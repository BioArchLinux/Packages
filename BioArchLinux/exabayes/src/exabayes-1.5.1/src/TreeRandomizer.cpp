#include "TreeRandomizer.hpp"

#include <vector>
// #include "Branch.hpp"
#include "ParallelSetup.hpp"

#include "BranchPlain.hpp"


extern "C"
{				// 
  void newviewParsimonyIterativeFast(pllInstance *tr, partitionList * pr); 
  unsigned int evaluateParsimonyIterativeFast(pllInstance *tr, partitionList *pr); 
  void buildSimpleTree (pllInstance *tr, partitionList *pr, int ip, int iq, int ir); 
  void makePermutationFast(int *perm, int n, pllInstance *tr); 
  void computeTraversalInfoParsimony(nodeptr p, int *ti, int *counter, int maxTips, boolean full); 
}


// // // this is a quick-and-dirty adaptation for building a random stepwise
// // addition parsimony tree


void TreeRandomizer::stepwiseAddition(pllInstance *tr, partitionList *pr, nodeptr p, nodeptr q, ParallelSetup& pl)
{            
  nodeptr 
    r = q->back;

  unsigned int 
    mp;
  
  int 
    counter = 4;
  
  p->next->back = q;
  q->back = p->next;

  p->next->next->back = r;
  r->back = p->next->next;
   
  computeTraversalInfoParsimony(p, tr->ti, &counter, tr->mxtips, PLL_FALSE);              
  tr->ti[0] = counter;
  tr->ti[1] = p->number;
  tr->ti[2] = p->back->number;
    
  mp = evaluateParsimonyIterativeFast(tr, pr);

  auto result = std::vector<nat>{mp}; 
  // std::cout <<  "my score was " << mp << std::endl; 
  result = pl.getChainComm().allReduce(result);
  mp = result.at(0); 
  // std::cout << "after allreduce "<< mp << std::endl; 
  
  
  if(mp < tr->bestParsimony)
    {    
      tr->bestParsimony = mp;
      tr->insertNode = q;     
    }
 
  q->back = r;
  r->back = q;
   
  if(q->number > tr->mxtips
#if 0 
     && tr->parsimonyScore[q->number] > 0
#endif
     )
    {         
      stepwiseAddition(tr, pr, p, q->next->back, pl);
      stepwiseAddition(tr, pr, p, q->next->next->back, pl);
    }
}



void TreeRandomizer::createStepwiseAdditionParsimonyTree(TreeAln &traln, ParallelSetup& pl )
{   
  auto *tr = &(traln.getTrHandle()); 
  auto *pr = &(traln.getPartitionsHandle()); 

  assert(tr->fastParsimony == PLL_FALSE); 
    
  nodeptr  
    p, 
    f;    

  int 
    nextsp; 
  auto perm = std::vector<int>(tr->mxtips+1,  0); 
       
  assert(!tr->constrained);

  makePermutationFast(perm.data(), tr->mxtips, tr);
  
  tr->ntips = 0;    
  
  tr->nextnode = tr->mxtips + 1;       
  
  buildSimpleTree(tr, pr, perm[1], perm[2], perm[3]);
  
  f = tr->start;       
  
  while(tr->ntips < tr->mxtips) 
    {   
      nodeptr q;
      
      tr->bestParsimony = std::numeric_limits<nat>::max();
      nextsp = ++(tr->ntips);             
      p = tr->nodep[perm[nextsp]];                 
      q = tr->nodep[(tr->nextnode)++];
      p->back = q;
      q->back = p;
        
      if(tr->grouped)
        {
          int 
            number = p->back->number;            

          tr->constraintVector[number] = -9;
        }
          
      stepwiseAddition(tr, pr, q, f->back, pl);
      
      // std::cout <<  SyncOut() << "bestPars: "<< tr->bestParsimony << std::endl; 
      
      {
        nodeptr   
          r = tr->insertNode->back;
        
        int counter = 4;
        
        hookupDefault(q->next,       tr->insertNode);
        hookupDefault(q->next->next, r);
        
        computeTraversalInfoParsimony(q, tr->ti, &counter, tr->mxtips, PLL_FALSE);              
        tr->ti[0] = counter;
        
        newviewParsimonyIterativeFast(tr, pr);
      }
    }    
}


void TreeRandomizer::createParsimonyTree(TreeAln &traln, Randomness& rand, ParallelSetup& pl)
{
  nat r = rand();  

  traln.unlinkTree();
  traln.getTrHandle().randomNumberSeed = r; 

  createStepwiseAdditionParsimonyTree(traln, pl);
}


void TreeRandomizer::randomizeTree(TreeAln &traln, Randomness& rand )
{
  traln.unlinkTree();

  // start with the simple tree 
  auto a = traln.getNode(1 ),
    b = traln.getNode(2 ), 
    c = traln.getNode( 3 ), 
    inner = traln.getNode(traln.getNumberOfTaxa() + 1); 
  
  traln.clipNodeDefault(inner,a); 
  traln.clipNodeDefault(inner->next, b); 
  traln.clipNodeDefault(inner->next->next,c); 

  for(nat i = 4; i < traln.getNumberOfTaxa() +1 ;  ++i)
    {
      inner = traln.getNode(traln.getNumberOfTaxa() + i-2 );       
      auto taxonP = traln.getNode(i); 
      traln.clipNodeDefault(taxonP, inner); 

      auto p1 = traln.findNodePtr(drawBranchUniform_helper(traln, rand, i-1)),
	p2 = p1->back; 
      
      traln.clipNodeDefault(p1, inner->next); 
      traln.clipNodeDefault(p2, inner->next->next); 
    }
}


BranchPlain TreeRandomizer::drawInnerBranchUniform( const TreeAln& traln, Randomness &rand)  
{
  bool acc = false;   
  int aNode = 0; 
  nodeptr p = nullptr; 
  while(not acc)
    {      
      aNode = drawInnerNode(traln, rand); 
      p = traln.getNode(aNode); 
      
      nat numTips = 0; 
      if(  traln.isTipNode(p->back) ) 
	numTips++; 
      if(traln.isTipNode(p->next->back))
	numTips++;
      if(traln.isTipNode(p->next->next->back))
	numTips++; 
      
      assert(numTips != 3); 
      
      acc = numTips == 0 || rand.drawRandDouble01() <  (3. - double(numTips)) / 3.;       
    }
  assert(aNode != 0); 
  
  std::vector<nat> options; 
  if(not traln.isTipNode(p->back))
    options.push_back(p->back->number); 
  if(not traln.isTipNode(p->next->back))
    options.push_back(p->next->back->number); 
  if(not traln.isTipNode(p->next->next->back))
    options.push_back(p->next->next->back->number); 

  nat other = 0;
  if(options.size() == 1 )
    other = options[0]; 
  else 
    other = options.at(rand.drawIntegerOpen(int(options.size())));   
  return BranchPlain(aNode, other); 
}


nat TreeRandomizer::drawInnerNode(const TreeAln& traln, Randomness &rand )  
{    
  nat curNumTax = traln.getNumberOfTaxa(); 
  nat res =  1 + curNumTax + rand.drawIntegerClosed(curNumTax - 3 );   
  return res; 
}


/** 
    @brief draw a branch that has an inner node as primary node   
    
    => equals draw subtree uniformly
 */ 
BranchPlain TreeRandomizer::drawBranchWithInnerNode(const TreeAln& traln,Randomness &rand)  
{
  nat idA = drawInnerNode(traln, rand); 
  nat r = rand.drawIntegerClosed(2);  
  nodeptr p = traln.getNode(idA); 
  assert(not traln.isTipNode(p)) ; 

  BranchPlain b; 
  switch(r)
    {
    case 0: 
      b = BranchPlain(idA, p->back->number); 
      break; 
    case 1: 
      b = BranchPlain(idA, p->next->back->number); 
      break; 
    case 2: 
      b = BranchPlain(idA, p->next->next->back->number); 
      break; 
    default: assert(0); 
    }
 
  return b; 
}



BranchPlain TreeRandomizer::drawBranchUniform(const TreeAln & traln, Randomness &rand )  
{
  return drawBranchUniform_helper(traln, rand, traln.getNumberOfTaxa()); 
}

BranchPlain TreeRandomizer::drawBranchUniform_helper(const TreeAln &traln, Randomness &rand , nat curNumTax)  
{ 
  int randId = 0; 
  double r = rand.drawRandDouble01(); 

  // for the randomization part, i assume that when a tree is built
  // successively, I assume that the inner nodes start with an offset
  // in the nodeptr array that is the number of trees in the final
  // tree
  if( r <= 0.75) 		// draw an inner node 
    randId = 1 + traln.getTrHandle().mxtips + rand.drawIntegerClosed(curNumTax -3 )  ; 
  else 				// draw a tip 
    randId = rand.drawIntegerOpen(curNumTax) + 1 ;           

  auto p = traln.getNode(randId); 
  nat thisNode = randId,
    thatNode = 0;   
  if(traln.isTipNode(p))
    {
      thatNode = p->back->number; 
    }
  else 
    {
      switch(rand.drawIntegerOpen(3))
  	{
  	case 0 : 
  	  thatNode = p->back->number; 
  	  break; 
  	case 1 : 
  	  thatNode = p->next->back->number; 
  	  break; 
  	case 2: 
  	  thatNode = p->next->next->back->number; 
  	  break; 
  	default: assert(0); 
  	}
    }

  return BranchPlain(thisNode, thatNode); 
}


