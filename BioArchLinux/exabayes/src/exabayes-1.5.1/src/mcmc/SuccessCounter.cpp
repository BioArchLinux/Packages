#include <iostream>
#include <cassert>
#include "SuccessCounter.hpp"

#include "common.h"

SuccessCounter::SuccessCounter() 
  : globalAcc(0)
  , globalRej(0)
  , localAcc(0)
  , localRej(0)
  , batch(0)
{
}


// SuccessCounter::SuccessCounter(const SuccessCounter& rhs)
//   : globalAcc(rhs.globalAcc)
//   , globalRej(rhs.globalRej)
//   , localAcc(rhs.localAcc)
//   , localRej(rhs.localRej)
//   , batch(rhs.batch)
// {  
// }


void SuccessCounter::accept() 
{
  ++globalAcc;
  ++localAcc; 
}


void SuccessCounter::reject()
{
  ++globalRej;
  ++localRej; 
}


double SuccessCounter::getRatioInLastInterval() const 
{ 
  double ratio =   ((double)localAcc) / ((double)(localAcc + localRej) ); 
  // cout << "ratio is " <<  ratio << endl; 
  return  ratio ; 
}


double SuccessCounter::getRatioOverall() const 
{
  return (double) globalAcc / ((double)(globalAcc + globalRej)); 
}

void SuccessCounter::nextBatch()
{
  batch++;
  reset();
}



void SuccessCounter::reset()
{
  localRej = 0;
  localAcc = 0;
}


void SuccessCounter::deserialize( std::istream &in )   
{
  globalAcc = cRead<decltype(globalAcc)>(in); 
  globalRej = cRead<decltype(globalRej)>(in); 
  localAcc = cRead<decltype(localAcc)>(in); 
  localRej = cRead<decltype(localRej)>(in); 
  batch = cRead<decltype(batch)>(in); 
}
 
void SuccessCounter::serialize( std::ostream &out)  const
{
  // local stuff needed for the proposals!!!

  cWrite<decltype(globalAcc)>(out, globalAcc); 
  cWrite<decltype(globalRej)>(out, globalRej); 
  cWrite<decltype(localAcc)>(out, localAcc); 
  cWrite<decltype(localRej)>(out, localRej); 
  cWrite<decltype(batch)>(out, batch); 
} 



SuccessCounter SuccessCounter::operator+(const SuccessCounter &rhs) const 
{
  auto result =  SuccessCounter{}; 
  result.globalAcc = rhs.globalAcc + globalAcc; 
  result.globalRej = rhs.globalRej + globalRej; 
  result.localAcc = rhs.localAcc + localAcc; 
  result.localRej = rhs.localRej + localRej; 
  result.batch = rhs.batch + batch; 
  
  return result; 
} 


std::ostream& operator<<( std::ostream &out, const SuccessCounter &rhs)
{
  out << rhs.globalAcc << "/" <<  rhs.globalRej; 
  return out; 
}


