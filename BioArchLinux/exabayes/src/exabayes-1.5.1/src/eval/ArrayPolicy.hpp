#ifndef _ARRAYPOLICY_HPP
#define _ARRAYPOLICY_HPP

#include <vector>
#include <memory>

// #include "Branch.hpp"
#include "ArrayOrientation.hpp"

class TreeAln; 

class ArrayPolicy
{
public: 
  ArrayPolicy()
    : partitionLnls{}
  {}
  
  virtual ~ArrayPolicy(){}

  virtual void imprintPolicy(const TreeAln &traln, ArrayOrientation &arrayOrient)  = 0; 

  void imprint(const TreeAln &traln, ArrayOrientation &arrayOrient) 
  {
    // prevLnl = traln.getTrHandle().likelihood; 
    partitionLnls = traln.getPartitionLnls(); 
    imprintPolicy(traln, arrayOrient); 
  }

  virtual void freeMemory(ArrayReservoir& res) = 0;  


  /**
     @brief deal with rejection
   */ 
  void accountForRejection(TreeAln &traln, const std::vector<bool> &partitions, const std::vector<nat>& invalidNodes, ArrayOrientation &arrayOrient, ArrayReservoir &res ) 
  {
    accountForRejectionPolicy(traln, partitions, invalidNodes, arrayOrient, res); 
   
    auto lnls = traln.getPartitionLnls(); 
    auto sum = log_double::fromAbs(1.); 
    for(nat i = 0; i < traln.getNumberOfPartitions() ; ++i)
      {
	if(partitions[i])
	  lnls[i] =  partitionLnls[i] ; 
	sum *= lnls[i]; 
      }
    traln.setPartitionLnls(lnls); 
    
    traln.getTrHandle().likelihood = sum.getRawLog();  
  }

  virtual void accountForRejectionPolicy(TreeAln &traln, const std::vector<bool> &partitions, const std::vector<nat>& invalidNodes, ArrayOrientation &arrayOrient , ArrayReservoir &res)  = 0; 


  virtual std::unique_ptr<ArrayPolicy> clone() const = 0; 
  virtual void prepareForEvaluation(TreeAln &traln, BranchPlain virtualRoot, nat models, ArrayOrientation& arrayOrientation , ArrayReservoir& res ) = 0; 

  virtual void enableRestoreGapVector(){}

protected: 
  // double prevLnl; 
  std::vector<log_double> partitionLnls;
}; 

#endif
