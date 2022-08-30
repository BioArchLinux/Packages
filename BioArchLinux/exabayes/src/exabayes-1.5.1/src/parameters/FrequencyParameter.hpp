#ifndef FREQ_PARAMETER
#define FREQ_PARAMETER

#include "TreeAln.hpp"
#include "AbstractParameter.hpp"
#include "Category.hpp"
  
class FrequencyParameter : public AbstractParameter
{
public: 
  FrequencyParameter(nat id, nat idOfMyKind, std::vector<nat> partitions )
    : AbstractParameter(Category::FREQUENCIES, id , idOfMyKind, partitions, 1 )
  { 
  }

  FrequencyParameter(const FrequencyParameter &rhs )
    : AbstractParameter(rhs)
  {    
  }

  virtual void applyParameter(TreeAln& traln, const ParameterContent &content);
  virtual ParameterContent extractParameter(const TreeAln &traln)  const;   
  virtual AbstractParameter* clone () const {return new FrequencyParameter(*this); } 

  virtual void printSample(std::ostream& fileHandle, const TreeAln &traln) const ; 
  virtual void printAllComponentNames(std::ostream &fileHandle, const TreeAln &traln) const  ; 

  virtual void verifyContent(const TreeAln &traln, const ParameterContent &content) const;

  virtual void checkSanityPartitionsAndPrior(const TreeAln& traln) const ; 
  
  virtual bool priorIsFitting(const AbstractPrior &prior, const TreeAln &traln) const
  {
    auto content = prior.getInitialValue();
    auto& partition = traln.getPartition(_partitions.at(0));
    return content.values.size() == nat(partition.getStates()); 
  } 

  virtual log_double getPriorValue(const TreeAln& traln) const{assert(0); return log_double::fromAbs(1); } 
}; 


#endif
