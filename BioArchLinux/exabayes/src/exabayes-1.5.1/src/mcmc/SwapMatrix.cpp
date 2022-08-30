#include "SwapMatrix.hpp"
#include "GlobalVariables.hpp"

#include <iostream>
#include <cassert>

SwapMatrix::SwapMatrix(size_t numChains)
  : matrix{}
  , numEntries(numChains)
{
  for(nat i = 0; i < numEntries; ++i)
    for(nat j = i  + 1 ; j < numEntries; ++j)
      matrix.push_back(SuccessCounter()); 
}


void SwapMatrix::update(nat a, nat b, bool acc)
{
  auto &elem = matrix.at(mapToIndex(a,b));
  if(acc)
    elem.accept();
  else 
    elem.reject();
}


const SuccessCounter& SwapMatrix::getCounter(nat a, nat b ) const
{
  return matrix.at(mapToIndex(a,b)); 
} 


size_t SwapMatrix::mapToIndex(nat a, nat b) const 
{
  assert(a != b);
  if(b < a )
    std::swap(a,b); 

  size_t result = 0; 
  for(nat i = 0; i < a && a != 0; ++i)
    result += numEntries -i  -1 ; 
  result += b - a - 1  ; 

  if(not (result < matrix.size()))
    {
      tout << "for " << a << "," << b << " the index was "
      	   << result << ", but matrix has only " << matrix.size() << std::endl; 
      
      assert(result < matrix.size()); 
    }

  return result; 
} 


std::ostream&  operator<<(std::ostream &out, const SwapMatrix& rhs ) 
{
  for(nat i = 0; i < rhs.numEntries-1; ++i)
    {
      tout << "(" ;
      bool isFirst = true;
      for(nat j = i+1; j < rhs.numEntries; ++j)
	{
	  auto &elem = rhs.matrix[rhs.mapToIndex(i,j)]; 
	  // out << (isFirst ? "" : ",") << std::setprecision(1) << 100 * elem.getRatioOverall() << "%"; 
	  out << ( isFirst ? "" : "," ) << "(" << elem << ")" ; 
	  isFirst = false; 
	}
      tout << ")"; 
    }
  return out ; 
}


void SwapMatrix::deserialize( std::istream &in )   
{  
  for(auto &s : matrix)
    s.deserialize(in);
} 

void SwapMatrix::serialize( std::ostream &out)  const
{
  for(auto &s : matrix)
    s.serialize(out); 
}


SwapMatrix SwapMatrix::operator+(const SwapMatrix& rhs) const
{
  // tout << "rhs was: " << rhs << std::endl;     

  auto result = SwapMatrix(numEntries); 

  for(nat i = 0; i < rhs.matrix.size(); ++i)
    result.matrix[i] = matrix[i] + rhs.matrix[i]; 

  // tout << "result is" << result << std::endl; 

  return result; 
}


SwapMatrix& SwapMatrix::operator+=(const SwapMatrix& rhs) 
{
  *this = *this + rhs; 
  return *this;
}


