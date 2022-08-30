#ifndef _SWAP_MATRIX_H 
#define _SWAP_MATRIX_H 

#include <vector>
#include <iostream>

#include "common.h"
#include "SuccessCounter.hpp"

#include "Serializable.hpp"

class SwapMatrix : public Serializable
{
public: 
  explicit SwapMatrix(size_t numChains ); 
  SwapMatrix(SwapMatrix&& rhs) = default;  
  SwapMatrix& operator=(const SwapMatrix &rhs)  = default; 
  SwapMatrix& operator=( SwapMatrix &&rhs)  = default; 
  SwapMatrix(const SwapMatrix & rhs)  = default; 

  void update(nat a, nat b, bool acc); 
  const SuccessCounter& getCounter(nat a, nat b ) const; 

  virtual void deserialize( std::istream &in )   ; 
  virtual void serialize( std::ostream &out) const;   

  std::vector<SuccessCounter> getMatrix() const {return matrix; }

  SwapMatrix operator+(const SwapMatrix& rhs) const; 
  SwapMatrix& operator+=(const SwapMatrix& rhs) ; 

  friend void swap(SwapMatrix& a, SwapMatrix &b); 

private: 
  size_t mapToIndex(nat a, nat b) const ;
  
private:  
  std::vector<SuccessCounter> matrix; 
  size_t numEntries; 

  friend  std::ostream& operator<<(std::ostream &out, const SwapMatrix& rhs ) ; 
}; 


#endif
