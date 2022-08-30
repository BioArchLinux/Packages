#include "NodeAge.hpp"


void NodeAge::deserialize( std::istream &in )  
{
  BranchPlain::deserialize(in); 
  _height = cRead<double>(in); 
}


void NodeAge::serialize( std::ostream &out) const
{
  BranchPlain::serialize(out); 
  cWrite(out, _height); 
}   

