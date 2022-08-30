#ifndef REV_MAT_PARAMETER
#define REV_MAT_PARAMETER


#include "RateHelper.hpp"
#include "Category.hpp"
#include "AbstractParameter.hpp"
  
class RevMatParameter : public AbstractParameter
{
public: 
  RevMatParameter(nat id, nat idOfMyKind, std::vector<nat> partitions ) 
    : AbstractParameter(Category::SUBSTITUTION_RATES, id, idOfMyKind, partitions, 1  )
  {
  }

  RevMatParameter(const RevMatParameter &rhs )
    : AbstractParameter(rhs)
  {    
  }

  virtual void applyParameter(TreeAln& traln, const ParameterContent &content);
  virtual void applyParameterRaw(TreeAln &traln, const ParameterContent & content) const ; 
  virtual ParameterContent extractParameter(const TreeAln &traln )  const;   
  virtual ParameterContent extractParameterRaw(const TreeAln& traln) const ; 

  virtual AbstractParameter* clone () const {return new RevMatParameter(*this); }

  virtual void printSample(std::ostream& fileHandle, const TreeAln &traln) const ; 
  virtual void printAllComponentNames(std::ostream &fileHandle, const TreeAln &traln) const ; 

  virtual void verifyContent(const TreeAln&traln, const ParameterContent &content) const ; 
  
  virtual void checkSanityPartitionsAndPrior(const TreeAln &traln) const ; 

  virtual bool priorIsFitting(const AbstractPrior &prior, const TreeAln &traln) const
  {
    auto content = prior.getInitialValue();
    auto& partition = traln.getPartition(_partitions.at(0));
    return content.values.size()  ==  RateHelper::numStateToNumInTriangleMatrix(partition.getStates()); 
  }
  
    virtual log_double getPriorValue(const TreeAln& traln) const{assert(0); return log_double::fromAbs(1); } 
}; 

#endif
