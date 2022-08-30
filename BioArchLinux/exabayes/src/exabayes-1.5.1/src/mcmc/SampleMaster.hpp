/** 
    @file SampleMaster.hpp
    
    @brief represents various chains sampling the posterior probability space
    
    Despite of its modest name, this is in fact the master class.  
 */ 

#ifndef _SAMPLING_H
#define _SAMPLING_H

#include <vector>

#include "ProposalSet.hpp"
#include "BlockRunParameters.hpp"
#include "BlockProposalConfig.hpp"
#include "CommandLine.hpp"
#include "CoupledChains.hpp"
#include "ConfigReader.hpp"
#include "ParallelSetup.hpp"
#include "TimeTracker.hpp"
#include "Serializable.hpp"
#include "DiagnosticsFile.hpp"

class SampleMaster : public Serializable
{
public:   
  SampleMaster(size_t numCpu) ; 
  SampleMaster(const SampleMaster& rhs) = default; 
  SampleMaster& operator=(const SampleMaster &rhs)  = default; 

  /** 
      @brief initializes the runs  
      @notice this is the top-level function 
   */ 
  void initializeRuns(Randomness rand); 
  // nat peekNumTax(std::string filePath); 
  /** 
      @brief cleanup, once finished
   */ 
  void finalizeRuns();  
  /** 
      @brief starts the MCMC sampling   
   */ 
  void simulate(); 
  /** 
      @brief initializes the config file 
   */ 
  std::tuple<ParameterList , std::vector<std::unique_ptr<AbstractProposal> > , std::vector<ProposalSet> >  
  processConfigFile(string configFileName, const TreeAln &tralnPtr ) ; 
  void initializeWithParamInitValues(TreeAln &tree , const ParameterList &params , bool hasBl ) const ; 

  void makeTreeUltrametric( TreeAln &traln, std::vector<AbstractParameter*> divTimes, std::vector<AbstractParameter*> &divRates) const; 
  /** 
      @brief EXPERIMENTAL 
   */ 
  void branchLengthsIntegration(Randomness &rand)  ;  
  /** 
      @brief print information about the alignment  
   */ 
  void printAlignmentInfo(const TreeAln &traln);   
  /** 
      @brief gets the runs 
   */ 
  const std::vector<CoupledChains>& getRuns() const {return _runs; }
  
  virtual void deserialize( std::istream &in ) ; 
  virtual void serialize( std::ostream &out) const ;

  void setCommandLine(CommandLine cl) { _cl = cl; }

  void setParallelSetup(ParallelSetup* pl ) { _plPtr = pl; }

private: 
  void printParameters(const TreeAln &traln, const ParameterList &params) const; 
  void printProposals( std::vector<unique_ptr<AbstractProposal> > &proposals,  std::vector<ProposalSet> &proposalSets, ParameterList &params  ) const ; 
  void printInitializedFiles() const; 
  std::vector<std::string> getStartingTreeStrings(); 
  void informPrint(); 
  void printInitialState(); 
  LikelihoodEvaluator createEvaluatorPrototype(const TreeAln &initTree, bool useSEV); 
  void writeCheckpointMaster(); 
  void initializeFromCheckpoint(); 
  std::pair<double,double> convergenceDiagnostic(nat &begin, nat &end); 
  std::vector<bool> initTrees(std::vector<TreeAln> &trees, randCtr_t seed, std::vector<std::string> startingTreeStrings, const std::vector<AbstractParameter*> &params); 
  bool initializeTree(TreeAln &traln, std::string startingTree, Randomness &treeRandomness, const std::vector<AbstractParameter*> &params); 
  void printDuringRun(uint64_t gen) ; 
  std::string getOrCreateBinaryFile() const ; 
  void catchRunErrors() const ; 

private:			// ATTRIBUTES 
  std::vector<CoupledChains> _runs; 
  ParallelSetup* _plPtr;  	// non-owning!
  BlockRunParameters _runParams;  
  CommandLine _cl; 
  DiagnosticsFile _diagFile; 

  TimeTracker _timeTracker; 
  TimeTracker _printTime; 
};  

#endif
