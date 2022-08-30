#include <sstream>
#include <fstream>
#include <memory>
#include <algorithm>

#include "ByteFile.hpp"
#include "TreePrinter.hpp"
#include "BasicTreeReader.hpp"
#include "BlockProposalConfig.hpp"
#include "BlockRunParameters.hpp"
#include "ConfigReader.hpp"

#include "TopologyParameter.hpp"
#include "RevMatParameter.hpp"	
#include "FrequencyParameter.hpp"

#include "MemoryMode.hpp"
#include "SampleMaster.hpp"
#include "TreeRandomizer.hpp"
#include "RunFactory.hpp"	

#include "GlobalVariables.hpp"

#include "BoundsChecker.hpp"
#include "ArrayRestorer.hpp"
#include "SplitFreqAssessor.hpp"

#include "FullCachePolicy.hpp"
#include "NoCachePolicy.hpp"


#include "AlignmentPLL.hpp"

#include "NodeAge.hpp"

#include "DivergenceTimes.hpp"
#include "DivergenceRates.hpp"

using std::endl; 
using std::vector;
using std::unique_ptr; 
using std::move; 
using std::get; 


SampleMaster::SampleMaster( size_t numCpu) 
  : _runs{}
  , _plPtr{nullptr}
  , _runParams{}
  , _cl{}
  , _diagFile{}
  , _timeTracker{numCpu}
  , _printTime{numCpu}
{
}


bool SampleMaster::initializeTree(TreeAln &traln, std::string startingTree, Randomness &treeRandomness, const vector<AbstractParameter*> &params)
{  
  bool hasBranchLength = false; 
  if(startingTree.compare("") != 0 )
    {
      pllInstance * instance = &(traln.getTrHandle());
      pllNewickTree * newickTree = pllNewickParseString(startingTree.c_str());
      pllTreeInitTopologyNewick(instance, newickTree, PLL_TRUE);
    }
  else
    {	      
      if(_runParams.isUseParsimonyStarting())
	TreeRandomizer::createParsimonyTree(traln, treeRandomness, *_plPtr); 
      else
	TreeRandomizer::randomizeTree(traln, treeRandomness); 
    }

  return hasBranchLength; 
}



std::vector<bool> 
SampleMaster::initTrees(vector<TreeAln> &trees, randCtr_t seed, vector<std::string> startingTreeStrings, const vector<AbstractParameter*> &params)
{  
  auto hasBl = vector<bool>{}; 

  nat treesConsumed = 0; 
  auto treeRandomness = Randomness(seed); 

  auto treesToInitialize = vector<TreeAln*>{}; 
  treesToInitialize.push_back(&(trees[0])); 
  if(not _runParams.isHeatedChainsUseSame())
    {
      for(auto iter =  trees.begin() + 1  ; iter < trees.end(); ++iter)
	treesToInitialize.push_back(&(*iter)); 
    }

  // choose how to initialize the topology
  for(auto &tralnPtr : treesToInitialize)
    {
      auto stTr = treesConsumed < startingTreeStrings.size() ? startingTreeStrings.at(treesConsumed) : std::string{""}; 
      ++treesConsumed;
      
      auto hadBl = initializeTree(*tralnPtr, stTr, treeRandomness, params); 
      hasBl.push_back(hadBl); 
    }

  // propagate the tree to the coupled chains, if necessary
  if(_runParams.isHeatedChainsUseSame())
    {
      auto &ref =  trees[0]; 
      bool isFirst = true; 
      for(auto& treePtr : trees)
	{
	  if(isFirst )
	    isFirst = false; 
	  else 
	    treePtr  = ref;
	}
    }
  
  return hasBl; 
}


void SampleMaster::printAlignmentInfo(const TreeAln &traln)
{
  tout << endl << "The (binary) alignment file you provided, consists of the following\n" 
       << "partitions:" 
       << endl;
  
  for(nat i = 0 ;i < traln.getNumberOfPartitions() ;++i)
    {
      auto& partition = traln.getPartition(i);       
      nat length = partition.getUpper() - partition.getLower(); 
      
      tout << endl; 

      tout << "number:\t\t" << i << endl;     
      tout << "name:\t\t" << partition.getName() << endl; 
      tout << "#patterns:\t" << length << endl;       

      switch(partition.getDataType())
	{
	case PLL_BINARY_DATA : 
	  tout << "type:\t\tBINARY" << std::endl; 
	  break; 
	case PLL_DNA_DATA: 
	  tout << "type:\t\tDNA" << endl; 
	  break; 
	case PLL_AA_DATA: 
	  tout << "type:\t\tAA" << endl; 
	  break; 
	default : 
	  assert(0); 
	}      
    }
  tout << "\nParameter names will reference the above partition numbers."
       << endl
       << "================================================================"
       << endl  ; 
}


void SampleMaster::informPrint()
{
  if(not isYggdrasil && _plPtr->getRunsParallel()  > 1)
    {
      tout << "Will execute "<< _plPtr->getRunsParallel() << " runs in parallel." << endl;       
      if(nat (_runParams.getNumRunConv()) < _plPtr->getRunsParallel())
	{
	  tout 
	    << "Error: in the configuration file you specified to run " <<  _runParams.getNumRunConv() << " independent\n" 
	    << "runs. Your command line indicates, that you want to run " << _plPtr->getRunsParallel() << " of them\n"
	    << "in parallel. To shield you from surprises, " << PROGRAM_NAME << " will conservatively abort."<< endl; 
	  exitFunction(-1, true); 
	}
    }

  if(not isYggdrasil && _plPtr->getChainsParallel() > 1)
    {
      tout << "Will execute " << _plPtr->getChainsParallel() << " chains in parallel."<< endl; 
      if(nat(_runParams.getNumCoupledChains()) < _plPtr->getChainsParallel())	
	{
	  tout
	    << "Error: in the configuration file you specified to run " <<  _runParams.getNumCoupledChains() << " coupled\n" 
	    << "chains per run. Your command line indicates, that you want to run " << _plPtr->getChainsParallel() << " of them\n"
	    << "in parallel. To shield you from surprises, " << PROGRAM_NAME << " will conservatively abort."<< endl; 
	  exitFunction(-1, true); 
	}
    }

  tout << endl; 
}


void SampleMaster::initializeFromCheckpoint()
{
  // continue from checkpoint  

  auto prevId = _cl.getCheckpointId(); 
      
  auto &&ss = std::stringstream{} ;       
  ss << _cl.getWorkdir() << ( _cl.getWorkdir().compare("") == 0  ? "" : "/") 
     << PROGRAM_NAME << "_checkpoint." << _cl.getCheckpointId();       
  auto checkPointFile = ss.str(); 
  auto &&chkpnt = std::ifstream{} ; 
  Serializable::getIfstream(checkPointFile, chkpnt); 
  if( not chkpnt )
    {
      tout  << "Warning! Could not open / find file " << checkPointFile << endl; 
      tout << "Although extremely unlikely, the previous run may have been killed\n"
	   << "during the writing process of the checkpoint. Will try to recover from backup..." << endl; 
	  
      ss.str(""); 
      ss << PROGRAM_NAME << "_prevCheckpointBackup." << _cl.getCheckpointId();
      checkPointFile = ss.str();	  
      Serializable::getIfstream(checkPointFile, chkpnt); 
	  
      if( not chkpnt)
	{
	  tout << "Could not recover checkpoint file backup either. Probably there is no backup, since not enough generations have completed. Giving up. Please start a new run." << endl; 
	  exitFunction(-1, false); 
	}
      else 
	tout << "Success! You lost one checkpoint, but we can continue from the backup. " << endl; 	  
    }

  deserialize(chkpnt); 

  if(_plPtr->isGlobalMaster())
    {
      for(auto &thisRun : _runs)
	thisRun.regenerateOutputFiles(_cl.getWorkdir(), _cl.getCheckpointId());
      auto curGen = _runs[0].getChains()[0].getGeneration();
      _diagFile.regenerate(  _cl.getWorkdir(), _cl.getRunid(),  _cl.getCheckpointId(), 
			     curGen);
    }

}


// TODO could move this method somewhere else  
LikelihoodEvaluator SampleMaster::createEvaluatorPrototype(const TreeAln &initTree, bool useSEV)
{
  auto &&plcy =  unique_ptr<ArrayPolicy>();
  auto res = std::make_shared<ArrayReservoir>(useSEV); 
  
  switch(_cl.getMemoryMode())
    {
    case MemoryMode::RESTORE_ALL: 
      {
	plcy = unique_ptr<ArrayPolicy>(new FullCachePolicy(initTree, true, true));
	if(useSEV)
	  plcy->enableRestoreGapVector();
      }
      break; 
    case MemoryMode::RESTORE_INNER_TIP: 
      {
	plcy = unique_ptr<ArrayPolicy>(new FullCachePolicy(initTree, false, true));
	if(useSEV)
	  plcy->enableRestoreGapVector();
      }
      break; 
    case MemoryMode::RESTORE_INNER_INNER: 
      {
	plcy = unique_ptr<ArrayPolicy>(new FullCachePolicy(initTree, false, false));
	if (useSEV)
	  plcy->enableRestoreGapVector();
      }
      break; 
    case MemoryMode::RESTORE_NONE: 
      {
	plcy = unique_ptr<ArrayPolicy>(new NoCachePolicy(initTree)); 
      }
      break; 
    default: 
      assert(0); 
    }

  auto eval = LikelihoodEvaluator(initTree, plcy.get(),res, _plPtr); 

#ifdef DEBUG_LNL_VERIFY
  auto dT = make_shared<TreeAln>(initTree.getNumberOfTaxa(), false);
  eval.setDebugTraln(dT);
#endif

  return eval; 
}


void SampleMaster::initializeWithParamInitValues(TreeAln &traln , const ParameterList &params, bool hasBl) const 
{
  // we cannot have more than one set of branch lentghs. So if we had
  // initial branch lengths, extract them first before changing the
  // mean substition rate
  auto branches = vector<std::tuple<BranchPlain,double>>{};

  if( hasBl )
    {
      AbstractParameter* blParam = nullptr;
      for(auto param : params)
	{
	  if(param->getCategory() == Category::BRANCH_LENGTHS)
	    {
	      blParam = param; 
	      break; 
	    }
	}

      for(auto b : traln.extractBranches())
	{
	  auto len = traln.getBranch(b, blParam).toMeanSubstitutions(blParam->getMeanSubstitutionRate()); 
	  // tout << b << "\t" << len << endl; 
	  branches.emplace_back( b, len ); 
	}
    }
  else 
    {
      for(auto b : traln.extractBranches())
	branches.emplace_back(b, 0.);
    }

  //  do normal parameters first 
  for(auto &param : params)
    {
      auto cat = param->getCategory( ); 
      auto datatype =  traln.getPartition(param->getPartitions()[0] ).getDataType(); 
      
      if( not ( cat == Category::TOPOLOGY || cat == Category::BRANCH_LENGTHS ) )
	{
	  // :NOTICE: treat prot frequencies differently!
	  if(cat == Category::FREQUENCIES && datatype == PLL_AA_DATA ) 
	    {
	      for(auto p : param->getPartitions())
		{
		  auto& partition = traln.getPartition(p);
		  partition.setProtFreqs(PLL_TRUE); 
		}
	    }
	  
	  // :NOTICE: rev mat parameters for aa-partitions must be prepared here! 
	  if(cat == Category::SUBSTITUTION_RATES && datatype == PLL_AA_DATA)
	    {
	      for(auto p : param->getPartitions())
		{
		  auto &partition = traln.getPartition(p);
		  partition.setProtModel(PLL_GTR); 
		}
	    }

		if (cat != Category::DIVERGENCE_RATES)
		{
	  auto&& prior = param->getPrior(); 
	  auto content = prior->getInitialValue();

	  // TODO specific function for setting initially 
	  param->verifyContent(traln, content); 
	  // tout << "APPL " << content << endl; 

	  param->applyParameter(traln, content); 
	}
    }
    }


  // initialize branches or convert to internal representation if
  // necessary
  for(auto &param : params)
    {
      auto cat = param->getCategory(); 
      if(cat == Category::BRANCH_LENGTHS)
	{
	  param->updateMeanSubstRate(traln);

	  auto &&prior = param->getPrior();
	  auto content = prior->getInitialValue();
	  auto initVal = content.values.at(0); 
	  
	  for(auto belem : branches)
	    {
	      auto absLen =  ( hasBl  && param->getPrior()->isKeepInitData() )  
		? std::get<1>(belem) : initVal ;
	      auto b = BranchLength(std::get<0>(belem),  InternalBranchLength::fromAbsolute(absLen, param->getMeanSubstitutionRate())); 

	      if( not BoundsChecker::checkBranch(b))
		{
		  BoundsChecker::correctBranch(b); 
		  auto newLen = b.toMeanSubstitutions(param->getMeanSubstitutionRate()); 
		  tout << "Warning: had to modify branch length " << absLen << " to " << newLen << " because it violated the maximum range of branch lengths allowed." << endl; 
		}

	      traln.setBranch(b,param); 
	    }
	}
    }

  auto divRates = std::vector<AbstractParameter*>{}; 
  auto divTimes = std::vector<AbstractParameter*>{}; 

  for(auto & p : params)
    {
      if(p->getCategory() == Category::DIVERGENCE_RATES)
	divRates.push_back(p); 
      if(p->getCategory() == Category::DIVERGENCE_TIMES)
	divTimes.push_back(p); 
    }

  if(divRates.size() > 0 )
    {
      // assert(divTimes.size() == 1 ); // not more than one time parameter !
      makeTreeUltrametric(traln, divTimes, divRates);
    }
}

static double traverseDepthFromRoot(TreeAln &traln, const BranchPlain &branch, std::vector<NodeAge *> & nodeAges) {

	double currentHeight = nodeAges[branch.getPrimNode()-1]->getHeight();
	nodeAges[branch.getPrimNode()-1]->setPrimNode(branch.getPrimNode());
	nodeAges[branch.getPrimNode()-1]->setSecNode(branch.getSecNode());
	if (!traln.isTipNode(branch.getPrimNode())) {

		auto plainDescendants = traln.getDescendents(branch);

		nodeAges[plainDescendants.first.getSecNode()-1]->setHeight(currentHeight+1.0);
		nodeAges[plainDescendants.second.getSecNode()-1]->setHeight(currentHeight+1.0);

		return std::max(
				traverseDepthFromRoot(traln, plainDescendants.first.getInverted(), nodeAges),
				traverseDepthFromRoot(traln, plainDescendants.second.getInverted(), nodeAges)
				);
	} else {
		return currentHeight;
	}
}

void SampleMaster::makeTreeUltrametric( TreeAln &traln, std::vector<AbstractParameter*> divTimes, std::vector<AbstractParameter*> &divRates) const 
{

	assert(divRates.size() == 1 );
	/* initialize the rates */
  
  vector<NodeAge *> nodeAges(traln.getNumberOfNodes());
  for (nat i = 0; i < traln.getNumberOfNodes(); i++)
  {
	  nodeAges[i] = new NodeAge();
  }

  /* set root at random */
  traln.setRootBranch(traln.getAnyBranch());
 
  nodeAges[traln.getRootBranch().getPrimNode()-1]->setHeight(1.0);
  nodeAges[traln.getRootBranch().getSecNode()-1]->setHeight(1.0);

  double maxHeight = std::max(
		  traverseDepthFromRoot(traln, traln.getRootBranch(), nodeAges),
		  traverseDepthFromRoot(traln, traln.getRootBranch().getInverted(), nodeAges));

	/* correct the node heights from the tips to the root and initialize branches */
	for (auto b : nodeAges)
	{
		b->setHeight(
				traln.isTipNode(b->getPrimNode()) ?
						0 : (maxHeight - b->getHeight()));
	}

	auto rootNodeAge = NodeAge();
	rootNodeAge.setHeight(maxHeight);
	auto rootContent = ParameterContent();

	for (auto b : nodeAges)
    {
		if (b->getPrimNode() > traln.getNumberOfTaxa())
		{
			auto divtime =
					static_cast<DivergenceTimes *>(divTimes[b->getPrimNode()
							- traln.getNumberOfTaxa() - 1]);
			auto content = ParameterContent();

			/* adding both current and parental branches */
			content.nodeAges.push_back(*b);
			if (traln.isRootChild(b->getPrimNode()))
			{
				content.nodeAges.push_back(rootNodeAge);
				rootContent.nodeAges.push_back(*b);
			}
			else
			{
				content.nodeAges.push_back(*nodeAges[b->getSecNode() - 1]);
			}
			divtime->initializeParameter(traln, content);
		}
	}
      
	auto divtime = static_cast<DivergenceTimes *>(divTimes[divTimes.size()-1]);
	rootContent.nodeAges.push_back(rootNodeAge);
	divtime->initializeParameter(traln, rootContent, true);

	auto ratesContent = ParameterContent();
	for (nat i = 0; i < traln.getNumberOfNodes(); i++)
	{
		ratesContent.branchLengths.push_back(BranchLength(*nodeAges[i],InternalBranchLength(0.0)));
		delete nodeAges[i];
    }

	auto divrate = static_cast<DivergenceRates *>(divRates[0]);
	divrate->initializeParameter(traln, ratesContent);
}


vector<std::string> SampleMaster::getStartingTreeStrings()
{
  auto result =  vector<std::string>{};

  auto&& ifh = std::ifstream{_cl.getTreeFile()}; 
  if(ifh)
    {
      auto aString = std::string{}; 
      while(std::getline(ifh, aString))
	result.push_back(aString); 
    }

  return result; 
}



void SampleMaster::printProposals( std::vector<std::unique_ptr<AbstractProposal> > &proposals,  std::vector<ProposalSet> &proposalSets, ParameterList &params  ) const 
{
  // something is rotten in the state of denmark: it should not be
  // necessary to reset the parameters here again, yet if we dont, we
  // get an error.
  for(auto &p : proposals)
    p->setParams(&params); 

  for(auto &p : proposalSets)
    p.setParameterListPtr(&params); 

  double sum = 0; 
  for(auto &p : proposals)
    sum += p->getRelativeWeight(); 
  for(auto &p : proposalSets)
    sum += p.getRelativeWeight();
  
  tout << "Will employ the following proposal mixture (frequency,id,type,affected variables): " << endl; 
  for(auto &p : proposals )
    {
      tout << PERC_PRECISION << p->getRelativeWeight() / sum * 100 <<   "%\t" ; 
      tout << p->getId() << "\t" ; 
      p->printShort(tout ) ; 
      tout << endl; 
    }
  if(proposals.size() == 0)
    tout << "None." << endl; 

  tout << endl; 

  if(_runParams.isComponentWiseMH() && proposalSets.size() > 0 )
    {
      tout << "In addition to that, the following sets below will be executed \n"
	   << "in a sequential manner (for efficiency, see manual for how to\n"
	   << "disable)." << endl; 
	for(auto &p : proposalSets )
	  {
	    p.printVerboseAbbreviated(tout, sum);
	    tout << endl;       
	  }
    }
}


void SampleMaster::printParameters(const TreeAln &traln, const ParameterList &params) const 
{
  // merely some printing and we are done  
  tout << endl << "Parameters to be integrated (initial values derived from prior): " << endl; 
  tout << SOME_FIXED_PRECISION; 
  for(auto &v : params)
    {
      tout << v->getId() << "\t" << v  << endl; 
      tout << "\tsub-id:\t" << v->getIdOfMyKind() << endl; 
      tout << "\tprior:\t" << v->getPrior() << endl; 

      tout << "\tinit value:\t" ; 

      if(dynamic_cast<TopologyParameter*>(v)  != nullptr )
	{
	  auto p = v->getPrior(); 

	  if(not p->needsIntegration())
	    tout << "fixed" ; 
	  else if(_runParams.isUseParsimonyStarting())
	    tout << "parsimony" ; 
	  else 
	    tout << "random" ; 
	  
	  if(_cl.getTreeFile().compare("") != 0)
	    tout << " or given (tree file)" ; 
	  tout << endl; 
	}
      else if ( dynamic_cast<RevMatParameter*>(v)  || dynamic_cast<FrequencyParameter*>(v) )
	{
	  auto content = v->getPrior()->getInitialValue(); 
	  RateHelper().convertToSum1(content.values);
	  tout << content << endl; 
	}
      else 
	{
	  if(v->getPrior() ->isKeepInitData())
	    tout << "original value (if available) / " ; 
	  tout << v->getPrior()->getInitialValue()  << endl; 
	}
    }
  tout << "================================================================" << endl;
  tout << endl; 
}


std::string SampleMaster::getOrCreateBinaryFile() const 
{
  auto binaryAlnFile =std::string{}; 
  if( not _cl.alnFileIsBinary())
    {
      tout << "You provided an alignment file in phylip format. Trying to parse it..." << endl; 
      
      bool haveModelFile = _cl.getModelFile().compare("") != 0; 
      auto modelInfo = haveModelFile ? _cl.getModelFile( ): _cl.getSingleModel(); 

      binaryAlnFile = std::string(_cl.getWorkdir() 
				  +  ( _cl.getWorkdir().compare("") == 0 ? "" : "/"   ) 
				  +   "ExaBayes_binaryAlignment" + "." + _cl.getRunid()) ;

      if( _plPtr->isGlobalMaster() )
	{

	  auto &&phyAln = AlignmentPLL{} ; 

	  if(std::ifstream(binaryAlnFile))
	    {
	      tout << "removing previous binary alignment representation " << binaryAlnFile << endl; 
	      remove(std::string(binaryAlnFile).c_str()); 
	    }

          auto format = AlignmentPLL::guessFormat(_cl.getAlnFileName());
	  phyAln.initAln(_cl.getAlnFileName(), format);

	  if(not haveModelFile)
	    {
	      auto type = getTypeFromString(modelInfo); 
	      phyAln.createDummyPartition(type); 
	    }
	  else 
	    {
	      phyAln.initPartitions(modelInfo); 
	    }

	  phyAln.writeToFile(binaryAlnFile); 
	}

      _plPtr->getGlobalComm().waitAtBarrier();

      if(not std::ifstream(binaryAlnFile))
      	{
      	  std::cout << "Error: tried to create intermediate file "<< binaryAlnFile << ", but did not succeed!"  << endl; 
      	  exitFunction(-1, false); 
      	}
    }
  else 
    {
      binaryAlnFile = _cl.getAlnFileName();   
    }
  
  return binaryAlnFile; 
}


void SampleMaster::catchRunErrors() const 
{
  if( int(_runParams.getNumRunConv()) <_cl.getNumRunParallel() )
    {
      tout << "\n\tERROR: you want to run  "<< _cl.getNumRunParallel() << " independent runs in parallel, while there are only " << _runParams.getNumRunConv() << " to be run in total.\n\n" << endl; 
      exitFunction(-1, true); 
    }

  if ( _runParams.getNumCoupledChains() < _cl.getNumChainsParallel() )
    {
      tout << "\n\tERROR: you want to run " << _cl.getNumChainsParallel() << " coupled chains in parallel, while there are only "<< _runParams.getNumCoupledChains() << " to be run in total.\n\n" << endl; 
      exitFunction(-1,true); 
    }

  if( (  _runParams.getNumRunConv() % _cl.getNumRunParallel() )  != 0 )
    {
      tout << "\n\tERROR: you specified to run " << _runParams.getNumRunConv() << " independent runs and " << _cl.getNumRunParallel() << " of them in parallel. Currently, the total number of runs must be a multiple of the number of parallel runs." << endl; 
      exitFunction(-1,true); 
    }

  if( (  _runParams.getNumCoupledChains() % _cl.getNumChainsParallel() ) != 0  )
    {
      tout << "\n\tERROR: you specified to run " << _runParams.getNumCoupledChains() << " coupled chains for each independent run and "<< _cl.getNumChainsParallel() << " of them in parallel. Currently, the total number of coupled chains must be a multiple of the number of parallel coupled chains." << endl; 
      exitFunction(-1, true); 
    }
}


void SampleMaster::initializeRuns(Randomness rand)
{  
  auto startingTrees = getStartingTreeStrings(); 

  if(_cl.getCheckpointId().compare(_cl.getRunid()) == 0)
    {
      std::cerr << "You specified >" << _cl.getRunid() << "< as runid and intended\n"
		<< "to restart from a previous run with id >" << _cl.getCheckpointId() << "<."
		<< "Please specify a new runid for the restart. " << endl; 
      exitFunction(-1, true); 
    }

  // correctly initialize the first tree / alignment
  auto binaryAlnFile = getOrCreateBinaryFile(); 

  auto &&bFile = ByteFile(binaryAlnFile, _cl.isSaveMemorySEV()); 

  bFile.parse(*_plPtr); 
  auto partitions = bFile.getPartitions();

  auto taxa = bFile.getTaxa(); 
  auto numPart = partitions.size(); 

  // tout << "\nNUM" << PLL_NUM_BRANCHES << endl; 
  PLL_NUM_BRANCHES = int(numPart); 

  auto&& initTree = TreeAln(taxa.size(), _cl.isSaveMemorySEV() );
  initTree.setPartitions(partitions, true); 
  initTree.setTaxa(taxa);

  auto bls = BranchLengthResource{}; 
  bls.initialize(taxa.size(), numPart);
  initTree.setBranchLengthResource(bls); 

  auto evalUptr = createEvaluatorPrototype(initTree,  _cl.isSaveMemorySEV()); 
  
  auto aRes = processConfigFile(_cl.getConfigFileName(), initTree);

  auto&& params = get<0>(aRes); 
  auto&& proposals = get<1>(aRes);
  auto&& proposalSets = get<2>(aRes); 
  
  catchRunErrors();

  auto runSeeds = vector<randCtr_t>{};
  auto treeSeeds = vector<randCtr_t>{}; 
  for(nat i = 0; i <  _runParams.getNumRunConv();++i)
    {
      runSeeds.push_back(rand.generateSeed()); 
      treeSeeds.push_back(rand.generateSeed()); 
    }

  // determine if topology is fixed 
  auto topoIsFixed = false; 
  for(auto &v :params)
    {
      if( dynamic_cast<TopologyParameter*>(v) != nullptr && not v->getPrior()->needsIntegration() )
	topoIsFixed = true; 
    }

  // gather branch length parameters
  auto blParams = vector<AbstractParameter*>{} ;
  for(auto &v : params)
    {
      if(v->getCategory() == Category::BRANCH_LENGTHS)
	blParams.push_back(v);
    }

  if(startingTrees.size() > 1 )
    {
      tout << "You provided " << startingTrees.size() << " starting trees" << endl;       

      if(topoIsFixed)
	tout << "Since the topology is fixed, only the first one will be used." << endl; 
    }

  auto hadBl = false; 
  if(topoIsFixed)
    {
      auto treeRandomness = Randomness(treeSeeds[0]); 
      hadBl = initializeTree(initTree, 
				  startingTrees.size() > 0  ? startingTrees.at(0): std::string(""), // meh 
				  treeRandomness, blParams); 
    }

  for(nat i = 0; i < _runParams.getNumRunConv() ; ++i)
    {    
      auto trees = vector<TreeAln>{}; 
      for(nat j = 0; j < _runParams.getNumCoupledChains() ; ++j ) 
	trees.emplace_back(initTree);

      auto hadBls = vector<bool>(trees.size() , topoIsFixed  && hadBl ? true : false);
      if(not topoIsFixed)
	hadBls = initTrees(trees, treeSeeds[i],  startingTrees, blParams); 

      // TODO method could be moved to treealn 
      for(nat j = 0; j < trees.size() ;++j)
	initializeWithParamInitValues(trees[j], params, hadBls[j] );
      
      auto runRand =  Randomness(runSeeds[i]); 
      
      auto chains = vector<Chain>{};       
      for(nat j = 0; j < _runParams.getNumCoupledChains(); ++j)
	{
	  auto &t = trees.at(j);
	  chains.emplace_back( runRand.generateSeed(), t, params, proposals, proposalSets, evalUptr, _cl.isDryRun() ); 
	  auto &chain = chains[j]; 		
	  chain.setRunId(i); 
	  chain.setTuneFreuqency(_runParams.getTuneFreq()); 
	  chain.setHeatIncrement(j); 
	  chain.setDeltaT(_runParams.getHeatFactor()); 
	}

      _runs.emplace_back(runRand, i, _cl.getWorkdir(), _cl.getRunid(), _runParams.getNumCoupledChains(), chains); 
      auto &run = _runs.back(); 
      run.setTemperature(_runParams.getHeatFactor());
      run.setSamplingFreq(_runParams.getSamplingFreq()); 
      run.setNumSwapsPerGen(_runParams.getNumSwapsPerGen());

      if(_plPtr->isRunLeader() && _plPtr->isMyRun(run.getRunid()))
	run.initializeOutputFiles(_cl.isDryRun());
    }

  // important: initialize asynchronous tag based system 
  _plPtr->getChainComm().initWithMaxChains( _runParams.getNumCoupledChains() , _plPtr->getChainComm().size()); 

  if(_cl.getCheckpointId().compare("") != 0)
    initializeFromCheckpoint(); 
  
  if(_plPtr->isGlobalMaster() && not _diagFile.isInitialized() && not _cl.isDryRun() )
    _diagFile.initialize(_cl.getWorkdir(), _cl.getRunid(), _runs);
  
  // post-pone all printing to the end  
  if(not _cl.isQuiet())
    {
      printAlignmentInfo(initTree);   
      printParameters(initTree, params);
      printProposals(proposals, proposalSets, params ); 
      informPrint();
      teeOut->flush();
      printInitializedFiles(); 
    }

  printInitialState(); 
  if(not _cl.isQuiet() && _plPtr->getGlobalComm().size() > 1)
    {
      auto res = _plPtr->printLoadBalance(initTree, _runParams.getNumRunConv(), _runParams.getNumCoupledChains());
      tout << res; 
    }

  // BEGIN DEBUG 
  auto printTheAlignment = false;  
  if(printTheAlignment)
    {
      auto &p = initTree.getPartition(0); 
      auto &&ss = std::stringstream{}; 
      p.printAlignment(ss); 
      std::cout << ss.str() << endl; 
    }
  // END 

}


void SampleMaster::printInitializedFiles() const 
{
  auto &&stream = tout;

  bool isRestart = _cl.getCheckpointId().compare("") != 0;
  auto initString = isRestart ? "regenerated" : "initialized" ; 

  stream << initString  << " diagnostics file " << _diagFile.getFileName()  << endl; 

  for(auto &run : _runs)
    {
      for(auto &elem : run.getAllFileNames())
	{
	  stream << initString << " file " << elem << endl; 
	}
    }
} 


void SampleMaster::printInitialState()  
{    
  auto& res = _runs.at(0).getChains().at(0).getEvaluator().getArrayReservoir(); 

  // compute the initial state 
  for(auto &run: _runs)
    {    
      for(nat i = 0; i < run.getChains().size(); ++i)	
	{
	  auto &chain = run.getChains()[i]; 
	  auto &eval = chain.getEvaluator(); 
	  auto &traln  = chain.getTralnHandle();
	  chain.setLikelihood(traln.getLikelihood());
	  eval.evaluate(traln, traln.getAnyBranch(), true);

	  // NOTICE was not here before 
	  chain.setLikelihood(traln.getLikelihood()) ; 
	  
	  eval.freeMemory(); 
	  traln.clearMemory(res); 

	  if(_plPtr->isMyChain(run.getRunid(), i))
	    {
	      chain.resume(); 
	      chain.suspend();
	    }
	}
    }

  // if we are parallel, we must inform the master about what we
  // computed
  _plPtr->synchronizeChainsAtMaster(_runs, CommFlag::PRINT_STAT);

  tout << endl << "initial state: " << endl; 
  tout << "================================================================" << endl; 
  for(auto &run: _runs)
    {
      for(auto &chain: run.getChains())
	{
	  tout << chain ; 
	  tout << "\tRNG(" << chain.getChainRand() << ")" << endl; 
	}
      tout << "================================================================" << endl; 
    }
  tout << endl; 
}  


std::pair<double,double> SampleMaster::convergenceDiagnostic(nat &start, nat &end)
{
  if(_runParams.getNumRunConv() == 1)
    return std::pair<double,double>(std::numeric_limits<double>::min(),std::numeric_limits<double>::min());

  auto fns = vector<string>{}; 
  for(nat i = 0; i < _runParams.getNumRunConv(); ++i)
    {
      auto &&ss = stringstream{};

      ss << OutputFile::getFileBaseName(_cl.getWorkdir())  << "_topologies.run-" << i  << "." << _cl.getRunid();

      // if we do not find the file, let's assume we have multiple branch lengths 
      if (not std::ifstream(ss.str()) ) 
        ss << ".tree.0";

      fns.push_back(ss.str());
    }

  auto&& asdsf = SplitFreqAssessor(fns, false);
  end = asdsf.getMinNumTrees() ; 


  // :HACK: possibly the number of trees is parsed incorrectly. This
  // is not problematic, but could lead to a hang, if we have too few
  // trees. 
  if(end < 4  )
    return std::make_pair(nan(""), nan(""));   

  if( _runParams.getBurninGen() > 0 )
    {
      int treesToDiscard =  _runParams.getBurninGen() / _runParams.getSamplingFreq(); 

      if(int(end) < treesToDiscard + 2 )
	return make_pair(nan(""), nan("")); 
      else 
	start = treesToDiscard; 
    }
  else 
    {
      assert(_runParams.getBurninGen() == 0); 
      start = (int)((double)end * _runParams.getBurninProportion()  ); 
    } 

  // dont forget that we also sampled gen0, therefore: 
  ++end; 
  
  asdsf.extractBips(std::vector<nat>( _runParams.getNumRunConv(), start), std::vector<nat>(_runParams.getNumRunConv(), end) );
  auto asdsfVals = asdsf.computeAsdsfNew(_runParams.getAsdsfIgnoreFreq());

  return asdsfVals;
}



std::tuple<ParameterList , std::vector<std::unique_ptr<AbstractProposal> > , std::vector<ProposalSet> >   
SampleMaster::processConfigFile(string configFileName, const TreeAln &traln )  
{
  auto reader = ConfigReader{}; 
  auto && fh = ifstream(configFileName); 
  auto token = NxsToken(fh); 
  auto&& paramBlock = BlockParams{}; 
  paramBlock.setTree(&traln); 
  auto&& priorBlock = BlockPrior(traln.getNumberOfPartitions());
  auto proposalConfig = BlockProposalConfig{}; 
  
  reader.Add(&paramBlock); 
  reader.Add(&priorBlock);
  reader.Add(&_runParams);
  reader.Add(&proposalConfig);   
  reader.Execute(token);
  
  _runParams.verify();
  proposalConfig.verify(); 

  proposalConfig.verify(); 

  auto r = RunFactory{}; 
  auto paramResult = paramBlock.getParameters();
  r.addStandardParameters(paramResult, traln);
  auto paramList = ParameterList(move(paramResult)); 

  auto proposalSetResult = vector<ProposalSet>{};
  auto&& proposalResult = vector<unique_ptr<AbstractProposal>>{}; 
  std::tie(proposalResult, proposalSetResult) = r.produceProposals(proposalConfig, priorBlock , paramList, traln, _runParams.isComponentWiseMH() && traln.getNumberOfPartitions() > 1, *_plPtr);

  // sanity check 
  for(auto &p : paramList)
    p->checkSanityPartitionsAndPrior(traln);

  // now enumerate the proposals 
  nat ctr = 0; 
  for(auto &p : proposalResult)
    {
      p->setId(ctr); 
      ++ctr; 
    }
  for(auto &p : proposalSetResult)
    ctr = p.numerateProposals(ctr);

  return std::make_tuple(paramList, move(proposalResult), proposalSetResult);
}


void SampleMaster::printDuringRun(uint64_t gen)   
{
  _plPtr->synchronizeChainsAtMaster(_runs, CommFlag::PRINT_STAT); 

  auto time = _printTime.getRecentElapsed(); 
  _printTime.updateTime();
  
  auto && ss = std::stringstream{} ; 
  ss << SOME_FIXED_PRECISION; 
  ss << "[" << gen << "," <<  time << "s]\t"; 

  bool isFirst = true; 
  for(auto &run : _runs)
    {
      if(isFirst)
	isFirst = false; 
      else 
	ss << " ==="; 
      
      auto sortedLnls = vector<std::pair<nat,log_double>>{}; 
      for(auto &c : run.getChains() ) 
	sortedLnls.emplace_back(c.getCouplingId(), c.getLikelihood()); 
      std::sort(begin(sortedLnls), end(sortedLnls), [] (const std::pair<nat,log_double> &elem1, const std::pair<nat,log_double> &elem2 ) { return elem1.first < elem2.first;  }); 

      for(auto elem : sortedLnls)
	{
	  ss.imbue(locale(ss.getloc(), new ThousandsSeparator<char>(',') )); 
	  
	  ss << " " << elem.second; 
	}
    }

  // print it 
  tout << ss.str() << endl; 
}


void SampleMaster::simulate()
{
  if(not _cl.isQuiet())
    {
      tout << "Starting MCMC sampling " ; 

#if USE_AVX
      auto impl=std::string{"AVX"};
#elif USE_SSE
      auto impl=std::string{"SSE"};
#else 
      auto impl=std::string{"standard"};
#endif 

      tout << "using the " << impl << " implementation" <<    (  _cl.isSaveMemorySEV() ?  " with SEVs" : "" ) << " for likelihood computations." << endl; 
      
      if(_runParams.getNumRunConv() > 1 &&  _runParams.isUseStopCriterion() )
	tout << PROGRAM_NAME << " will run until topological convergence is achieved\n"
	     << "(" << ( _runParams.isUseAsdsfMax() ? "MSDSF < " : "ASDSF < " ) 
	     << _runParams.getAsdsfConvergence() * 100 <<  "%, at least " 
	     << _runParams.getNumGen() << " generations).\n" ; 
      else 
	tout  << PROGRAM_NAME << " will run for "<< _runParams.getNumGen() << " generations.\n" ;
      tout << PROGRAM_NAME << " will print log-likelihoods of all chains, grouped by\n"
	   << "run id (separated by '=') and sorted by heat (starting with the\n"
	   << "cold chain). First column indicates generation number (completed\n"
	   << "by all chains) and the time elapsed for this increment.\n\n"; 
    } 

  auto hasConverged = false;   
  auto curGen = _runs[0].getChains()[0].getGeneration(); // dangerous 
  for(auto & run: _runs)
    for(auto &c : run.getChains())
      assert( nat(c.getGeneration()) == curGen );       

  auto lastPrint = (curGen / _runParams.getPrintFreq() )  * _runParams.getPrintFreq() ; 
  auto lastDiag =  (curGen / _runParams.getDiagFreq() ) * _runParams.getDiagFreq(); 
  auto lastChkpnt = ( curGen / _runParams.getChkpntFreq() )* _runParams.getChkpntFreq() ; 
  
  printDuringRun(curGen); 

  while(curGen < nat(_runParams.getNumGen()) || not hasConverged)   
    { 
      auto nextPrint =  uint64_t(lastPrint + _runParams.getPrintFreq()); 
      auto nextDiag = uint64_t(lastDiag + _runParams.getDiagFreq()); 
      auto nextChkpnt = uint64_t(lastChkpnt + _runParams.getChkpntFreq()); 

      auto stopPoints =std::vector<uint64_t> { nextChkpnt , nextPrint, nextDiag } ; 
      
      if(not _runParams.isUseStopCriterion())
	stopPoints.push_back(uint64_t(_runParams.getNumGen()));

      if(curGen < _runParams.getNumGen())
	stopPoints.push_back(uint64_t(_runParams.getNumGen()));

      auto  nextStop = *(std::min_element(stopPoints.begin(), stopPoints.end())); 
      auto toExecute = nextStop - curGen; 

      for(auto &run : _runs)
	{
	  // std::cout <<   "running a part" << endl; 
	  // tout <<   "running a part" << endl; 
	  if(_plPtr->isMyRun(run.getRunid()))
	    run.executePart(curGen, toExecute, *_plPtr );
	}

      curGen += toExecute; 

      hasConverged = not _runParams.isUseStopCriterion()  || (( _runs.size() == 1) && (curGen >= _runParams.getNumGen())); 

      if( (curGen % _runParams.getDiagFreq()) == 0 )
	{
	  auto asdsf = std::make_pair(nan(""), nan("")); 

	  if(_runs.size() > 1 && _runParams.isUseStopCriterion() )
	    // if(not hasConverged)
	    {
	      if( _plPtr->isGlobalMaster())
		{
		  nat start = 0, 
		    end = 0; 
		  asdsf = convergenceDiagnostic(start, end); 

		  double convCrit = _runParams.getAsdsfConvergence();  
		  if(_runParams.isUseAsdsfMax())
		    hasConverged = asdsf.second < convCrit;  
		  else 
		    hasConverged = asdsf.first < convCrit;  
	      
		  tout << endl  << "standard deviation of split frequencies for trees " << start << "-" << end  << " (avg/max):\t"
		       << PERC_PRECISION << asdsf.first * 100 << "%\t" << asdsf.second * 100 << "%"   << endl << endl; 
		}
	    }
	  else 
	    hasConverged = true ; 

	  auto swapBackup = std::vector<SwapMatrix> {};
	  if(_plPtr->isGlobalMaster())
	    {
	      for(auto &run : _runs)
		{
		  if(_plPtr->isMyRun(run.getRunid()))
		    swapBackup.push_back(run.getSwapInfo());
		}
	    } 

	  _plPtr->synchronizeChainsAtMaster(_runs, CommFlag::PRINT_STAT | CommFlag::SWAP | CommFlag::PROPOSALS);

	  // hack 
	  int bla = hasConverged ? 1 : 0 ; 
	  bla = _plPtr->getGlobalComm().broadcastVar(bla); 
	  hasConverged = bla == 1 ? true : false; 
	  // end

	  if(_plPtr->isGlobalMaster()) 
	    _diagFile.printDiagnostics(curGen, asdsf.first, _runs);

	  if(_plPtr->isGlobalMaster())
	    {
	      auto iter = begin(swapBackup);
	      for(auto &run : _runs)
		{
		  if(_plPtr->isMyRun(run.getRunid()))
		    {
		      run.setSwapInfo(*iter);	
		      ++iter;
		    }
		}
	    }
	  
	  lastDiag = curGen; 
	}
      
      if(curGen % _runParams.getPrintFreq() == 0 )
	{
	  printDuringRun(curGen);
	  lastPrint = curGen; 
	}

      if(curGen % _runParams.getChkpntFreq() == 0)
	{
	  writeCheckpointMaster();
	  lastChkpnt = curGen; 
	} 
    }


}

 
void SampleMaster::finalizeRuns()
{
  for(auto &run : _runs)
    {
      for(auto &chain : run.getChains())
	{
	  if(  std::fabs(chain.getChainHeat() - 1. ) < std::numeric_limits<double>::epsilon())
	    {
	      tout << SOME_FIXED_PRECISION << "best state for run " << run.getRunid() << " was: "  << chain.getBestState( )<< endl;       
	    }
	}
    }

  _timeTracker.updateTime();

  auto wallTimeElapsed = _timeTracker.getAccWallTime();
  auto altFormat = TimeTracker::formatForWatch(wallTimeElapsed); 
  tout << endl << "Converged/stopped after " <<  _runs[0].getChains()[0].getGeneration() << " generations" << endl;   
  tout << endl << "Total walltime elapsed:\t" 
       <<  SOME_FIXED_PRECISION << wallTimeElapsed << " seconds "
       << "\tor " <<  std::setfill('0') << std::setw(2) <<  int(altFormat[0]) << ":" << std::setfill('0') << std::setw(2) << int(altFormat[1]) << ":" << altFormat[2] << " (hh:mm:ss)." << endl; 

  auto cpuTime =  _timeTracker.getAccCpuTime();
  altFormat = TimeTracker::formatForWatch(cpuTime) ; 
  
  tout << "Total CPU time elapsed:\t"  ; 
  tout <<  SOME_FIXED_PRECISION << cpuTime << " seconds "
       << "\tor " <<  std::setfill('0') << std::setw(2) <<  int(altFormat[0]) << ":" << std::setfill('0') << std::setw(2) << int(altFormat[1]) << ":" << altFormat[2] << " (hh:mm:ss)." << endl; 
}


void SampleMaster::deserialize( std::istream &in ) 
{
  for(auto &run : _runs)
    run.deserialize(in); 

  _timeTracker.deserialize(in);
  _timeTracker.updateTime();
}

 
void SampleMaster::serialize( std::ostream &out) const
{  
  for(auto & run : _runs)    
    run.serialize(out);

  _timeTracker.serialize(out);
}


void SampleMaster::writeCheckpointMaster() 
{
  // whenever we synchronize swap, we have to backup our own swap
  // matrices
  auto swb=  std::vector<SwapMatrix> {}; 
  if( _plPtr->isGlobalMaster())
    {
      for(auto &run : _runs)
	swb.push_back(run.getSwapInfo());
    }

  _plPtr->synchronizeChainsAtMaster(_runs, CommFlag::PRINT_STAT | CommFlag::PROPOSALS | CommFlag::TREE | CommFlag::SWAP ); 

  if( _plPtr->isGlobalMaster())
    {
      auto iter = begin(swb); 
      for(auto &run : _runs)
	{
	  run.setSwapInfo(*iter); 
	  ++iter; 
	}
    }

  if( _plPtr->isGlobalMaster() )
    {
      auto &&ss = std::stringstream{}; 
      ss <<  OutputFile::getFileBaseName(_cl.getWorkdir()) << "_newCheckpoint." << _cl.getRunid()  ; 
      auto newName = ss.str();
      auto&& chkpnt = std::ofstream{}; 
      Serializable::getOfstream(newName, chkpnt); 
      _timeTracker.updateTime();
      serialize(chkpnt);

      ss.str("");
      ss << OutputFile::getFileBaseName(_cl.getWorkdir()) << "_checkpoint." << _cl.getRunid(); 
      auto curName = ss.str();
      if( std::ifstream(curName) )
	{
	  ss.str("");
	  ss << OutputFile::getFileBaseName(_cl.getWorkdir())  << "_prevCheckpointBackup." << _cl.getRunid(); 
	  auto prevName =  ss.str();
	  if( std::ifstream(prevName) )
	    {
	      int ret = remove(prevName.c_str()); 
	      assert(ret == 0); 
	    }
	      
	  int ret = rename(curName.c_str(), prevName.c_str()); 
	  assert(ret == 0); 
	}
	  
      int ret = rename(newName.c_str(), curName.c_str()); 
      assert(ret == 0); 
    }
  else 
    {
      auto &&nullstream = std::ofstream("/dev/null"); 
      serialize(nullstream); 
    }
} 
