#include "ConsensusTree.hpp"
#include <limits>
#include <iomanip>
#include <map> 
#include <cassert> 
#include <algorithm>


ConsensusTree::ConsensusTree(std::vector<std::string> files, double burnin, double threshold, bool isMRE  )
  : bipEx(files, true, true)
  , _threshold{threshold}
  , _isMRE{isMRE}
{
  auto nums = std::vector<nat>{};

  for( auto &file : files)
    nums.push_back( bipEx.getNumTreesInFile(file)); 

  nat minTrees = *(std::min_element(begin(nums), end(nums)));
  nat absBurnin = int(double(minTrees) * burnin); 
  bipEx.extractBips<true>(absBurnin); 
}


std::vector<Bipartition> ConsensusTree::getRefinedConsensusTree(const std::vector<Bipartition> &consensusBips, const std::vector<Bipartition> &minorityBips) const 
{
  auto result = consensusBips; 
  auto maxBipsNeeded =  2 * nat(bipEx.getTaxa().size())  - 3 ; 
  auto maxElem = bipEx.getTaxa().size(); 
  
  for(auto minBip : minorityBips)
    {
      if(result.size() >= maxBipsNeeded)
	break; 

      auto isCompat = true; 
      for(auto &bip : result)
	{
	  isCompat &= bip.isCompatible(minBip, int(maxElem));
	  if(not isCompat)
	    break; 
	}
      
      if(isCompat)
	result.push_back(minBip); 
    }

  return result; 
}


std::string ConsensusTree::getConsensusTreeString( bool printNames) const 
{
  auto result = std::string{}; 
  const auto& bipHashes = bipEx.getBipartitionHashes();

  auto bip2Occ = std::unordered_map<Bipartition,nat>{}; 
  auto maxBip = 2 * bipEx.getTaxa().size() - 3; 

  nat totalTreesAdded = 0; 
  for(auto &bipHash : bipHashes)
    totalTreesAdded += bipHash.getTreesAdded(); 

  nat absThreshold = 0; 
  if(_isMRE)
    absThreshold = nat(ceil(totalTreesAdded * .5));
  else 
    absThreshold = nat(ceil(totalTreesAdded * _threshold)); 

  assert(absThreshold > 0. ); 

  // determine unique bipartitions  
  for(auto &bipHash : bipHashes)
    {
      for(auto &bipOccPair : bipHash)
	{
	  auto &bip = std::get<0>(bipOccPair); 
	  if( bip2Occ.find(bip) == bip2Occ.end())
	    bip2Occ[bip] = 0; 
	}
    }
  
  for(auto &bipOccPair : bip2Occ)
    {
      auto &bip = std::get<0>(bipOccPair); 
      auto &occ = std::get<1>(bipOccPair); 
      for(auto bipHash : bipHashes)
	occ += bipHash.getPresence(bip).count(); 
    }

  // sort the bipartitions 
  auto sortBySecond = [](const std::pair<Bipartition,nat> &elemA, const std::pair<Bipartition,nat> &elemB)
    { 
      return elemA.second < elemB.second; 
    }; 

  auto bip2occVect = std::vector<std::pair<Bipartition,nat> >{bip2Occ.begin(), bip2Occ.end()}; 
  std::sort(bip2occVect.begin(), bip2occVect.end(), sortBySecond); 

  auto consensus = std::vector<Bipartition>{}; 
  auto minorityBips = std::vector<Bipartition>{}; 

  for(auto &bipPair : bip2occVect)
    {
      if(absThreshold <= bipPair.second )
	consensus.push_back(bipPair.first); 
      else 
	minorityBips.push_back(bipPair.first); 
    }

  assert(consensus.size() <= maxBip ); 

  if(_isMRE)
    consensus = getRefinedConsensusTree(consensus, minorityBips); 

  assert(consensus.size() <= maxBip ); 

  result = bipEx.bipartitionsToTreeString(consensus, true, true, printNames); 

  return result; 
}




std::string ConsensusTree::getTreeHeader() const
{
  auto taxa = bipEx.getTaxa();

  auto&& oss = std::ostringstream{}; 
  oss << "#NEXUS\n" ; 
  oss << "begin taxa;\n"; 
  oss << "\tdimensions ntax="  << taxa.size() ; 
  oss << ";\n"; 
  oss << "\ttaxlabels\n"; 
  for(auto t : taxa)
    {
      oss << "\t\t" << t << "\n"; 
    }
  oss << "\t;\n"; 
  oss << "end;\n" ; 
  oss << "begin trees;\n"; 
  oss << "\ttranslate\n"; 
  nat ctr = 1; 
  for(auto t : taxa)
    {
      oss << "\t\t" << ctr << "\t" << t << ( ctr == taxa.size() ? ";"  : "," ) << "\n"; 
      ++ctr;
    }


  auto type = getType();

  oss << "\ttree " << type << " = [&U] "; 
  return oss.str(); 
} 



std::string ConsensusTree::getType() const 
{
  auto type = std::string{};
  if(_isMRE)
    {
      type = "ConsensusExtendedMajorityRule" ; 
    }
  else 
    {
      if( std::fabs(_threshold -1. )  < std::numeric_limits<double>::epsilon() ) 
	type = "ConsensusStrict" ; 
      else if( std::fabs( _threshold - 0.5 ) <  std::numeric_limits<double>::epsilon())
	type = "ConsensusMajorityRule" ; 
      else 
	{
	  auto &&ss = std::ostringstream{}; 
	  ss << "ConsensusThreshold" << std::setprecision(0) << std::fixed << nat(_threshold * 100.); 
	  type = ss.str();
	}
    }
  return type; 
}

