#ifndef RATE_HET_PARAMETER
#define RATE_HET_PARAMETER

#include "AbstractParameter.hpp"
#include "Category.hpp"
  
class RateHetParameter : public AbstractParameter
{
public: 

  RateHetParameter(nat id, nat idOfMyKind, std::vector<nat> partitions )
    : AbstractParameter(Category::RATE_HETEROGENEITY, id, idOfMyKind, partitions,1 )
  {
  }
  
  virtual void applyParameter(TreeAln& traln, const ParameterContent &content);
  virtual ParameterContent extractParameter(const TreeAln &traln )  const;   
  virtual AbstractParameter* clone () const {return new RateHetParameter(*this); } 

  virtual void printSample(std::ostream& fileHandle, const TreeAln &traln) const ; 
  virtual void printAllComponentNames(std::ostream &fileHandle, const TreeAln &traln) const ; 

  virtual void verifyContent(const TreeAln&traln, const ParameterContent &content) const; 

    virtual log_double getPriorValue(const TreeAln& traln) const{assert(0); return log_double::fromAbs(1); } 
}; 

#endif
