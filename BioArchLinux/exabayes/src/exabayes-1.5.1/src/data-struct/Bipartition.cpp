#include "Bipartition.hpp"

#include <iostream>
#include <limits>


// TODO at some point improve this  
size_t Bipartition::numBits = sizeof(nat) * 8; 
size_t Bipartition::numBitsMinusOne = numBits - 1 ; // not esthetic, but efficient 
size_t Bipartition::bitShift = 5; 
nat Bipartition::allOne = std::numeric_limits<nat>::max();
std::vector<nat> Bipartition::perBitMask = {
  1u, 
  1u <<  1, 
  1u <<  2,
  1u <<  3,
  1u <<  4,
  1u <<  5,
  1u <<  6,
  1u <<  7,
  1u <<  8,
  1u <<  9,
  1u <<  10,
  1u <<  11,
  1u <<  12,
  1u <<  13,
  1u <<  14,
  1u <<  15,
  1u <<  16,
  1u <<  17,
  1u <<  18,
  1u <<  19,
  1u <<  20,
  1u <<  21,
  1u <<  22,
  1u <<  23,
  1u <<  24,
  1u <<  25,
  1u <<  26,
  1u <<  27,
  1u <<  28,
  1u <<  29,
  1u <<  30,
  1u <<  31
}; 




static const char LogTable256[256] = 
{
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
  -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
  LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
  LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};



Bipartition::Bipartition()
  : bip(0)
  , hash(0)
{
  
}


bool Bipartition::operator==(const Bipartition &rhs) const
{
  // TODO ui ui ui that's inefficient...
  // size cannot be used at all here. And hash woudl be nice, but definitely makes trouble with operator& 
  

  // if( not ( getHash() == rhs.getHash() && bip.size() == rhs.bip.size()) )
  //   return false; 
  // else 
  //   {

  // auto longestLength = std::max(bip.size(), rhs.bip.size()); 
  auto shortestLength = std::min(bip.size(), rhs.bip.size()); 

  auto result = true; 
  for(nat i = 0; result && i < shortestLength; ++i)
    result &= ( bip.at(i) == rhs.bip.at(i)) ;  
  
  for(auto i = shortestLength; result && i < bip.size(); ++i)
    result &= ( bip.at(i) == 0 ) ; 
  
  for(auto i = shortestLength; result && i < rhs.bip.size() ;++i)
    result &= (rhs.bip.at(i) == 0); 
  
  return result; 
}


Bipartition Bipartition::operator| (const Bipartition &rhs) const
{
  auto result = Bipartition{}; 

  auto longestLength = std::max(bip.size() , rhs.bip.size()); 
  auto shorterLength = std::min(bip.size(), rhs.bip.size()); 
  
  result.bip.resize(longestLength, 0u); 
  // std::cout << "resizing to " << longestLength << std::endl; 
  // std::cout << "now: " << result << std::endl; 

  for(nat i = 0; i < shorterLength; ++i)
    result.bip[i] = bip[i] | rhs.bip[i]; 

  if( bip.size() < rhs.bip.size() )
    {
      for(auto i = shorterLength; i < longestLength; ++i)
	result.bip[i] = rhs.bip[i]; 
    }
  else if(rhs.bip.size() < bip.size())
    {
      for(auto i = shorterLength; i < longestLength; ++i)
	result.bip[i] = bip[i]; 
    }
  
  result.hash = hash ^ rhs.hash; 

  return result; 
}



void Bipartition::initializeWithTaxon(nat pos, nat ranNum)
{
  reserve(pos + 1); 
  set(pos); 
  hash = ranNum; 
} 



void Bipartition::reserve(size_t num)
{
  ++num ;
  auto elemsNeeded = num / numBits + ( num % numBits > 0 ? 1 : 0  ) ; 

  // auto initSize = bip.size(); 
  // bool didResize = false; 
  if( bip.size()  < elemsNeeded )
    {
      // didResize = true; 
      bip.resize(elemsNeeded , 0); // meh
    }
  // std::cout  << "have " << initSize << " needing " << elemsNeeded << (didResize ? " => RESIZE " : "")<< std::endl; 
}

void Bipartition::set(nat pos)
{
  bip[ pos >> bitShift ] |= perBitMask[ pos &  numBitsMinusOne ]; 
}


bool Bipartition::isSet(nat pos) const
{
  if(getElemsReserved() <= pos )
    return false; 
  else 
    return (bip[ pos >> bitShift ] & perBitMask[ pos & numBitsMinusOne ] ) != 0; 
}

// void Bipartition::unset(nat pos)
// {
//   bip[ pos  >> bitShift ] &= ( allOne & perBitMask[ pos & numBitsMinusOne ]) ; 
// } 


std::ostream& operator<<(std::ostream& out, const Bipartition& rhs)
{
  nat total = rhs.count(); 
  nat seen = 0;

  for(auto &elem : rhs.bip)
    {
      for(nat i = 0; i < Bipartition::numBits; ++i)
	{
	  if( elem & Bipartition::perBitMask[i] )
	    {
	      ++seen;
	      out << "1"; 
	      if(seen == total)
		break; 
	    }
	  else 
	    out << "0"; 
	}

      if(seen == total)
	break; 
    }
  
  return out;   
}


nat Bipartition::count() const 
{
  nat result = 0; 
  for(auto &elem : bip )
    result += __builtin_popcount(elem);

  return result; 
}


void Bipartition::printVerbose(std::ostream &out, const std::vector<std::string> nameMap) const
{
  auto numTax = nameMap.size() ; 
  bool printInverse = count() > numTax / 2;
  
  bool isFirst = true; 
  
  for(nat i = 0; i < numTax  ; ++i)
    {
      bool isOne =  isSet(i); 
      if( not printInverse != not isOne )
	{
	  out << (isFirst ? "" : "," ) << nameMap.at(i); 
	  isFirst = false; 
	}
    }
  
  assert(not isFirst);  
} 


bool Bipartition::isSubset(const Bipartition& rhs) const 
{			
  // TODO expensive 
  return ( *this & rhs ) == *this   ; 
}


Bipartition Bipartition::operator& (const Bipartition &rhs) const
{
  auto result = Bipartition{}; 
  auto shorterLength = std::min(bip.size(), rhs.bip.size()); 
  result.bip.resize(shorterLength, 0u); 

  for(nat i = 0; i < shorterLength; ++i)
    result.bip[i] = bip[i] & rhs.bip[i]; 

  // TODO reduce the hash accordingly 
  result.hash = 0; 

  return result;   
}


nat Bipartition::getHash() const
{
  if(hash == 0)
    {
      std::cout  << "hash of " << *this  << " was " << hash; 
      assert(hash != 0); 
    }
  return hash; 
}


Bipartition Bipartition::getComplement( nat maxElem) const 
{
  auto result = Bipartition{}; 
  result.reserve(maxElem); 

  for(nat i = 0; i < maxElem; ++i)
    {
      if(not  isSet(i))
	{
	  result.set(i); 
	}
    }

  return result; 
}


bool Bipartition::isCompatible(const Bipartition& rhs, nat maxElem) const
{
  auto intersect = *this & rhs; 
  if(intersect.count() == 0)
    return true; 
  
  intersect = this->getComplement(maxElem) & rhs; 
  if(intersect.count ()  == 0)
    return true; 
  
  intersect = *this & rhs.getComplement(maxElem); 
  if(intersect.count() == 0)
    return true;  
  
  return false; 
} 



nat Bipartition::findIndex() const 
{
  auto result = 0; 
  auto iter = begin(bip); 
  auto theEnd = end(bip); 
  while( iter != theEnd && *iter == 0  )
    {
      ++iter; 
      result += 32; 
    }

  assert(*iter != 0); 

  // this actually compute the logarithm, i.e., the most significant
  // bit
  
  unsigned int v = *iter; // 32-bit word to find the log of
  unsigned r;     // r will be lg(v)
  unsigned int t, tt; // temporaries
  
  if (  ( tt =  v >> 16    )  )
    r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
  else 
    r = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];

  return result + r; 
} 
