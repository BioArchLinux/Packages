#include "InternalBranchLength.hpp"

#include <cmath>

InternalBranchLength InternalBranchLength::fromAbsolute(double absLen, double meanSubstRate)
{
  auto result = InternalBranchLength{}; 
  auto internal = exp( - (absLen  /  meanSubstRate) ) ;
  result._internalBranchLength = internal;
  return result;
}


double InternalBranchLength::toMeanSubstitutions(double meanSubstRate) const 
{
  return -log(_internalBranchLength) * meanSubstRate; 
}



void InternalBranchLength::deserialize( std::istream &in )
{
  _internalBranchLength = cRead<double>(in); 
}


void InternalBranchLength::serialize( std::ostream &out) const
{
  cWrite(out, _internalBranchLength); 
}   
