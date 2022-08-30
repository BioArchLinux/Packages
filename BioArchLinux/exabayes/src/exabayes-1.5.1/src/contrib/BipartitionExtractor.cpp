#include "BipartitionExtractor.hpp"

#include <iostream>
#include <unordered_set>
#include <cassert>
#include <fstream>

#include "Arithmetics.hpp" 
#include "BipartitionHash.hpp"

#include "TreePrinter.hpp"
#include "BranchLengthsParameter.hpp"


static void rejectIfExists(std::string filename)
{
  if( std::ifstream(filename) ) 
    {
      std::cerr << std::endl <<  "File " << filename << " already exists (probably \n"
		<< "from previous run). Please choose a new run-id or remove previous output files. " << std::endl; 
      exitFunction(-1, false); 
    }
}


// does not include length of tips currently   
BipartitionExtractor::BipartitionExtractor(std::vector<std::string> files, bool extractToOneHash, bool expensiveCheck)
  : TreeProcessor(files, expensiveCheck)
  , _bipHashes{}
  , _uniqueBips{}
  , _extractToOneHash(extractToOneHash)
{
  assert(_taxa.size( ) != 0); 
}


nat BipartitionExtractor::getNumTreesInFile(std::string file) const 
{
  nat result = 0; 
  
  auto && fh = std::ifstream(file); 
  auto line = std::string();
  while(getline(fh, line))
    {
      if(line.compare("end;") == 0 )
	break;

      if(line.find(";") != std::string::npos) 
	++result; 
    }

  if(result < 1)
    {
      std::cout << "Header of file " << file << " possibly broken. Expected list of taxa terminated by semi-colon." << std::endl; 
      exitFunction(-1, false); 
    }

  result -= 2;
  return result; 
}


template<bool readBl>
void BipartitionExtractor::extractBips(nat burnin)
{
  int ctr = 0; 
  for (auto filename : _fns)
    {
      auto &&ifh = std::ifstream(filename); 
      
      nat end = getNumTreesInFile(filename); 

      if( not _extractToOneHash || _bipHashes.size() == 0)
	_bipHashes.emplace_back(_tralnPtr->getNumberOfTaxa());
      auto &bipHash = _bipHashes.back(); 

      // skip the burnin 
      nat i = 0; 
      for( ; i < burnin; ++i)
	skipTree(ifh); 

      for( ; i < end; ++i)
	{
	  nextTree<readBl>(ifh);
	  bipHash.addTree(*_tralnPtr,true, true);
	}
      
      ++ctr; 
    }

  extractUniqueBipartitions();
}


void BipartitionExtractor::extractUniqueBipartitions()
{
  auto set = std::unordered_set<Bipartition>{};

  for(auto &bipHash : _bipHashes)
    for(auto &bip : bipHash)
      set.insert(std::get<0>(bip)); 

  assert(_uniqueBips.size() == 0); 
  
  nat ctr = 0; 
  for(auto &bip :set)
    {
      _uniqueBips[bip] = ctr; 
      ++ctr; 
    }
}


void BipartitionExtractor::printBipartitionStatistics(std::string id) const 
{
  assert(_uniqueBips.size() != 0); 

  auto && ss = std::stringstream{}; 
  ss << PROGRAM_NAME << "_bipartitionStatistics." << id ; 
  rejectIfExists(ss.str()); 

  auto && freqFile = std::ofstream (ss.str()); 
  freqFile << "id\t";

  freqFile
    << "freq"
    << "\tbl.mean"
    << "\tbl.sd"
    << "\tbl.cv"
    << "\tbl.skew"
    << "\tbl.ESS"
    << "\tbl.perc5"
    << "\tbl.perc25"
    << "\tbl.perc50"
    << "\tbl.perc75"
    << "\tbl.perc95"
    ; 

  if(_fns.size() > 1)
    freqFile << "\tbl.prsf" ; 
  freqFile << std::endl; 

  for(auto &bipElem: _uniqueBips)
    {
      auto allBls = std::vector<std::vector<double>>{};      
      for(auto &bipHash : _bipHashes )
	allBls.push_back(bipHash.getBranchLengths(bipElem.first)); 

      auto allBlsConcat = std::vector<double>{}; 
      for(auto &allBl : allBls)
	allBlsConcat.insert(end(allBlsConcat), begin(allBl), end(allBl));
      
      assert(allBls.size() >  0); 

      auto num = allBlsConcat.size() ; 
      auto mean = Arithmetics::getMean(allBlsConcat) ; 
      auto sd = sqrt(Arithmetics::getVariance(allBlsConcat) ); 
      auto cv = Arithmetics::getCoefficientOfVariation(allBlsConcat); 
      auto skew = Arithmetics::getSkewness(allBlsConcat);
      auto ess = Arithmetics::getEffectiveSamplingSize(allBlsConcat) ; 
      auto perc5= Arithmetics::getPercentile(0.05,allBlsConcat) ; 
      auto perc25 = Arithmetics::getPercentile(0.25,allBlsConcat)  ; 
      auto perc50 = Arithmetics::getPercentile(0.50,allBlsConcat) ; 
      auto perc75 = Arithmetics::getPercentile(0.75,allBlsConcat) ; 
      auto perc95 = Arithmetics::getPercentile(0.95,allBlsConcat) ; 

      freqFile << bipElem.second // the id 
	       << "\t" << num 
	       << "\t" << mean
	       << "\t" << sd 
	       << "\t" << cv
	       << "\t" << skew
	       << "\t" << ess 
	       << "\t" << perc5 
	       << "\t" << perc25
	       << "\t" << perc50 
	       << "\t" << perc75
	       << "\t" << perc95
	; 
	
      if(_fns.size() > 1)
	freqFile << "\t" << Arithmetics::PRSF(allBls); 

      freqFile << std::endl; 
    }
  
  std::cout << "printed bipartition statistics to file " << ss.str() << std::endl; 
}


void BipartitionExtractor::printBranchLengths(std::string id) const 
{
  assert(_uniqueBips.size() != 0); 
  
  auto &&ss = std::stringstream {}; 
  ss << PROGRAM_NAME << "_bipartitionBranchLengths."  << id; 
  rejectIfExists(ss.str()); 

  auto&& blFile = std::ofstream(ss.str());

  blFile << MAX_SCI_PRECISION; 
  blFile  << "bipId\tfileId\tlength"  << std::endl;
  nat ctr = 0; 
  for(auto &bipHash : _bipHashes)
    {
      for(auto &bip : bipHash)
	{
	  for(auto length : bipHash.getBranchLengths(bip.first) )
	    {
	      blFile << _uniqueBips.at(bip.first) << "\t"  << ctr  << "\t" << length << std::endl; 
	    }
	}
      ++ctr ; 
    }

  std::cout << "printed branch lengths associated with bipartitions to " << ss.str() << std::endl; 
}

void BipartitionExtractor::printFileNames(std::string id) const 
{
  assert(_uniqueBips.size() != 0); 

  auto &&ss = std::stringstream{}; 
  ss << PROGRAM_NAME << "_fileNames." << id; 
  rejectIfExists(ss.str()); 

  auto  &&ff = std::ofstream(ss.str());
  ff << "id\tfileName" << std::endl; 
  nat ctr = 0; 
  for(auto &fn : _fns)
    {
      ff << ctr << "\t" << fn << std::endl; 
      ++ctr; 
    }

  std::cout << "printed file name identifiers (for future reference) to "<< ss.str() << std::endl; 
}


void BipartitionExtractor::printBipartitions(std::string id) const 
{
  assert(_uniqueBips.size() != 0); 

  auto &&ss = std::stringstream{}; 
  ss << PROGRAM_NAME << "_bipartitions." << id; 
  rejectIfExists(ss.str()); 

  auto &&out = std::ofstream(ss.str());

  for(auto &bipElem :  _uniqueBips)
    {
      auto &bip = std::get<0>(bipElem); 
      auto myId = std::get<1>(bipElem); 

      assert(bip.count() != 0 ); 
      
      out << myId << "\t" ; 
      bip.printVerbose(out, _taxa);

      out << std::endl;       
    }

  std::cout << "printed bipartitions and identifiers to file " << ss.str() << std::endl; 
}


std::string BipartitionExtractor::bipartitionsToTreeString(std::vector<Bipartition> bips, bool printSupportLocal, bool printBranchLengthsLocal, bool phylipStyle) const 
{
  // contains ids of direct children 
  auto directSubBips  = std::vector<std::vector<nat> >(bips.size()); 

  std::sort(bips.begin(), bips.end(),
	    [](const Bipartition& bipA, const Bipartition& bipB)
	    {
	      return bipA.count() < bipB.count() ; 
	    } ); 

  auto toplevelBip = Bipartition();
  const auto& taxa = getTaxa();	
  toplevelBip.reserve(taxa.size()); 

  for(nat i = 0; i < taxa.size(); ++i)
    toplevelBip.set(i); 
  auto belowTop = std::vector<nat>{}; 

  // search the parent bipartition for each bipartition 
  for(nat i = 0; i < bips.size(); ++i)
    {
      bool parentFound = false; 
      const auto &bipA = bips[i]; // a <=> i
      for(nat j = i+1 ; j < bips.size() ;++j)
	{
	  const auto &bipB = bips[j];  // b <=> j 
	  if(bipA.isSubset(bipB))
	    {

	      directSubBips[j].push_back(i);
	      parentFound = true ; 
	      break; 
	    }
	}
      
      if( not parentFound )
	belowTop.push_back(i);
    }

  bips.push_back(toplevelBip); 
  directSubBips.push_back(belowTop); 

  auto&& ss = std::stringstream{}; 
  buildTreeRecursive( nat (bips.size()-1), directSubBips, bips, ss, printSupportLocal, printBranchLengthsLocal, phylipStyle); 

  return ss.str(); 
} 



void BipartitionExtractor::buildTreeRecursive(nat currentId, const std::vector<std::vector<nat> >& directSubBips, 
					      const std::vector<Bipartition> &bips, std::stringstream &result, 
					      bool printSupportLocal, bool printBranchLengthsLocal, bool phylipStyle) const
{
  assert(_bipHashes.size() ==  1); 
  nat totalTrees = _bipHashes[0].getTreesAdded();
  auto taxa = getTaxa(); 
  
  const auto& curBip = bips.at(currentId); 
  auto children = directSubBips.at(currentId); 

  double mySupport = 0 ; 

  if(currentId != bips.size() -1 )
    {
      auto lengths = _bipHashes[0].getBranchLengths(curBip);
      mySupport = double(_bipHashes[0].getPresence(curBip).count()) / totalTrees; 
      assert(mySupport > 0. && mySupport <= 1.); 
    }

  // can only be one taxon, print it 
  bool isTaxon = children.size() == 0; 
  if(isTaxon )
    {
      //  TODO an efficient implementation (relevant for credibleset)
      // would do that only once ...
      auto ind = curBip.findIndex(); 
      
      if(phylipStyle)
	result << taxa.at(ind); 
      else 
	{
	  // ids are 1-based
	  result << ind + 1 ; 
	}
      assert(curBip.count() == 1 ); 
    }
  else 
    {
      result << "("; 
      bool isFirst = true; 

      for(auto childId : children)
	{
	  result << ( isFirst ? "" : ",") ; 
	  buildTreeRecursive(childId,directSubBips, bips, result, printSupportLocal, printBranchLengthsLocal, phylipStyle); 
	  isFirst = false;  
	}

      result << ")"; 
    }


 if(currentId != bips.size()  -1 )
   {
     if(printSupportLocal)
       {
	 if(phylipStyle)
	   {
	     if(not isTaxon)
	       {
		 result << "" << SOME_FIXED_PRECISION << mySupport << ""; 
	       }
	   }
	 else 
	   {
	     result << "[&prob=" << MAX_SCI_PRECISION   << mySupport
		    << ",prob(percent)=\"" << std::fixed << std::setprecision(0) << mySupport * 100  << "\"" 
		    << "]"; 
	   }
       }

     if(printBranchLengthsLocal)
       {
	 assert(_bipHashes.size() == 1); 
	 auto lengths = _bipHashes[0].getBranchLengths(curBip);
	 auto median = Arithmetics::getPercentile(.5, lengths);
	 auto mean = Arithmetics::getMean(lengths); 
	 auto p95 = Arithmetics::getPercentile(.95, lengths); 
	 auto p5 = Arithmetics::getPercentile(.05, lengths); 
	 if(phylipStyle	)
	   result << MAX_SCI_PRECISION << ":" << median ; 
	 else 
	   result << MAX_SCI_PRECISION << ":" << median << "[&length_mean="<< mean << ",length_median=" << median << ",length_95%HDP={" << p5 << "," << p95 << "}]" ; 
       }
   }
 else 
   result << ";";
}

template void BipartitionExtractor::extractBips<true>(nat burnin); 
template void BipartitionExtractor::extractBips<false>(nat burnin); 
