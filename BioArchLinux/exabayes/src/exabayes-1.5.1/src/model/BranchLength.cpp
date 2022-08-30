#include "BranchLength.hpp"

void BranchLength::deserialize( std::istream &in )  
{
  BranchPlain::deserialize(in);
  _length.deserialize(in); 
}

 
void BranchLength::serialize( std::ostream &out) const
{
  BranchPlain::serialize(out); 
  _length.serialize(out); 
}  
