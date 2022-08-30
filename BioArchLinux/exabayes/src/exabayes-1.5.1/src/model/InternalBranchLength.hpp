#ifndef INTERNALBRANCHLENGTH_H
#define INTERNALBRANCHLENGTH_H


#include "Serializable.hpp"

class InternalBranchLength : public Serializable
{
public: 			// INHERITED
  
  virtual void deserialize( std::istream &in )   ; 
  virtual void serialize( std::ostream &out) const;   


public:
  InternalBranchLength(double z = 0)
    : _internalBranchLength {z }
  {
  }
  
  virtual ~InternalBranchLength() { }
  

  /** 
      Creates an internal branch length (i.e., z-value) for a tree 
   */ 
  static InternalBranchLength fromAbsolute(double absLen, double meanSubstRate);

  double getValue() const 
  {
    return _internalBranchLength; 
  }


  void setValue(double val)
  {
    _internalBranchLength = val;
  }

  
  double toMeanSubstitutions(double meanSubstRate) const ;

private: 
  double _internalBranchLength; 
};
 



#endif /* INTERNALBRANCHLENGTH_H */
