#include <iostream>
#include <fstream>

#include "Path.hpp"
#include "Chain.hpp"
#include "AbstractProposal.hpp"


void Path::clear()
{
  stack.clear(); 
}

void Path::append(BranchPlain value)
{
  stack.push_back(value);
}



void Path::pop()
{
  stack.pop_back();
}

void Path::popFront()
{
  stack.erase(stack.begin()); 
}


void Path::pushToStackIfNovel(BranchPlain b, const TreeAln &traln)
{  
  assert(stack.size() >= 2 ); 

  auto bPrev = at(size()-1); 
  if(stack.size() == 2)
    {
      // if(not b.equalsUndirected(bPrev))
      if(bPrev != b && bPrev.getInverted() != b)
	append(b); 
    }
  else 
    {      
      // if(b.equalsUndirected(bPrev))
      if(b == bPrev || b.getInverted() == bPrev)
	stack.pop_back();
      else if(traln.isTipBranch(bPrev))
	{
	  stack.pop_back();
	  append(b); 
	}
      else 
	append(b ); 
    }
}




void Path::debug_assertPathExists(TreeAln& traln)
{
#ifdef DEBUG_CHECK_TREE_CONSISTENCY
  tree *tr = traln.getTr();
  for(auto b : stack)
    assert(branchExists(tr, b)); 
#endif
}


bool Path::nodeIsOnPath(int aNode) const
{
  for(auto b : stack)
    if(b.hasNode(aNode))
      return true;       

  return false; 
}


std::ostream& operator<<(std::ostream &out, const Path &rhs)  
{
  for(auto b : rhs.stack)
    out << "(" << b.getPrimNode() << "," << b.getSecNode() << "),";       
  return out; 
}


nat Path::getNthNodeInPath(size_t num)   const
{ 
  int result; 
  
  assert(num <= stack.size( )+  1 ); 
  // TODO stronger warrenty when constructing 
  // assert(stack.size() != 1 ); 
  
  if(num == 0)
    {
      auto  b = stack[0]; 
      if(stack[1].hasNode(b.getPrimNode()))
	result= b.getSecNode(); 
      else 
	{
	  assert(stack[1].hasNode(b.getSecNode())); 
	  result= b.getPrimNode() ; 
	}
    }
  else if(num == stack.size() )
    {
      auto b = stack[num-1]; 
      
      if(stack[num-2].hasNode(b.getPrimNode() ))
	result=  b.getSecNode(); 
      else 
	{
	  assert(stack[num-2].hasNode(b.getSecNode() )); 
	  result= b.getPrimNode(); 
	}
    }
  else 
    {      
      auto b = stack[num]; 
      if(stack[num-1].hasNode(b.getPrimNode() ))
	result= b.getPrimNode(); 
      else 
	{
	  assert(stack[num-1].hasNode(b.getSecNode() )); 
	  result= b.getSecNode(); 
	}	
    }

  return result; 
}


void Path::reverse()
{
  std::reverse(stack.begin(), stack.end());
}




bool Path::findPathHelper(const TreeAln &traln, nodeptr p, const BranchPlain &target)
{
  auto  curBranch = BranchPlain(p->number, p->back->number); 
  // if( curBranch.equalsUndirected( target) ) 
  if( curBranch == target || curBranch.getInverted() == target)
    return true; 
  
  bool found = false; 
  for(auto q = p->next ; p != q && not found ; q = q->next)
    {
      found = findPathHelper(traln, q->back, target); 
      if(found)	
	{
	  append(BranchPlain(q->number, q->back->number));
	  return found; 
	}
    }
  return false; 
}



 void Path::findPath(const TreeAln& traln, nodeptr p, nodeptr q)
{  
  // cout << "trying to find path between " << p->number << "/" << p->back->number << " and "  << q->number << "/" << q->back->number  << endl; 
  
  stack.clear();   

  auto targetBranch = BranchPlain(q->number, q->back->number ); 

  bool found = findPathHelper(traln, p->back, targetBranch); 
  if(found)  
    {      
      append(BranchPlain(p->number, p->back->number)); 
      return ; 
    }

  assert(stack.size() == 0); 
  
  found = findPathHelper(traln, p->next->back, targetBranch); 
  if(found  )
    {
      append(BranchPlain(p->next->number, p->next->back->number)); 
      return; 
    }

  assert(stack.size() == 0);   
  found = findPathHelper(traln, p->next->next->back, targetBranch); 
  if(found)
    append(BranchPlain(p->next->next->number, p->next->next->back->number)); 
  assert(found);
}
