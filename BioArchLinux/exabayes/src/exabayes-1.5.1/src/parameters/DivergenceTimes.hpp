#ifndef DIVERGENCETIME_H
#define DIVERGENCETIME_H


#include "AbstractParameter.hpp"

#include "NodeAge.hpp"

class DivergenceTimes : public AbstractParameter
{
public:
  DivergenceTimes(nat id, nat idOfMyKind, std::vector<nat> partitions, NodeAge age);
  virtual ~DivergenceTimes(){}

  virtual void applyParameter(TreeAln& traln,  const ParameterContent &content);
  virtual ParameterContent extractParameter(const TreeAln &traln)  const  ;   
  virtual void printSample(std::ostream& fileHandle, const TreeAln &traln ) const ; 
  virtual void printAllComponentNames(std::ostream &fileHandle, const TreeAln &traln) const  ; 
  virtual void verifyContent(const TreeAln &traln, const ParameterContent &content) const; 
  void initializeParameter(TreeAln& traln,  const ParameterContent &content, bool root = false);
  
  virtual std::ostream& printShort(std::ostream& out) const;

  virtual AbstractParameter* clone() const  
  {
    return new DivergenceTimes(*this); 
  } 

  virtual log_double getPriorValue(const TreeAln& traln) const; // {assert(0); return log_double::fromAbs(1); } 
  
private:
  bool _rootNode;
  NodeAge _nodeAge;
};



#endif /* DIVERGENCETIME_H */
