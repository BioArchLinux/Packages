#include "AlignmentPLL.hpp"
#include "GlobalVariables.hpp"

#include <time.h> 

#include <cwctype>

#include <cstdio>
#include <cassert>
#include <cstring>
#include <algorithm> 

extern "C"
{
  void freeLinkageList( linkageList* ll); 
}

// NASTY, needed to copy that 

static const char PLL_MAP_BIN[256] =
 {
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  3, -1, -1,
    1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  3,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
  };

static const char PLL_MAP_NT[256] =
 {
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15,
   -1,  1, 14,  2, 13, -1, -1,  4, 11, -1, -1, 12, -1,  3, 15, 15,
   -1, -1,  5,  6,  8,  8,  7,  9, 15, 10, -1, -1, -1, -1, -1, -1,
   -1,  1, 14,  2, 13, -1, -1,  4, 11, -1, -1, 12, -1,  3, 15, 15,
   -1, -1,  5,  6,  8,  8,  7,  9, 15, 10, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
 };

static const char PLL_MAP_AA[256] =
 {
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 22, -1, -1, 22, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 22,
   -1,  0, 20,  4,  3,  6, 13,  7,  8,  9, -1, 11, 10, 12,  2, -1,
   14,  5,  1, 15, 16, -1, 19, 17, 22, 18, 21, -1, -1, -1, -1, -1,
   -1,  0, 20,  4,  3,  6, 13,  7,  8,  9, -1, 11, 10, 12,  2, -1,
   14,  5,  1, 15, 16, -1, 19, 17, 22, 18, 21, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
 };



AlignmentPLL::AlignmentPLL()
  : _pllAlignmentData{nullptr}
  , _partitions{nullptr}
{
} 


AlignmentPLL::AlignmentPLL(AlignmentPLL&& rhs)
  : _pllAlignmentData{rhs._pllAlignmentData}
  , _partitions{rhs._partitions}
{
  rhs._partitions = nullptr; 
  rhs._pllAlignmentData = nullptr; 
}


AlignmentPLL& AlignmentPLL::operator=( AlignmentPLL &&rhs) 
{
  if(this == &rhs)
    return *this; 
  else 
    {
      _partitions = rhs._partitions; 
      rhs._partitions = nullptr; 
  
      _pllAlignmentData = rhs._pllAlignmentData; 
      rhs._pllAlignmentData = nullptr; 
  
      return *this; 
    }
}


void AlignmentPLL::initAln(std::string alnFile, int fileType) 
{
  // properly initialize 
  errno = 0; 

  _pllAlignmentData = pllParseAlignmentFile(fileType, alnFile.c_str()); 

  switch(errno)
    {
    case PLL_ERROR_FILE_OPEN: 
      std::cout << "could not open file" << std::endl;
      break; 
    case PLL_ERROR_INVALID_FILETYPE: 
      std::cout << "invalid file type" << std::endl;
      break; 
    case  PLL_ERROR_PHYLIP_HEADER_SYNTAX: 
      std::cout << "phylip header error" << std::endl;
      break; 
    case PLL_ERROR_PHYLIP_BODY_SYNTAX: 
      std::cout << "phylip body syntax incorrect" << std::endl;
      break; 
    case  PLL_ERROR_FASTA_SYNTAX: 
      std::cout << "fasta syntax error" << std::endl;
      break; 
    default: 
      ; 
    }

  assert(errno == 0); 
}


void AlignmentPLL::substituteBases () 
{
  auto numSeq = _pllAlignmentData->sequenceCount; 

  const char * d;
  int i, j, k;

  for (i = 0; i < _partitions->numberOfPartitions; ++ i)
   {
     switch (_partitions->partitionData[i]->dataType)
      {
        case PLL_DNA_DATA:
          d = PLL_MAP_NT;
          break;
        case PLL_BINARY_DATA:
          d = PLL_MAP_BIN;
          break;
        case PLL_AA_DATA:
          d = PLL_MAP_AA;
          break;
        default:
          assert(0);
      }
     
     for (j = 1u; j <= numSeq; ++ j)
      {
        for (k = _partitions->partitionData[i]->lower; k < _partitions->partitionData[i]->upper; ++ k)
         {
	   auto vect = _pllAlignmentData->sequenceData; 
	   vect[j][k] = d[vect[j][k]];
         }
      }
   }
}


void AlignmentPLL::initPartitions(std::string partitionFile)
{
  // guess number of partitions 

  auto &&in = std::ifstream(partitionFile); 
  auto str = std::string{""}; 
  nat ctr = 0 ; 
  while(in)
    {
      getline(in, str); 
      ++ctr ; 
    }
  PLL_NUM_BRANCHES = ctr;
  
  assert(_pllAlignmentData != nullptr);
  auto queue = pllPartitionParse(partitionFile.c_str()); 

  auto result = pllPartitionsValidate(queue, _pllAlignmentData);
  
  if (result != 1)
    {
      std::cout << "\n\nError: parsing file "  << partitionFile << " failed. \n\n"
        "Please double-check, whether each site is assigned a partition. \n"
        "Side note: the notation for choosing every n-th (e.g., triplet) \n"
        "character has changed from '\\3' to '/3'.";
      assert(0);
    }

  _partitions = pllPartitionsCommit(queue, _pllAlignmentData);
  pllQueuePartitionsDestroy(&queue); 

  pllAlignmentRemoveDups(_pllAlignmentData, _partitions);

  substituteBases();
}


AlignmentPLL::~AlignmentPLL()
{
  if(_pllAlignmentData != nullptr)
    {
      pllAlignmentDataDestroy(_pllAlignmentData); 
      _pllAlignmentData = nullptr; 
    }

  if(_partitions != nullptr )
    {
      if(_partitions->partitionData != nullptr )
	{
	  for(auto i = 0; i < _partitions->numberOfPartitions; ++i)
	    {
	      free(_partitions->partitionData[i]->partitionName); 
	      free(_partitions->partitionData[i]); 
	    }
	  free(_partitions->partitionData); 
	}
 
      free(_partitions); 
      _partitions = nullptr; 
    }
} 



void AlignmentPLL::print() const 
{
  pllAlignmentDataDumpConsole(_pllAlignmentData) ;
}


void AlignmentPLL::writeHeader(std::ofstream &out) const 
{
  auto fileId = std::string{ "BINARY"} ; 
  myWrite(out, fileId.c_str(), fileId.size() ); 

  myWrite(out, &_pllAlignmentData->sequenceCount, 1); 
  myWrite(out, &( _partitions->numberOfPartitions), 1); 
  myWrite(out,&_pllAlignmentData->sequenceLength, 1) ; 
}


void AlignmentPLL::writeToFile( std::string fileName) const 
{
  auto numPart = _partitions->numberOfPartitions; 
  auto numTax = _pllAlignmentData->sequenceCount; 

  auto &&out = std::ofstream(fileName, std::ios::binary); 
  
  writeHeader(out); 

  writeWeights(out); 

  for(auto i = 1u; i <= (size_t) numTax; i++)
    {
      auto str = _pllAlignmentData->sequenceLabels[i]; 
      int len = int(strlen(str) + 1u);
      myWrite(out, &len, 1); 
      myWrite(out, str, len); 
    } 

  // create the partition contributions 
  auto partConts = std::vector<double>(numPart, 0. ); 
  auto sum = 0.; 
  for(auto model = 0; model < numPart; ++model)
    {
      auto p = _partitions->partitionData[model]; 
      for(auto i = p->lower; i < p->upper; ++i)
	{
	  partConts[model] += _pllAlignmentData->siteWeights[i]; 
	  sum +=  _pllAlignmentData->siteWeights[i]; 
	}
    }

  for(auto &p : partConts)
    p /= sum; 

  myWrite(out, partConts.data(), numPart); 

  for(auto model = 0u; model < (size_t)numPart; model++)
    {
      auto len = 0;
      auto p = _partitions->partitionData[model];

      myWrite(out, &(p->states),1); 
      myWrite(out, &(p->maxTipStates),1); 
      myWrite(out, &(p->lower),1); 
      myWrite(out, &(p->upper),1); 
      myWrite(out, &(p->dataType),1); 
      myWrite(out, &(p->protModels),1); 
      myWrite(out, &(p->protUseEmpiricalFreqs),1); // ???
      myWrite(out, &(p->nonGTR),1); 

      len = int(strlen(p->partitionName) + 1);

      myWrite(out, &len,1); 
      myWrite(out, p->partitionName, len);
    } 

  
  for(auto i = 1; i <= _pllAlignmentData->sequenceCount; ++i)
    {
      myWrite(out, _pllAlignmentData->sequenceData[i], _pllAlignmentData->sequenceLength); 
    }

#if 0 

  // TODO 

  // also write infoness 
  auto handle = _infoness.getRawBip(); 
  
  // std::cout << "INFO: again: "; 
  // for(nat i = 0 ;i < tr->originalCrunchedLength; ++i)
  //   {
  //     if(_infoness.isSet(i))
  // 	std::cout << 1; 
  //     else 
  // 	std::cout << 0 ; 
  //   }
  // std::cout << std::endl; 
  
  myWrite(out, handle.data(), handle.size()); 
  
  // for(model = 0; model < (size_t) tr->NumberOfModels; ++model)
  //   {
  //     myWrite(out, &(tr->partitionData[model].parsimonyLength),1); 
  //     size_t numBytes =tr->partitionData[model].parsimonyLength * tr->partitionData[model].states * 2 * tr->mxtips ; 
  //     myWrite(out, tr->partitionData[model].parsVect, numBytes); 
  //   }
#endif
}



void AlignmentPLL::writeWeights(std::ofstream &out) const 
{
  auto weights = _pllAlignmentData->siteWeights; 
  auto seqLen = _pllAlignmentData->sequenceLength; 
  auto elem =  *(std::max_element(weights, weights + seqLen)); 

  // sorry, hard coding here 

  if(elem < std::numeric_limits<uint8_t>::max())
    {
      int len = sizeof(uint8_t); 
      myWrite(out, &len,1); 
      for(auto i = 0; i < seqLen; ++i)
	{
	  auto val = uint8_t(weights[i]); 
	  myWrite(out, &val, 1); 
	}
    }
  else if(elem < std::numeric_limits<uint16_t>::max())
    {
      int len = sizeof(uint16_t); 
      myWrite(out, &len,1); 
      for(auto i = 0; i < seqLen; ++i)
	{
	  auto val = uint16_t(weights[i]); 
	  myWrite(out, &val, 1); 
	}
    }
  else if(elem < std::numeric_limits<int32_t>::max())
    {
      int len = sizeof(uint32_t); 
      myWrite(out, &len,1); 
      for(auto i = 0; i < seqLen; ++i)
	{
	  auto val = uint32_t(weights[i]); 
	  myWrite(out, &val, 1); 
	}
    }
  else 
    {
      assert(0); 
      // cannot do that 
      int len = sizeof(int64_t); 
      myWrite(out, &len,1); 
      for(auto  i = 0; i < seqLen; ++i)
	{
	  auto val = int64_t(weights[i]); 
	  myWrite(out, &val, 1); 
	}
    }
}


void AlignmentPLL::createDummyPartition(Alphabet alphabet) 
{
  // get a unique file name 
  srand((unsigned int)(time(NULL))) ; 		// MEH 
  auto fn = std::string("") ;
  auto fExists = true; 
  while(fExists)
    {
      auto r = rand(); 

      auto&& ss = std::stringstream{}; 
      ss << "tmp." << r ; 
      fn = ss.str(); 

      fExists = static_cast<bool>(std::ifstream(fn)) ;
    }

  auto str = getStringFromType(alphabet); 
  auto&& out = std::ofstream{fn}; 

  out << str << ", unnamedPartition = 1 - " <<  _pllAlignmentData->sequenceLength << std::endl; 

  out.close();

  initPartitions(fn);

  remove(fn.c_str()); 
} 


int AlignmentPLL::guessFormat(std::string file)
{
  auto &&in = std::ifstream(file); 
  auto line = std::string{""};
  getline(in, line); 
  
  auto iter = begin(line); 
  auto lineEnd = end(line); 

  while(iter != lineEnd && iswspace(*iter))
    ++iter ; 

  // first non-whitespace character must be > for fasta
  if(*iter == '>')
    return PLL_FORMAT_FASTA; 
  else
    return PLL_FORMAT_PHYLIP; 

  // TODO much more could be done ...
}


