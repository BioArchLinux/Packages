#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#include <algorithm>
#include <cstring>
#include <cassert>

#include <iostream>

#include "Partition.hpp"
#include "GlobalVariables.hpp" 

#include "BitMask.hpp"

extern const char inverseMeaningDNA[16]; // TODO remove 

int Partition::maxCategories = 4; 	// TODO!!!

Partition::Partition(nat numTax, std::string name, int dataType, int states , int maxTipStates, bool saveMemory)
  : _saveMemory{saveMemory}
  , _numTax{numTax}
  , _partition{}
  , _name{name}
  , _wgtPtr{nullptr}
  , _y{nullptr}
  , _left{}
  , _right{}
  , _EV{}
  , _tipVector{}
  , _EIGN{}
  , _EI{}
  , _substRates{}
  , _frequencies{}
  , _empiricalFrequencies{}
  , _gammaRates{}
  , _globalScaler{}
  , _yPtrs{}
  , _xVector{}
  , _xSpaceVector{}
  , _parsVect(nullptr)
  , _parsimonyScore{}
  , _parsimonyInformative(0) 
  , _gapVector(0)
  , _gapColumn(0)
  , _sumBuffer{}
{
  // allocate more space for aa matrices, since we maybe use lg4    
  auto duplicity = dataType == PLL_AA_DATA ? 4 : 1 ; 

  // std::cout << SyncOut() << "init partition" << std::endl; x
  memset(&_partition, 0, sizeof(pInfo)); 
  defaultInit();

  _partition.dataType = dataType; 
  _partition.maxTipStates = maxTipStates; 
  _partition.states = states; 

  const partitionLengths 
    *pl = getPartitionLengths(&_partition); 
  
  // allocate the memory 
  _left.resize( pl->leftLength * (maxCategories + 1));  
  _right.resize( pl->rightLength * (maxCategories + 1)); 
  _EV.resize(pl->evLength * duplicity);
  _tipVector.resize(pl->tipVectorLength * duplicity);
  _EIGN.resize( pl->eignLength * duplicity ); 
  _EI.resize(pl->eiLength * duplicity);
  _substRates.resize(pl->substRatesLength * duplicity); 
  _frequencies.resize(pl->frequenciesLength * duplicity); 
  _empiricalFrequencies.resize(pl->frequenciesLength); 
  _globalScaler.resize(2 * _numTax); 
  _yPtrs.resize(_numTax + 1);

  _parsimonyScore.resize( 2 * numTax); 

  _gammaRates.resize(Partition::maxCategories); // =(

  _xVector.resize(_numTax, nullptr);
  _xSpaceVector.resize(_numTax); 
  
  setPtrs();
}


Partition::Partition(const Partition &rhs)
  : _saveMemory (rhs._saveMemory )
  , _numTax(rhs._numTax)
  , _partition(rhs._partition)
  , _name (rhs._name )
  , _wgtPtr (rhs._wgtPtr )
  , _y (rhs._y )
  , _left (rhs._left )
  , _right (rhs._right )
  , _EV (rhs._EV )
  , _tipVector (rhs._tipVector )
  , _EIGN (rhs._EIGN )
  , _EI (rhs._EI )
  , _substRates (rhs._substRates )
  , _frequencies (rhs._frequencies )
  , _empiricalFrequencies(rhs._empiricalFrequencies) 
  , _gammaRates  (rhs._gammaRates  )
  , _globalScaler (rhs._globalScaler )
  , _yPtrs (rhs._yPtrs )
  , _xVector (rhs._xVector.size(), nullptr  )
  , _xSpaceVector (rhs._xSpaceVector.size() , 0  )
  , _parsVect (rhs._parsVect )
  , _parsimonyScore (rhs._parsimonyScore )
  , _parsimonyInformative(rhs._parsimonyInformative)
  , _gapVector(rhs._gapVector)
  , _gapColumn(rhs._gapColumn)
  , _sumBuffer(rhs._sumBuffer)
{
  defaultInit();
  setPtrs();
}


void Partition::defaultInit()
{
  _partition.optimizeBaseFrequencies = PLL_FALSE; 
  _partition.partitionLH = 1; 
  _partition.executeModel = PLL_TRUE; 
}


void Partition::setPtrs()
{
  _partition.left = _left.data(); 
  _partition.right = _right.data();

  _partition.EV = _EV.data();
  _partition.tipVector = _tipVector.data(); // ***TODO*** actually needed? 
  _partition.EIGN = _EIGN.data();
  _partition.EI = _EI.data();
  _partition.substRates = _substRates.data();
  _partition.frequencies = _frequencies.data();
  _partition.empiricalFrequencies = _empiricalFrequencies.data();
  _partition.gammaRates = _gammaRates.data();
  _partition.globalScaler = _globalScaler.data(); 
  _partition.yVector = _yPtrs.data();

  _partition.xVector = _xVector.data(); 
  _partition.xSpaceVector = _xSpaceVector.data();

  _partition.parsVect = _parsVect.get();
  _partition.parsimonyScore = _parsimonyScore.data();		

  _partition.gapVector = _gapVector.data();
  _partition.gapColumn = _gapColumn.data(); 
  
  _partition.sumBuffer = _sumBuffer.data();

  // set lg4 ptrs 
  if(getDataType() == PLL_AA_DATA)
    {
      const partitionLengths 
	*pl = getPartitionLengths(&_partition); 

      for(int i = 0 ; i < 4 ; ++i)
	{
	  _partition.EIGN_LG4[i] = _EIGN.data() + (pl->eignLength * i ); 
	  _partition.EI_LG4[i] = _EI.data() + (pl->eiLength * i); 
	  _partition.EV_LG4[i] = _EV.data() + (pl->evLength * i); 
	  _partition.tipVector_LG4[i] =  _tipVector.data() + (pl->tipVectorLength * i); 
	  _partition.substRates_LG4[i] = _substRates.data() + (pl->substRatesLength * i); 
	  _partition.frequencies_LG4[i] = _frequencies.data() +  (pl->frequenciesLength * i) ; 
	}
    }
}


// TODO should be movable, non-copyable 


Partition& Partition::operator=( Partition rhs)
{
  // std::cout << SyncOut() << "assign partition" << std::endl; 
  swap(rhs,*this); 
  return *this; 
} 


void swap(Partition& lhs, Partition& rhs)
{
  using std::swap; 

  swap(lhs._saveMemory, rhs._saveMemory); 
  swap(lhs._numTax, rhs._numTax); 
  swap(lhs._partition, rhs._partition); 
  swap(lhs._name, rhs._name); 

  swap(lhs._wgtPtr, rhs._wgtPtr); 
  swap(lhs._y, rhs._y); 

  swap(lhs._left, rhs._left);
  swap(lhs._right,rhs._right);
  swap(lhs._EV,rhs._EV);
  swap(lhs._tipVector, rhs._tipVector); 
  swap(lhs._EIGN,rhs._EIGN);  
  swap(lhs._EI,rhs._EI);
  swap(lhs._substRates,rhs._substRates);
  swap(lhs._frequencies,rhs._frequencies);
  swap(lhs._empiricalFrequencies, rhs._empiricalFrequencies); 
  swap(lhs._gammaRates,rhs._gammaRates);  
  swap(lhs._globalScaler, rhs._globalScaler); 
  swap(lhs._yPtrs, rhs._yPtrs); 

  swap(lhs._xVector, rhs._xVector); 
  swap(lhs._xSpaceVector, rhs._xSpaceVector); 

  swap(lhs._parsVect, rhs._parsVect); 
  swap(lhs._parsimonyScore, rhs._parsimonyScore); 

  swap(lhs._parsimonyInformative, rhs._parsimonyInformative); 
  swap(lhs._gapVector, rhs._gapVector); 
  swap(lhs._gapColumn, rhs._gapColumn); 
  
  swap(lhs._sumBuffer, rhs._sumBuffer); 
} 


void Partition::initGapVectorStruct()  
{
  assert(_saveMemory); 
  setGapVectorLength(  getWidth() / 32 + 1 ); 
  assert(sizeof(unsigned int) == 4); 
  _gapVector.resize(getGapVectorLength() * 2 * _numTax , 0 );
  _gapColumn.resize(_numTax * getStates() * maxCategories,0); 
}



void Partition::setAlignment( shared_pod_ptr<unsigned char> aln, int width)
{
  _y = aln; 
  _partition.width = width; 

  _yPtrs.at(0) = nullptr; 
  nat pos = 0; 
  for(nat i = 1 ; i < _numTax+1; ++i)
    {
      _yPtrs.at(i) = _y.get() + (pos * width) ; 
      ++pos; 
    }

  _sumBuffer.resize(_partition.width * _partition.states * maxCategories,0);
  _partition.sumBuffer = _sumBuffer.data();

  _partition.yVector = _yPtrs.data(); 
  
  if(_saveMemory)
    initGapVectorStruct(); 

  prepareParsimony();
}
 

void Partition::setWeights(shared_pod_ptr<int> wgts, int width)
{
  _wgtPtr = wgts; 
  _partition.width = width; 
  _partition.wgt = _wgtPtr.get();
} 


void Partition::compressDNA(const std::vector<bool> &informative)
{
  auto totalNodes = 2 * _numTax;

  auto states = getStates(); 
  auto width = getWidth(); 

  auto entries = 0; 
  for(int i = 0; i < width; i++)    
    if(informative[i])
      entries += _partition.wgt[i];  

  nat compressedEntries = entries / PLL_PCF;

  if(entries % PLL_PCF != 0)
    compressedEntries++;

  nat compressedEntriesPadded = compressedEntries; 
#if (defined(__SSE3) || defined(__AVX))
  if(compressedEntries % INTS_PER_VECTOR != 0)
    compressedEntriesPadded = compressedEntries + (INTS_PER_VECTOR - (compressedEntries % INTS_PER_VECTOR));
  else
    compressedEntriesPadded = compressedEntries;
#else
  compressedEntriesPadded = compressedEntries;
#endif     

  auto mask32 = BitMask<nat>();

  _partition.parsimonyLength = compressedEntriesPadded; 

  auto ptr = aligned_malloc<nat, size_t(EXA_ALIGN)>(compressedEntriesPadded * states * totalNodes); 
  std::fill(ptr, ptr  + compressedEntriesPadded * states * totalNodes, 0 );
  _parsVect = shared_pod_ptr<nat>(ptr);
  _partition.parsVect = _parsVect.get(); 

  const nat 
    *bitValue = getBitVector(_partition.dataType);

  for(nat taxIter = 1; taxIter <= _numTax; taxIter++)
    {			
      size_t
	compressedIndex = 0,
	compressedCounter = 0; 

      auto alnLine = _partition.yVector[taxIter]; 

      auto compressedValues = std::vector<nat>(states , 0 );
      auto compressedTips = std::vector<nat*>(states, nullptr);
      for(int k = 0 ; k < states; k++)
	compressedTips[k] = _partition.parsVect + ( (compressedEntriesPadded * states * taxIter) + (compressedEntriesPadded * k) ) ; 

      for(int index = 0; index < width; index++)
	{
	  if(informative[index])
	    {
	      auto 
		value = bitValue[alnLine[index]]; 
              
	      for(int w = 0; w < _partition.wgt[index]; w++)
		{      
		  for(int k = 0; k < states; k++)
		    {
		      if(value & mask32[k])
			compressedValues[k] |= mask32[int(compressedCounter)];
		    }
                     
		  compressedCounter++;
                  
		  if(compressedCounter == PLL_PCF)
		    {
		      for(int k = 0; k < states; k++)
			{
			  compressedTips[k][compressedIndex] = compressedValues[k];
			  compressedValues[k] = 0;
			}                    
                          
		      compressedCounter = 0;
		      compressedIndex++;
		    }
		}
	    }
	}
                           
      for(;compressedIndex < compressedEntriesPadded; compressedIndex++)
	{   
	  for(;compressedCounter < PLL_PCF; compressedCounter++)              
	    for(int k = 0; k < states; k++)
	      compressedValues[k] |= mask32[int(compressedCounter)];               
          
	  for(int k = 0; k < states; k++)
	    {
	      compressedTips[k][compressedIndex] = compressedValues[k];
	      compressedValues[k] = 0;
	    }                     
              
	  compressedCounter = 0;
	}           
    }               
}




bool Partition::isInformative(int site) const 
{
  auto dataType = getDataType();

  int
    check[256],   
    undetermined = getUndetermined(dataType);
  memset(check, 0, sizeof(int) * 256); 

  const nat
    *bitVector = getBitVector(dataType);
  
  for(nat i = 1 ; i <= _numTax; ++i)
    {      
      auto nucleotide = _partition.yVector[i][site];            
      ++check[nucleotide];
      assert(bitVector[nucleotide] > 0);                   
    }
  
  nat infoCtr = 0; 
  for( int i = 0; i < undetermined; ++i)
    if(check[i] > 0)
      ++infoCtr ; 

  if(infoCtr <= 1 )
    return false;    
  else
    {        
      for(int i = 0; i < undetermined; i++)
        {
          if(check[i] > 1)
            return true;
        } 
    }
     
  return false; 
}


std::vector<bool> Partition::determineUninformativeSites() const 
{
  auto result = std::vector<bool>(getWidth(), false);
  for(int i = 0; i <  getWidth() ; ++i)
    if(isInformative(i))
      result[i]= true; 
  return result; 
}


void Partition::prepareParsimony()
{
  if( not _parsimonyInformative.empty() )
    {
      // auto tp = getTimePoint(); 
      compressDNA(_parsimonyInformative);
      // out << "compress: "<< getDuration(tp) << " sec"  << std::endl; 
    }
  else 
    {
      // auto tp = getTimePoint(); 
      auto informative = determineUninformativeSites();
      // tout << "determine uninfo: "<< getDuration(tp)  << " sec"<< std::endl; 

      // tp = getTimePoint(); 
      compressDNA(informative);
      // tout << "compress: "<< getDuration(tp) << " sec"  << std::endl; 
    }
}


std::ostream& Partition::printAlignment(std::ostream &out)
{
  for(nat i = 1; i < _numTax+1; ++i)
    {
      for(int j = 0; j < getWidth(); ++j)
	out << inverseMeaningDNA[_partition.yVector[i][j]] ; 
      out << "\n"; 
    }

  return out; 
}





std::ostream& operator<<(std::ostream& out, const Partition& rhs )
{
  out << SHOW(  rhs._saveMemory)
      << SHOW(rhs._numTax )
      << SHOW( &(rhs._partition) ) 
      << SHOW(rhs._name )
      // << SHOW(rhs._wgtPtr.get() )
      // << SHOW(rhs._y.get() )
      << SHOW(  std::vector<double>(begin(rhs._left), end(rhs._left)) )
      << SHOW(std::vector<double>(begin(rhs._right),end(rhs._right) ))
      << SHOW(std::vector<double>(begin(rhs._EV),end(rhs._EV) ))
      << SHOW(std::vector<double>(begin(rhs._tipVector),end(rhs._tipVector) ))
      << SHOW(rhs._EIGN )
      << SHOW(rhs._EI )
      << SHOW(rhs._substRates )
      << SHOW(rhs._frequencies )
      << SHOW(rhs._empiricalFrequencies )
      << SHOW(rhs._gammaRates  )
      << SHOW(rhs._globalScaler )
      << SHOW(rhs._yPtrs )
      << SHOW(rhs._xVector )
      << SHOW(rhs._xSpaceVector )
    
    // TODO 
      // << SHOW(rhs._parsVect.get() )
      // << SHOW(rhs._parsimonyScore)
      // << SHOW(rhs._parsimonyInformative )
      << SHOW(rhs._gapVector)
      << SHOW(std::vector<double>(begin(rhs._gapColumn),end(rhs._gapColumn)  )); 
  return out; 
}
