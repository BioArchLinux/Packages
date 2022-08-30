#ifndef _PRIORS_H
#define _PRIORS_H

#include <vector>
#include <limits>
#include <cassert>
#include <cmath>
#include <iostream>

#include "Density.hpp"

#include "Randomness.hpp"
#include "TreeAln.hpp"

#include "ParameterContent.hpp"


class AbstractPrior
{
public: 
  AbstractPrior()
    : _keepInitData{false}
  { }
  
  virtual ~AbstractPrior() {}

  /** 
      @brief obtains a pre-defined initial value, depending on the
      prior.
  */
  virtual ParameterContent getInitialValue() const = 0; 
  virtual log_double accountForMeanSubstChange( TreeAln &traln, const AbstractParameter* param , double myOld, double myNew ) const = 0; 
  virtual ParameterContent drawFromPrior(Randomness &rand)  const = 0 ; 
  virtual log_double getLogProb( const ParameterContent &content ) const = 0; 
  virtual void print(std::ostream &out) const = 0;

  virtual bool needsIntegration() const = 0; 

  virtual AbstractPrior* clone() const = 0;

  virtual double getFirstDerivative( const AbstractParameter& param) const = 0; 
  
  // only for internal branch lengths; this is very ugly, however we need this for maintaining numerical stability with > 300 partitions
  virtual log_double getUpdatedValue(double oldRawVal, double newRawVal, const AbstractParameter* param) const = 0;

  friend std::ostream& operator<<(std::ostream &out,  AbstractPrior* rhs)
  {

    rhs->print(out);
    return out; 
  }
  
  bool isKeepInitData() const {return _keepInitData; }
  void setKeepInitData()  { _keepInitData = true; }

protected: 
  bool _keepInitData;

}; 

 
#endif
