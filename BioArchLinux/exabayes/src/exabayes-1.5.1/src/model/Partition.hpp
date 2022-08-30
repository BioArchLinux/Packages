#ifndef _PARTITION_HPP
#define _PARTITION_HPP

#include "pll.h"
#include <memory>
#include "extensions.hpp"
#include <iosfwd>


class Partition
{
  static constexpr int INTS_PER_VECTOR = size_t(EXA_ALIGN) / sizeof(nat); 

public:

  static int maxCategories; 	// TODO 

  Partition(nat numTax, std::string name, int dataType, int states , int maxTipStates, bool saveMemory); 
  ~Partition(){}
  Partition(const Partition &rhs); 
  Partition( Partition &&rhs) = default ; 
  Partition& operator=( Partition rhs); 
  friend void swap(Partition& lhs, Partition& rhs) ; 

  void defaultInit(); 

  pInfo& getHandle() {return _partition;} 
  const pInfo& getHandle() const {return _partition;} 
  std::string getName() const {return _name; }

  int getStates() const {return _partition.states; }
  void setStates(int states) {_partition.states = states; }
  
  int getMaxTipStates() const {return _partition.maxTipStates; }
  void setMaxTipStates(int s) {_partition.maxTipStates= s ; }
  
  int getLower() const {return _partition.lower; }
  void setLower(int lower) { _partition.lower = lower; } 
  
  int getUpper() const {return _partition.upper; }
  void setUpper( int upper) {_partition.upper = upper; }
  
  int getWidth() const {return _partition.width; }
  void setWidth(int width) {_partition.width = width; }
  
  int getDataType() const {return _partition.dataType; }
  void setDataType(int dataType) {_partition.dataType = dataType; }
  
  int getProtModels() const {return _partition.protModels; } 
  void setProtModel(int protModel)  { _partition.protModels = protModel; }
  
  int getProtFreqs() const { return _partition.protUseEmpiricalFreqs ; }
  void setProtFreqs(int protFreqs ) {_partition.protUseEmpiricalFreqs = protFreqs; }

  void setNonGTR(int nonGTR){_partition.nonGTR = nonGTR; } 
  int getNonGTR() const {return _partition.nonGTR; }

  double getPartitionContribution() const {return _partition.partitionContribution; }
  void setPartitionContribution(double tmp) {_partition.partitionContribution = tmp; }

  void setAlignment(shared_pod_ptr<unsigned char> aln, int width ); 
  void setWeights(shared_pod_ptr<int> wgts, int width); 

  double getAlpha()const {return _partition.alpha; }
  void setAlpha(double a){_partition.alpha = a; }

  std::vector<double> getSubstRates() const { return _substRates; }
  void setSubstRates(  std::vector<double> r)  { _substRates = r; }

  std::vector<double> getFrequencies() const {return _frequencies; }
  void setFrequencies(std::vector<double> f) {_frequencies = f;} 

  void prepareParsimony(); 

  double getFracChange() const {return _partition.fracchange; }
  void setFracChange(double f) { _partition.fracchange = f; }

  nat getPartitionParsimony(){return _parsimonyScore.at(0); }

  std::ostream& printAlignment(std::ostream &out);

  int getGapVectorLength() const { return _partition.gapVectorLength; }
  void setGapVectorLength( int len) { _partition.gapVectorLength = len; }

  void setParsimonyInformative(std::vector<bool> theInfo){_parsimonyInformative = theInfo; }

  friend std::ostream& operator<<(std::ostream& out, const Partition& rhs ); 


private: 			// METHODS
  void setPtrs();
  bool isInformative(int site) const; 
  std::vector<bool> determineUninformativeSites() const ; 
  void compressDNA(const std::vector<bool> &informative); 
  void initGapVectorStruct()  ; 

private: 			// ATTRIBUTES
  bool _saveMemory; 
  nat _numTax; 
  pInfo _partition; 
  std::string _name ;

  shared_pod_ptr<int> _wgtPtr; 
  shared_pod_ptr<unsigned char> _y; 
  
  typename aligned_vector<double>::type _left; 
  typename aligned_vector<double>::type _right; 
  typename aligned_vector<double>::type _EV; 
  typename aligned_vector<double>::type _tipVector; 
  std::vector<double> _EIGN; 
  std::vector<double> _EI; 
  std::vector<double> _substRates; 
  std::vector<double> _frequencies; 
  std::vector<double> _empiricalFrequencies; 
  std::vector<double> _gammaRates ; 
  std::vector<nat> _globalScaler; 
  std::vector<unsigned char*> _yPtrs; 

  std::vector<double*> _xVector; 
  std::vector<size_t> _xSpaceVector; 

  shared_pod_ptr<nat> _parsVect; 
  std::vector<nat> _parsimonyScore;

  std::vector<bool> _parsimonyInformative; 
  std::vector<nat> _gapVector;
  typename aligned_vector<double>::type _gapColumn ; 
  typename aligned_vector<double>::type _sumBuffer; 

};


#endif
