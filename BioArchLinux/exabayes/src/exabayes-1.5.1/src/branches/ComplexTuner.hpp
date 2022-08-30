#ifndef _COMPLEX_TUNER_HPP
#define _COMPLEX_TUNER_HPP


#include "SuccessCounter.hpp"
#include "Serializable.hpp"


/**
   This is a style of tuning that is necessary for tuning the branch
   length distribution proposal.
   
 */ 



class ComplexTuner   : public Serializable
{

public: 			// INHERITED 
  virtual void deserialize( std::istream &in )   ; 
  virtual void serialize( std::ostream &out) const;   

public: 
  ComplexTuner(double parameter, double minBound, double maxBound, double maxDelta, bool logScale )
    : _parameter{parameter}
    , _sctr{}
    , _tuneUp{true}
    , _prevSuccess{0.}
    , _minBound{minBound}
    , _maxBound{maxBound}
    , _maxDelta{maxDelta}
    , _logScale{logScale}
  {
  } 
  
  void accept(){ _sctr.accept(); }
  void reject(){ _sctr.reject(); }
  double getRecentlySeen() const { return _sctr.getRecentlySeen(); }

  double tuneParameter(int batch, double parameter, bool increase ) const ; 

  void tune()  ; 

  void setParameter(double p)  { _parameter = p; }
  double getParameter() const { return _parameter; }
  double getRatio() const { return _sctr.getRatioInLastInterval(); }

private: 
  double _parameter; 
  SuccessCounter _sctr; 
  bool _tuneUp; 
  double _prevSuccess; 

  double _minBound; 
  double _maxBound; 
  
  double _maxDelta; 		// upper bound by how much we tune in one step   
  
  bool  _logScale; 
}; 


#endif
