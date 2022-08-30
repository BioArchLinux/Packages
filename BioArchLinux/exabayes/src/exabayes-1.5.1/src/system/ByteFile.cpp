#include "ByteFile.hpp"

#include "Bipartition.hpp"
#include "PartitionAssignment.hpp"

#include "BandWidthTest.hpp"

#include "ParallelSetup.hpp"

#include <iostream>
#include <cassert>
#include <cstring>

#define DIRECT_READ

extern const char inverseMeaningDNA[16]; // TODO remove 

ByteFile::ByteFile(std::string name, bool saveMemory)
  : _fileName(name)  
  , _in{_fileName, std::ios::binary}
  , _taxa{}
  , _numPat{0}
  , _partitions{}
  , _saveMemory{saveMemory}
  , _weightType{IntegerWidth::BIT_8}
{
}



nat ByteFile::determineOptStride(Communicator &comm)
{
  auto t = BandWidthTest(); 
  auto opt = t.determineOptimum(comm, _fileName);
  return opt; 
}


template<typename T>
std::vector<std::vector<T>> ByteFile::readAndDistributeArrays(ParallelSetup& pl, PartitionAssignment& pAss, size_t numberOfArrays)
{
  auto result = std::vector<std::vector<T>>(); 

  auto fileStart = _in.tellg();
  auto numProcPerChain = std::max( ( pl.getGlobalComm().size() / pl.getChainsParallel()) / pl.getRunsParallel(),size_t(1u)); 
  auto participatingProcs = numProcPerChain * pl.getChainsParallel() * pl.getRunsParallel(); 
  auto numIter = (numberOfArrays / participatingProcs) +   ( ((numberOfArrays % participatingProcs) == 0 ) ? 0 : 1 ); 

  auto countsPerProc = std::vector<int>{}; 
  auto displPerProc = std::vector<int>{};
  std::tie(countsPerProc, displPerProc) = pAss.getCountsAndDispls(pl.getChainComm().size()) ; 
  
  auto arrsProcessed = size_t(0u); 
  for(nat iter = 0; iter < numIter ; ++iter)
    {
      auto arraysThisRound = std::min(size_t(participatingProcs), numberOfArrays - iter * participatingProcs);
      auto arrs = std::vector< std::vector<T> >(arraysThisRound); 
      auto myArrNum = iter * participatingProcs + pl.getGlobalComm().getRank(); 

      if( nat(pl.getGlobalComm().getRank()) < participatingProcs)
	{
	  if( myArrNum < numberOfArrays)
	    {
	      _in.seekg( std::streamoff(size_t(fileStart) +  ( _numPat * sizeof(T) ) * myArrNum), std::ios::beg );
	      arrs[myArrNum - arrsProcessed ] = readArray<T>(_numPat); 
	    }

	  nat ctr = 0; 
	  for(auto j = myArrNum % numProcPerChain; j < arraysThisRound ; j += numProcPerChain)
	    {
	      if(arrs[j].size() == 0 )
		arrs[j].resize(_numPat); 
	      // arrs[j] = pl.getChainLeaderComm().broadcast(arrs[j], ctr); 
	      ++ctr; 
	    }
	}

      auto arrReordereds = std::vector< std::vector<T> >(numberOfArrays); 
      for(nat i = 0; i < arrs.size() ; ++i)
	{
	  if(arrs.at(i).size() > 0 )
	    {
	      // std::cout << "reordering array " << i << std::endl; 
	      auto &arr = arrs[i]; 
	      auto &arrReordered = arrReordereds[i] ; 
	      arrReordered.resize(arrs[i].size()); 
	      auto indexPerProc = displPerProc; 

	      auto ass = pAss.getAssignment(); 
	      for(auto assignmentPair : ass)
		{
		  auto assignment = std::get<1>(assignmentPair); 
		  auto &partition  = _partitions.at(assignment.partId);
		  auto beg = partition.getLower() + assignment.offset; 
		  std::copy(begin(arr) + beg, begin(arr) + beg + assignment.width, begin(arrReordered) + indexPerProc.at(assignment.procNum)); 
		  indexPerProc[assignment.procNum] += assignment.width; 
		}
	      for(nat j = 1; j < indexPerProc.size(); ++j)
		assert(displPerProc[j] == indexPerProc[j-1]); 
	      assert( indexPerProc.back() == int(arrReordered.size()) ); 
	      
	      arr.clear();
	      arr.shrink_to_fit();
	    }

	  auto &&res = pl.getChainComm().scatterVariableKnownLength<T>(arrReordereds[i], countsPerProc, displPerProc, i % int(numProcPerChain)); 
	  result.push_back(res);
	}
      arrsProcessed += arraysThisRound; 
    }

  return result; 
}
template std::vector<std::vector<int32_t>> ByteFile::readAndDistributeArrays(ParallelSetup& pl, PartitionAssignment& pAss, size_t numberOfArrays); 
template std::vector<std::vector<uint16_t>> ByteFile::readAndDistributeArrays(ParallelSetup& pl, PartitionAssignment& pAss, size_t numberOfArrays); 
template std::vector<std::vector<uint8_t>> ByteFile::readAndDistributeArrays(ParallelSetup& pl, PartitionAssignment& pAss, size_t numberOfArrays); 


std::tuple<int,int> ByteFile::parseHeader(ParallelSetup& pl)
{
  auto commBuf = std::vector<int>(3,0); 
  
  if(pl.isGlobalMaster())
    {
      if(not _in)
	{
	  tout << "Error: could not find file: " << _fileName << std::endl; 
	  exitFunction(-1, false) ; 
	}

    }

  checkMagicNumber();
  
  seek(Position::HEADER); 
  
  int numTax = readVar<int>();
  int numPart = readVar<int>();
  _numPat = readVar<// decltype(_numPat)
    int
		    >();
  int lenOfWeights = readVar<int>(); 

  _weightType = IntegerWidth(lenOfWeights); 

  return std::make_pair(numTax, numPart); 
}




void ByteFile::parse(ParallelSetup& pl )
{
  auto numTax = int(0); 
  auto numPart = int(0); 
  std::tie(numTax, numPart) = parseHeader(pl);

  // must be done here, because of the variable length strings
  parseTaxa(numTax); 
  parsePartitions(numPart);

  auto size = pl.getChainComm().size(); 
  auto pAss = PartitionAssignment(size); 
  pAss.assign(_partitions);

  switch(_weightType)
    {    
    case IntegerWidth::BIT_8: 
      parseWeights<uint8_t>(pl, pAss);
      break; 
    case IntegerWidth::BIT_16: 
      parseWeights<uint16_t>(pl, pAss);
      break; 
    case IntegerWidth::BIT_32: 
      parseWeights<uint32_t>(pl, pAss);
      break; 
    case IntegerWidth::BIT_64: 
      assert(0); 
      break; 
    default:  
      assert(0); 
    }

  parseAlnsDirectRead(pl, pAss); 
}



void ByteFile::parseAlnsDirect_newLayout(ParallelSetup& pl, PartitionAssignment& pAss )
{
  seek(Position::ALIGNMENT); 
  auto alnPos = _in.tellg();

  // auto pureIo = 0.; 
  // auto pureReorder = 0.;  
  // auto pureParsimony = 0.; 
  // auto pureCommTime = 0.; 

  auto range = pAss.getAssignment().equal_range(pl.getChainComm().getRank()); 

  for(auto iter = range.first; iter != range.second; ++iter )
    {
      auto assignment = iter->second; 
      auto &partition = _partitions.at(assignment.partId); 
      auto lenOfPat = sizeof(uint8_t) * _taxa.size(); 

      auto pos = alnPos + std::streamoff(lenOfPat *  (partition.getLower() + assignment.offset)); 
      _in.seekg( pos ); 
      
      // auto tp = getTimePoint();
      auto alnTmp = readArray<uint8_t>(assignment.width * lenOfPat);
      // pureIo += getDuration(tp); 
      
      auto aln = aligned_malloc<uint8_t, size_t(EXA_ALIGN)>(assignment.width * _taxa.size()); 

      // tp = getTimePoint(); 
      auto numTax =  _taxa.size(); 
      for(auto j = 0u;  j < assignment.width; ++j)
	{
	  auto iter2 = alnTmp.data() + j * numTax; 
	  for(auto i = 0u; i < numTax; ++i)
	    aln[ i * assignment.width + j ]  = iter2[i]; 
        }
      // pureReorder += getDuration(tp); 

      // parse information about parsimony informativeness; it
      // probably would be easier to read the entire bit vector
      // instead of cutting out a part of it =/
      seek(Position::PARSIMONY); 
      auto startPos = partition.getLower() + assignment.offset; 
      auto endPos = startPos + assignment.width; 
      auto startInt =    startPos   / 32  ; 
      // auto endInt = endPos / 32; 
      auto intWidth = assignment.width / 32   ; 
      bool addAtStart = (startPos  % 32)  != 0; 
      bool addAtEnd = (endPos % 32) != 0; 
      if(addAtStart || addAtEnd )
	++intWidth; 
      _in.seekg(startInt * sizeof(uint32_t), std::ios::cur);

      auto bv = readArray<uint32_t>(intWidth); 
      auto bp = Bipartition(); 
      bp.setRawBip(bv); 
      
      auto remainder = startPos % 32; 

      auto parsInfo = std::vector<bool>(assignment.width,true);

      for(auto i = 0u ; i < assignment.width ; ++i)
	{
	  if(not bp.isSet(i + remainder ))
	    parsInfo.at(i) = false; 
	}

#if 0 
      partition.setParsimonyInformative(parsInfo); 
#else 

      // TODO !!!! 
      // tout << "NOT setting parsimony information read from file "  << std::endl; 
#endif

      // tp = getTimePoint(); 
      partition.setAlignment(shared_pod_ptr<uint8_t>(aln), assignment.width); 
      // pureParsimony += getDuration(tp); 
    }

  // tout << "===> pure io: " << pureIo << " sec" << std::endl; 
  // tout << "===> pure reorder: "  << pureReorder << " sec" << std::endl; 
  // tout << "===> pure pars: " << pureParsimony << " sec" << std::endl; 
  // tout << "===> pure comm: "<< pureCommTime << " sec" << std::endl; 
}


void ByteFile::parseAlnsDirectRead(ParallelSetup& pl, PartitionAssignment& pAss)
{
  seek(Position::ALIGNMENT);

  // auto itime = getTimePoint(); 
  // double mallocTime = 0; 
  // double ioTime = 0; 

  auto alnPos = _in.tellg();
  auto range = pAss.getAssignment().equal_range(pl.getChainComm().getRank()); 
  for(auto iter = range.first; iter != range.second; ++iter)
    {
      auto assignment =  iter->second; 
      auto &partition = _partitions.at(assignment.partId); 
      // itime = getTimePoint(); 
      auto aln = aligned_malloc<uint8_t, size_t(EXA_ALIGN) >(assignment.width * _taxa.size());
      // mallocTime += getDuration(itime); 
      
      nat ctr = 0; 
      for(nat i = 0; i < _taxa.size(); ++i)
	{
	  auto start = size_t(alnPos) + ctr * _numPat +  partition.getLower() + assignment.offset; 
	  _in.seekg(start);
	  
	  // itime = getTimePoint(); 
	  readArray( assignment.width, aln +   assignment.width * ctr  ); 
	  // ioTime += getDuration(itime); 
	  ++ctr; 
	}

      // tout << aln << std::endl; 

      partition.setAlignment(shared_pod_ptr<uint8_t>(aln), assignment.width); 
    }

  // tout << "[ "  << getDuration(_initTime) << "] computed parsimony" << std::endl; 
  // tout << "io-time: " << ioTime << "\tmallocTime" << mallocTime << std::endl; 
  // _initTime = getTimePoint();

}


void ByteFile::parseAlns(ParallelSetup& pl, PartitionAssignment& pAss)
{
  seek(Position::ALIGNMENT); 
  
  auto myAln = std::vector< std::vector<unsigned char> >(); 

  myAln = readAndDistributeArrays<unsigned char>(pl, pAss ,_taxa.size()); 

  // tout << "[ "  << getDuration(_initTime) << "] distributed alignment" << std::endl; 
  // _initTime = getTimePoint();

  auto range = pAss.getAssignment().equal_range(pl.getChainComm().getRank()); 
  nat pos = 0; 

  for(auto iter = range.first ; iter != range.second; ++iter)
    {
      auto assignment = iter->second; 
      auto &partition = _partitions.at(assignment.partId); 
      auto aln = aligned_malloc<unsigned char, size_t (EXA_ALIGN) >(assignment.width * _taxa.size());
      

      nat ctr = 0; 
      for(auto myAlnRow : myAln)
	{
	  std::copy(begin(myAlnRow) + pos , begin(myAlnRow) + pos + assignment.width, aln  + ctr * assignment.width );
	  ++ctr; 
	}

      pos += assignment.width; 
      partition.setAlignment(shared_pod_ptr<unsigned char>(aln), assignment.width); 
    }

  // tout << "[ "  << getDuration(_initTime) << "] computed parsimony" << std::endl; 
  // _initTime = getTimePoint();
}







template<typename T> 
void ByteFile::parseWeights(ParallelSetup& pl, PartitionAssignment &pAss)
{
  seek(Position::WEIGHTS ); 

  auto assigns = pAss.getAssignment(); 
  auto wgtPos = std::streamoff(_in.tellg()); 
  auto range = assigns.equal_range(pl.getChainComm().getRank() ); 

  for(auto iter = range.first; iter != range.second; ++iter)
    {
      auto assignment = iter->second; 
      auto wgts = aligned_malloc<int, size_t(EXA_ALIGN)>(assignment.width);
      auto arrForReading = std::vector<T>(assignment.width,0); 
      auto &partition = _partitions.at(assignment.partId); 

      _in.seekg(wgtPos + ( partition.getLower() + assignment.offset ) * sizeof(T));
      readArray<T>( assignment.width , arrForReading.data() ); 

      // more efficient copy possible? probably not, since we change the size of the integer 

      for(nat i = 0; i < arrForReading.size(); ++i) 
	wgts[i] = arrForReading[i]; 

      partition.setWeights(shared_pod_ptr<int>(wgts), assignment.width); 
    }
} 
template void ByteFile::parseWeights<uint32_t>(ParallelSetup& pl, PartitionAssignment &pAss); 
template void ByteFile::parseWeights<uint16_t>(ParallelSetup& pl, PartitionAssignment &pAss); 
template void ByteFile::parseWeights<uint8_t>(ParallelSetup& pl, PartitionAssignment &pAss); 

 
void ByteFile::parseTaxa(int numTax)
{
  seek(Position::TAXA); 

  for(int i = 0; i < numTax; ++i)
    {
      // TODO this is one mess with the zero byte ... 

      int len = readVar<int>(); 
      auto arr = readArray<char>(len-1); 
      auto str = std::string(begin(arr), end(arr)); 
      _taxa.push_back(str);

      readVar<char>(); 
    }
}


void ByteFile::parsePartitions(int numPart)
{
  seek(Position::PARTITIONS); 

  // read partition contributions 
  auto pCont = readArray<double>(numPart); 

  for(int i = 0; i < numPart; ++i)
    {
      pInfo p; 

      auto states = readVar<decltype(p.states)>();      
      auto maxTipStates = readVar<decltype(p.maxTipStates)>(); 
      auto lower = readVar<decltype(p.lower)> (); 
      auto upper = readVar<decltype(p.upper)>(); 
      auto dataType = readVar<decltype(p.dataType)>();
      auto protModel = readVar<decltype(p.protModels)>(); 
      auto protFreqs = readVar<decltype(p.protUseEmpiricalFreqs)>(); 
      auto nonGTR = readVar<decltype(p.nonGTR)>(); 
      auto len = readVar<int>(); 

      auto arr = readArray<char>(len-1);  
      auto str =std::string(begin(arr), end(arr)); 
      readVar<char>(); 

      auto partition =  Partition(nat(_taxa.size()), str, dataType, states, maxTipStates, _saveMemory); 

      partition.setPartitionContribution(pCont.at(i)); 
      partition.setLower(lower);
      partition.setUpper(upper);
      partition.setWidth(0); // width
      partition.setProtModel(protModel); 
      partition.setProtFreqs(protFreqs); 
      partition.setNonGTR(nonGTR); 
      partition.setDataType(dataType); 

      _partitions.push_back(partition); 

      // tout << "parsed partition " << i << ": " << partition.getLower() << "\t" << partition.getUpper() << "\twidth" << partition.getWidth()  << std::endl; 
    }

  // tout << "after parsing partitions, pos was "<< _in.tellg() << std::endl;
}

 
void ByteFile::seek(Position pos)
{
  pInfo ps; 
  size_t toSkip = 6; 		// always skip the magic 
  switch(pos) 
    {
    case Position::PARSIMONY: 	// skip alignment 
      {
	size_t toSkipHere = _taxa.size() * _numPat * sizeof(uint8_t) ; 
	toSkip += toSkipHere; 
      }
    case Position::ALIGNMENT: 	// skips partitions
      {
	size_t toSkipHere = 0; 

	// skip partition contributios 
	toSkipHere += _partitions.size() * sizeof(double); 

	auto oneElem = 
	  sizeof(ps.states) + sizeof(ps.maxTipStates) + sizeof(ps.lower) + sizeof(ps.upper) + 
	  sizeof(ps.dataType) + sizeof(ps.protModels) + sizeof(ps.protUseEmpiricalFreqs) + sizeof(ps.nonGTR); 
	toSkipHere += _partitions.size() * (oneElem + sizeof(int)); 

	for(auto &p : _partitions)
	  toSkipHere += ( p.getName().size() +1 )  * sizeof(char) ; // TODO zero byte  

	toSkip += toSkipHere; 
      }
    case Position::PARTITIONS: 	// skips taxa
      {
	size_t toSkipHere = 0; 
	toSkipHere += _taxa.size() * sizeof(int); 
	for(auto &t : _taxa ) 
	  toSkipHere +=  ( t.size() +1) * sizeof(char); // TODO zero byte 
	toSkip += toSkipHere; 
      }
    case Position::TAXA: 
      {
	toSkip += _numPat * size_t(_weightType); // skips weights
      }
    case Position::WEIGHTS:
      {
	toSkip += sizeof(_numPat) + 3 * sizeof(int) ; // skips header 
      }
    case Position::HEADER: 
      // dont skip anything 
      break; 
    default: 
      assert(0); 
    }

  _in.seekg(toSkip, std::ios_base::beg); 
} 




void ByteFile::checkMagicNumber()  
{
  char tmp[7]; 
  _in.seekg(0, std::ios_base::beg);
  memset(&tmp, 0 , sizeof(char) * 7 ); 
  _in.read(tmp, 6 * sizeof(char)); 
  if( std::string(tmp).compare("BINARY") != 0 )
    {
      tout << "expected different file start (maybe reparse the binary file). Got >"  << tmp << "<"<< std::endl; 
      assert(0); 
    }
}


std::vector<Partition> ByteFile::getPartitions() const 
{
  

  return _partitions; 
}

