#ifndef _SWAP_ELEM
#define _SWAP_ELEM

#include <iosfwd>
#include "common.h" 


class ParallelSetup; 


class SwapElem
{
public: 
  SwapElem(nat gen, nat idA, nat idB, double r); 

  nat getGen() const;
  bool affectsChain(nat id ) const; 
  nat getOne() const ;
  nat getOther() const ; 
  double getR()const ;

  nat getMyId(ParallelSetup& pl, nat runid) const ; 
  nat getRemoteId(ParallelSetup& pl, nat runid) const ; 
  
  bool onlyOneIsMine(ParallelSetup& pl, nat runid) const ; 
  bool bothAreMine(ParallelSetup& pl, nat runid) const ; 

  friend bool operator==(const SwapElem &elemA, const SwapElem &elemB ) ; 
  friend std::ostream& operator<<(std::ostream& out, const SwapElem &elem); 
  
private:
  nat _gen; 
  nat _idA; 
  nat _idB; 
  double _r; 
}; 

#endif
