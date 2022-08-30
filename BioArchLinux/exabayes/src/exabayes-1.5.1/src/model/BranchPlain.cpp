#include "BranchPlain.hpp"



nat getCommonNode(const BranchPlain &oneBranch, const BranchPlain &otherBranch) 
{
  // assert(isAdjacent(oneBranch, otherBranch)); 
  if(not isAdjacent(oneBranch, otherBranch))
    {
      std::cout <<  SHOW(oneBranch) << SHOW(otherBranch) << std::endl; 
      assert(0); 
    }
  

  if(oneBranch.getPrimNode() == otherBranch.getPrimNode()
     || oneBranch.getPrimNode() == otherBranch.getSecNode()) 
    return oneBranch.getPrimNode(); 
  else if(oneBranch.getSecNode() == otherBranch.getPrimNode()
     || oneBranch.getSecNode() == otherBranch.getSecNode())
    return oneBranch.getSecNode();
  else 
    // return oneBranch.getSecNode(); 
    {
      assert(0); 
      return oneBranch.getPrimNode();
    }
} 

bool isAdjacent(const BranchPlain &oneBranch, const BranchPlain& otherBranch)
{
  return oneBranch.getPrimNode() == otherBranch.getPrimNode()
    || oneBranch.getPrimNode() == otherBranch.getSecNode()
    || oneBranch.getSecNode() == otherBranch.getPrimNode() 
    || oneBranch.getSecNode() == otherBranch.getSecNode(); 
} 



void BranchPlain::deserialize( std::istream &in )   
{
  _primNode = cRead<decltype(_primNode)>(in); 
  _secNode = cRead<decltype(_secNode)>(in); 
}

 
void BranchPlain::serialize( std::ostream &out) const
{
  cWrite(out, _primNode);
  cWrite(out, _secNode);
}
