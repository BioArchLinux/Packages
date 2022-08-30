#ifndef _SUCCESSCTR_H
#define _SUCCESSCTR_H

#include <iostream>
#include <list>

#include "Serializable.hpp"

#define SIZE_OF_LAST 100 

// TODO  an success interval would be nice 


class SuccessCounter : public Serializable
{
public: 
  SuccessCounter();  
  SuccessCounter(const SuccessCounter& rhs) = default ; 
  SuccessCounter( SuccessCounter&& rhs) = default ; 
  SuccessCounter& operator=(  const SuccessCounter& rhs)  = default ; 
  SuccessCounter& operator=(   SuccessCounter&& rhs)  = default ; 

  void accept(); 
  void reject();
  int getRecentlySeen() const {return localAcc + localRej; }
  double getRatioInLastInterval() const ; 
  double getRatioOverall() const ; 
  void nextBatch(); 
  int getBatch() const {return batch; }
  nat getTotalSeen()const  {return globalAcc + globalRej; }
  
  virtual void deserialize( std::istream &in )   ; 
  virtual void serialize( std::ostream &out) const ;   

  SuccessCounter operator+(const SuccessCounter &rhs) const ; 
  friend void swap(SuccessCounter &elemA, SuccessCounter &elemB); 
  
private: 			// METHODS
  void reset();  

private:		  // ATTRIBUTES
  nat globalAcc; 
  nat globalRej;   
  nat localAcc; 
  nat localRej; 
  nat batch; 			

  friend std::ostream& operator<<(std::ostream& rhs, const SuccessCounter &b ); 
}; 

#endif
