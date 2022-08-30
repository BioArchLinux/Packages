#ifndef _BLOCK_RUNPARAMETERS_H
#define _BLOCK_RUNPARAMETERS_H


#include <cassert>
#include "ExaBlock.hpp"
#include "common.h"


class BlockRunParameters : public ExaBlock
{
public: 
  BlockRunParameters(); 
  BlockRunParameters(const BlockRunParameters& rhs) = default ;
  BlockRunParameters(BlockRunParameters&& rhs) = default; 
  BlockRunParameters& operator=(const BlockRunParameters &rhs) = default; 
  BlockRunParameters& operator=(BlockRunParameters &&rhs) = default; 

  virtual void Read(NxsToken &token); 

  // getters 
  nat getTuneFreq() const { return tuneFreq;  }
  bool getTuneHeat() const { return tuneHeat; }
  double getNumSwapsPerGen() const {return numSwapsPerGen; }
  double getHeatFactor() const { return heatFactor ; }
  nat getPrintFreq() const { return printFreq; }
  nat getNumCoupledChains() const { return numCoupledChains; }
  bool isUseAsdsfMax() const { return useAsdsfMax; }
  uint64_t getNumGen() const { return numGen; }
  nat getNumRunConv() const { return numRunConv; }
  nat getSamplingFreq() const { return samplingFreq; }
  double getBurninProportion() const { return burninProportion; }
  nat getBurninGen() const { return burninGen; }
  double getAsdsfIgnoreFreq() const { return asdsfIgnoreFreq; 	}
  nat getDiagFreq() const { return diagFreq ; }
  double getAsdsfConvergence() const {return asdsfConvergence; }
  bool isUseParsimonyStarting() const {return useParsimonyStarting; } 
  bool isHeatedChainsUseSame() const {return heatedChainsUseSame;}
  nat getChkpntFreq() const {return chkpntFreq; }
  bool isComponentWiseMH() const {return componentWiseMH; }
  bool isUseStopCriterion() const {return useStopCriterion; }

  void verify() const ; 

private: 
  nat diagFreq ; 
  double asdsfIgnoreFreq; 	
  double asdsfConvergence; 
  bool useStopCriterion; 
  nat burninGen; 
  double burninProportion; 
  int samplingFreq; 
  int numRunConv; 
  uint64_t numGen; 
  int numCoupledChains; 
  int printFreq; 
  double heatFactor ; 
  bool tuneHeat; 
  nat tuneFreq;  
  bool useParsimonyStarting; 
  bool heatedChainsUseSame; 
  nat chkpntFreq; 
  bool componentWiseMH; 
  bool useAsdsfMax; 
  double numSwapsPerGen;   
}; 


#endif
