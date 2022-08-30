/**
   @file PriorBelief.hpp

   @brief top level object that represents the prior probability of a
   chain
 */ 


#ifndef _PRIORMANAGER_H
#define _PRIORMANAGER_H

#include <cassert>
#include <cmath>
#include <memory>
#include <vector>
#include <iostream>

#include "GlobalVariables.hpp"
#include "extensions.hpp"

#include "ParameterList.hpp"

class AbstractPrior; 
class AbstractParameter; 
class TreeAln;
class PriorBelief
{
public:
  PriorBelief();  
  /** 
      @brief intializes and scores the prior each parameter    
   */ 
  void initialize(const TreeAln &traln, ParameterList &params);   
  /** 
      @brief informs the prior about acceptance of the new state    
   */ 
  void accept()  { assert(wasInitialized); _lnPrior *= _lnPriorRatio;  _lnPriorRatio = log_double::fromAbs(1.); }  
  /** 
      @brief informs the prior about rejection of the new state 
   */
  void reject() { assert(wasInitialized) ; _lnPriorRatio = log_double::fromAbs(1.); }
  /** 
      @brief adds a (logarithmic!) value to the prior ratio
   */ 
  void addToRatio(log_double val); 
  
  /** 
      @brief accounts for branch length prior changes due to either
      substitution rate or state frequencies updates

      @param oldFc old fracchanges for each involved branch length parameter
      @param newFc new fracchanges (after proposal) for each involved branch length parameter 

      @notice indices in the fracchange arrays correspond to
      idOfMyKind of branch length parameters

      @notice this is very pedastrian, but I do not see how to avoid this
   */ 
  // void accountForFracChange( TreeAln &traln, const std::vector<double> &oldFc, const std::vector<double> &newFcs, 
  // 			    const std::vector<AbstractParameter*> &affectedBlParams )  ; 
  /** 
      @brief verifies the prior 
   */ 
  void verifyPrior(const TreeAln &traln, ParameterList& variables)  ;  

  ///////////////
  // OBSERVERS //
  ///////////////
  log_double getLnPrior () const { // assert(not std::isinf(_lnPrior)) ;
    assert(wasInitialized); return _lnPrior; } 
  log_double getLnPriorRatio() const {assert(wasInitialized) ; return _lnPriorRatio; }

private: 
  log_double scoreEverything(const TreeAln &traln, ParameterList& params) const ; 
  
  // having an internal state actually defies the logic of the randomVariables being external 
  log_double _lnPrior; 
  log_double _lnPriorRatio; 
  bool wasInitialized; 

  // double _error; 
}; 



#endif
