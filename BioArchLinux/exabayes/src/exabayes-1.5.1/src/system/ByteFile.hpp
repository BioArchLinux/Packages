#ifndef _BYTE_FILE_HPP
#define _BYTE_FILE_HPP

#include <fstream>
#include <string>
#include "Partition.hpp"
// #include "time.hpp"


class Communicator; 
class ParallelSetup; 
class PartitionAssignment; 


enum class Position
{
  HEADER,
    WEIGHTS, 
    TAXA, 
    PARTITIONS, 
    ALIGNMENT, 
    PARSIMONY
}; 



enum class IntegerWidth : size_t 
{
  BIT_64 = sizeof(uint64_t) , 
    BIT_32 = sizeof(uint32_t), 
    BIT_16 = sizeof(uint16_t), 
    BIT_8  = sizeof(uint8_t)
    }; 


class ByteFile
{
public: 
  ByteFile(std::string name, bool saveMemory = false); 
  void parse(ParallelSetup &pl); 
  
  std::vector<std::string> getTaxa() const {return _taxa; }
  std::vector<Partition> getPartitions() const; 

  nat determineOptStride(Communicator& comm); 

private: 			// METHODS
  std::tuple<int,int> parseHeader(ParallelSetup& pl); 
  void checkMagicNumber();
  void parsePartitions(int numPart ); 
  void seek(Position pos); 
  void parseTaxa(int numTax); 

  template<typename T>
  void parseWeights(ParallelSetup& pl, PartitionAssignment& pAss); 
  void parseAlns(ParallelSetup& pl, PartitionAssignment& pAss);
  void distributeArray(ParallelSetup& pl); 

  void parseAlnsDirectRead(ParallelSetup& pl, PartitionAssignment& pAss); 
  void parseAlnsDirect_newLayout(ParallelSetup& pl, PartitionAssignment& pAss); 

  template<typename T> std::vector<std::vector<T>> readAndDistributeArrays(ParallelSetup& pl, PartitionAssignment& pAss, size_t numberOfArrays); 

  template <typename T> T readVar(); 
  template<typename T> std::vector<T> readArray(size_t length); 
  template<typename T> void readArray(size_t length, T* array); 

private: 			// ATTRIBUTES
  std::string _fileName; 
  std::ifstream _in; 
  std::vector<std::string> _taxa; 
  int _numPat;
  std::vector<Partition> _partitions; 
  bool _saveMemory; 
  IntegerWidth _weightType; 
  // CLOCK::system_clock::time_point _initTime; 
}; 

#include "ByteFile.tpp"

#endif
