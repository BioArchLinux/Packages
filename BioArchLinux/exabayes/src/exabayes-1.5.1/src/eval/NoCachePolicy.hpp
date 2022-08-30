#ifndef NO_CACHE_POLICY_HPP
#define NO_CACHE_POLICY_HPP

#include "ArrayPolicy.hpp"
#include "BranchPlain.hpp"

class NoCachePolicy : public ArrayPolicy
{
public: 
  NoCachePolicy(const TreeAln &traln ); 
  virtual ~NoCachePolicy(){}

  virtual void imprintPolicy(const TreeAln &traln, ArrayOrientation &arrayOrient) {} 
  virtual void freeMemory(ArrayReservoir & res)  {} 
  virtual void prepareForEvaluation(TreeAln &traln, BranchPlain virtualRoot, nat models, ArrayOrientation& arrayOrientation, ArrayReservoir& res ) {} 
  virtual void accountForRejectionPolicy(TreeAln &traln, const std::vector<bool> &partitions, const std::vector<nat>& invalidNodes, ArrayOrientation &arrayOrient , ArrayReservoir &res); 
  virtual std::unique_ptr<ArrayPolicy> clone() const ; 

}; 



#endif
