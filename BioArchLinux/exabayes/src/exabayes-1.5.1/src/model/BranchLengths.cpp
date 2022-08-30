#include "BranchLengths.hpp"

void BranchLengths::deserialize( std::istream &in )
{
  BranchPlain::deserialize(in); 
  for(auto &v : _lengths)
    v.deserialize(in); 
}

 
void BranchLengths::serialize( std::ostream &out) const
{
  BranchPlain::serialize( out); 
  for(auto &v : _lengths)
    v.serialize(out); 
} 
