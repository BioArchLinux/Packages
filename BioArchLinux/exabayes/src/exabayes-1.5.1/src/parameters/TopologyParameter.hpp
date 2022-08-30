#ifndef _TOPOLOGY_PARAMETER
#define _TOPOLOGY_PARAMETER

#include "AbstractParameter.hpp"
#include "Category.hpp"

class TopologyParameter : public AbstractParameter
{
public: 
  TopologyParameter(nat id, nat idOfMyKind, std::vector<nat> partitions )
    : AbstractParameter(Category::TOPOLOGY, id, idOfMyKind, partitions, 1)
  {
    _printToParamFile = false; 
  }

  virtual void applyParameter(TreeAln& traln , const ParameterContent &content);
  virtual ParameterContent extractParameter(const TreeAln &traln )  const;   
  virtual AbstractParameter* clone () const {return new TopologyParameter(*this); } 

  virtual void printSample(std::ostream& fileHandle, const TreeAln &traln) const {}
  virtual void printAllComponentNames(std::ostream &fileHandle, const TreeAln &traln) const  {} 

  virtual void verifyContent(const TreeAln&traln, const ParameterContent &content)  const { } // nothing to do 

  virtual log_double getPriorValue(const TreeAln& traln) const{assert(0); return log_double::fromAbs(1); } 
}; 

#endif
