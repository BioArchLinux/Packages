#ifndef _COMMANDLINE_H
#define _COMMANDLINE_H

#include <memory>
#include <iosfwd>
#include <string>

#include "Randomness.hpp"
#include "MemoryMode.hpp"

class CommandLine
{
public: 
  CommandLine(); 
  void initialize(  int argc, char **argv); 

  randCtr_t getSeed() const; 
  std::string getConfigFileName() const {return configFileName; }
  std::string getAlnFileName() const{return alnFileName; }
  std::string getRunid() const {return runid; }
  std::string getTreeFile() const {return treeFile; }
  int getNumRunParallel() const {return runNumParallel; }
  std::string getWorkdir() const {return workDir; }
  void printVersion(std::ostream &out);
  nat getNumChainsParallel() const {return chainNumParallel; }
  std::string getCheckpointId()const {return checkpointId; }
  void parseAlternative(int argc, char *argv[]); 
  bool isSaveMemorySEV() const {return saveMemorySEV; }

  std::string getCommandLineString() const ; 
  MemoryMode getMemoryMode()const {return memoryMode ;  }
  bool isDryRun() const {return dryRun; }

  bool onlyPrintHelp() const {return _onlyPrintHelp; }
  bool onlyPrintVersion() const {return _onlyPrintVersion; }

  bool alnFileIsBinary() const; 

  bool hasThreadPinning() const {return _hasThreadPinning; }

  int getNumThreads() const {return _totalThreads; }

  std::string getSingleModel() const {return singleModel; }
  std::string getModelFile() const {return modelFile; }

  bool isQuiet() const {return quiet; }

  int getReaderStride() const {return readerStride; }

  void printHelp(std::ostream& out);

private: 			// METHODS
  void assertFileExists(std::string filename); 


private: 			// ATTRIBUTES
  randCtr_t seed; 
  std::string configFileName; 
  std::string alnFileName; 
  std::string runid; 
  std::string treeFile; 
  std::string workDir;
  int runNumParallel;   
  int chainNumParallel; 
  std::string checkpointId; 
  MemoryMode memoryMode; 
  bool saveMemorySEV; 
  bool dryRun; 
  std::string modelFile; 
  std::string singleModel; 
  bool quiet; 
  int readerStride; 
  std::string _cmdString;
  int _totalThreads; 
  bool _hasThreadPinning; 
  bool _onlyPrintHelp; 
  bool _onlyPrintVersion; 
}; 


#endif
