#include "ArrayOrientation.hpp"
#include <cassert>

#include "common.h"


const nat ArrayOrientation::maxSprSearch = 4; 


ArrayOrientation::ArrayOrientation(const TreeAln &traln)
  : orientation(traln.getNumberOfPartitions())
{
  for(nat i = 0 ; i < traln.getNumberOfPartitions(); ++i)
    orientation[i] = std::vector<nat>(traln.getNumberOfInnerNodes(false),  INVALID); 
}


nat ArrayOrientation::nodeNum2ArrayNum(nat nodeNum, nat numTax)
{
  auto res = int(nodeNum) - int(numTax) -1; 
  assert(res >= 0 ); 
  return  res;  
}



bool ArrayOrientation::searchNodeInSubtree(const TreeAln &traln, nodeptr p, nat nodeId, nat depth ) const 
{
  if( maxSprSearch < depth   )
    return false; 

  if(nat(p->number) ==  nodeId)
    return true; 

  if(traln.isTipNode(p))
    return false; 

  if(searchNodeInSubtree(traln, p->next->back, nodeId, depth + 1 ))
    return true; 

  if(searchNodeInSubtree(traln, p->next->next->back ,nodeId, depth + 1 ))
    return true; 

  return false; 
}



/** 
    checks whether the array associated with a node is still correctly
    oriented.

    the fact that this function is that complex was not intentional,
    but oversight. However, this should limit the damage effectively.
 */ 
bool ArrayOrientation::isCorrectNew( const TreeAln& traln, nat part, nodeptr p) const
{
  assert(not traln.isTipNode(p));

  auto numTax = traln.getNumberOfTaxa();
  auto id = nodeNum2ArrayNum(p->number, numTax); 
  auto theOrient = orientation.at(part).at(id); 

  // tout << "node=" << p->number << " is oriented " << SHOW(theOrient) << std::endl; 

  if(theOrient == INVALID)
    return false; 

  // maybe everything is as expected 
  auto correct = (theOrient == nat(p->back->number)); 

  // check, if a subtree was inserted (by an spr move) just between
  // this node and the correct array that this array is oriented
  // towards
  if(
#ifdef DANGEROUS_LNL_SHORTCUT_OFF
     false && 
#endif
     not correct && not traln.isTipNode(p->back->number))
    {
      auto numA = (nat) p->back->next->back->number; 
      auto numB = (nat) p->back->next->next->back->number; 

      correct |= ( theOrient == numA) ; 
      correct |= ( theOrient == numB) ; 

// #ifdef PR      
//       if(correct ) 
// 	tout << "CORRECT!\t" << SHOW(p->back->number) << std::endl; 
    }
  
  if( 
#ifdef DANGEROUS_LNL_SHORTCUT_OFF
     false && 
#endif
      not correct 
     )
    {
      // check, for topological rearrangement 

      // dont need to check p->back
      auto topoRearrangement = ( nat(p->next->back->number) != theOrient)  && ( nat(p->next->next->back->number) != theOrient ); 
      
      // if this is the case, we have to search the entire subtree to
      // check, whether after insertion, we are still oriented towards
      // it.

      // this should not have to be done more than 2 per generation
      if(topoRearrangement)
	{
	  correct |= searchNodeInSubtree(traln, p->back, theOrient, 0);
#ifdef LNL_PRINT_DEBUG
	  tout << "\n\nSEARCHING!!!\t" << (correct ? "FOUND" : "not found")  << "\n\n"; 
#endif
	}
    }

  return correct ; 
}


void ArrayOrientation::setOrientationForAllPartitions(nat id ,nat value)  
{
  for(nat i= 0; i < orientation.size() ; ++i)
    orientation[i][id] = value; 
}



void ArrayOrientation::setPartitionInvalid(nat part)  
{
  for(auto &elem  : orientation[part])
    elem = INVALID; 
} 


std::ostream& operator<<(std::ostream& out, const ArrayOrientation &rhs)
{
  nat ctr = 0; 
  for(auto &elem : rhs.orientation)
    {
      out << "[" << ctr << "] " ; 
      
      auto ctr2 = rhs.orientation[0].size() + 3 ;
      for(auto &subelem : elem ) 
	{
	  if(subelem == 0)
	    out << ctr2 <<  "->INVALID, " ; 
	  else 
	    out << ctr2 << "->" << subelem << ", " ;
	  ++ctr2; 
	}
      ++ctr ; 
      out << std::endl; 
    }
  return out; 
}


void ArrayOrientation::setInvalid(nat part, nat id)
{
  orientation[part][id] = INVALID; 
} 


nat ArrayOrientation::getOrientation(nat part, nat id ) const 
{
  auto value = orientation.at(part).at(id); 
  return value; 
}

