#include "BlockRunParameters.hpp" 

#include "GlobalVariables.hpp"


BlockRunParameters::BlockRunParameters()  
  : diagFreq(5000) 
  , asdsfIgnoreFreq(0.1)
  , asdsfConvergence(0.05)
  , useStopCriterion{true}
  , burninGen(0)
  , burninProportion(0.25)
  , samplingFreq (500)
  , numRunConv(1)
  , numGen(1e6)
  , numCoupledChains(1)
  , printFreq(500)
  , heatFactor(0.1)
  , tuneHeat(false)
  , tuneFreq(100)
  , useParsimonyStarting(true)
  , heatedChainsUseSame(false)
  , chkpntFreq(1000)
  , componentWiseMH(true)
  , useAsdsfMax(false)
  , numSwapsPerGen(1.)
{
  NCL_BLOCKTYPE_ATTR_NAME = "run"; 
}


static int myConvertToInt(NxsString &elem)
{
  return int(elem.ConvertToDouble());
}

static uint64_t myConvertToLongInt(NxsString& elem)
{
  return uint64_t(elem.ConvertToDouble());
}


void BlockRunParameters::Read(NxsToken &token)
{ 
  DemandEndSemicolon(token, "runconfig");

  while(true)
    {
      token.GetNextToken();
      NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token); 

      if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
	return;
      if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
	{
	  auto key = token.GetToken(false);
	  token.GetNextToken(); 
	  auto value = token.GetToken(false); 	    

	  if(key.EqualsCaseInsensitive("numGen"))
	    numGen = myConvertToLongInt(value);
	  else if (key.EqualsCaseInsensitive("parsimonystart"))
	    useParsimonyStarting = convertToBool(value); 
	  else if (key.EqualsCaseInsensitive("checkpointinterval"))
	    chkpntFreq = myConvertToInt(value); 
	  else if(key.EqualsCaseInsensitive("samplingfreq"))
	    samplingFreq = myConvertToInt(value); 
	  else if(key.EqualsCaseInsensitive("proposalsets"))
	    componentWiseMH = convertToBool(value); 
	  else if(key.EqualsCaseInsensitive("numRuns"))	    
	    numRunConv = myConvertToInt(value); 
	  else if(key.EqualsCaseInsensitive("diagFreq"))
	    diagFreq = myConvertToInt(value); 
	  else if(key.EqualsCaseInsensitive("heatedChainsUseSame"))
	    heatedChainsUseSame = convertToBool(value); 
	  else if(key.EqualsCaseInsensitive("numcoupledChains"))
	    numCoupledChains = myConvertToInt(value); 
	  else if(key.EqualsCaseInsensitive("printFreq") )	   
	    printFreq = myConvertToInt(value);
	  else if (key.EqualsCaseInsensitive("sdsfIgnoreFreq"))
	    asdsfIgnoreFreq = value.ConvertToDouble(); 
	  else if (key.EqualsCaseInsensitive("sdsfConvergence"))
	    asdsfConvergence = value.ConvertToDouble();
	  else if (key.EqualsCaseInsensitive("heatFactor"))
	    heatFactor = value.ConvertToDouble();
	  else if(key.EqualsCaseInsensitive("numSwapsPerGen"))
	    numSwapsPerGen = value.ConvertToDouble();
	  else if(key.EqualsCaseInsensitive("tuneHeat"))
	    tuneHeat = convertToBool(value);
	  else if(key.EqualsCaseInsensitive("tuneFreq"))
	    tuneFreq = myConvertToInt(value);
	  else if(key.EqualsCaseInsensitive("burninGen"))
	    burninGen = myConvertToInt(value);
	  else if(key.EqualsCaseInsensitive("burninProportion"))
	    burninProportion = value.ConvertToDouble();
	  else if(key.EqualsCaseInsensitive("convergencecriterion"))
	    {
	      if(value.EqualsCaseInsensitive("mean"))
		{
		  useStopCriterion = true; 
		  useAsdsfMax = false; 
		}
	      else if(value.EqualsCaseInsensitive("max"))
		{
		  useStopCriterion = true; 
		  useAsdsfMax = true ; 
		}
	      else if( value.EqualsCaseInsensitive("none"))
		{
		  useStopCriterion = false; 
		}
	      else 
		{
		  std::cerr << "Error: valid values for config entry >convergenceCriterion< are 'mean', 'max' or 'none'."  << std::endl; 
		  exitFunction(-1, true);
		}
	    }
	  else 	      
	    cerr << "WARNING: ignoring unknown value >"  << key << "< and >" << value <<  "<" << endl; 
	}
    }
}


#pragma GCC diagnostic ignored "-Wfloat-equal"

static void verifyProbability(double value, bool lowerIncluded, bool upperIncluded, std::string name)
{
  auto lowOkay = 0 < value || ( lowerIncluded &&  0 ==  value) ; 
  auto upperOkay = value < 1. || (upperIncluded && value == 1); 

  char lowBracket = lowerIncluded ? '[': '('; 
  char upperBracket = upperIncluded ? ']' : ')' ; 

  if( not ( lowOkay && upperOkay ) )
    {
      std::cerr << "Error: >" << name << "< must be in the interval " << lowBracket  << "0,1" << upperBracket << std::endl; 
      exitFunction(-1, true); 
    }

}


template<typename T>
static void verifyGreaterZero(T value, std::string name )
{
  if( not ( value > 0 )   )
    {
      std::cout << "Error: >name< must be > 0 "  << std::endl; 
      exitFunction(-1, true); 
    }  
}




void BlockRunParameters::verify()   const 
{
  verifyGreaterZero(diagFreq, "diagFreq"); 
  
  verifyProbability(asdsfIgnoreFreq, true, true , "asdsfIgnoreFreq" ); 
  verifyProbability(asdsfConvergence, false, false, "asdsfConvergence"); 
  verifyProbability(burninProportion, false, false, "burninProportion"); 
  verifyProbability(heatFactor, false, false, "heatFactor"); 
  
  if(numSwapsPerGen < 0.)
    {
      std::cerr << "Error: >numSwapsPerGen< must be in > 0."  << std::endl; 
      exitFunction(-1, true); 
    }

  // verifyGreaterZero(numSwaps, "numSwaps"); 
  verifyGreaterZero(samplingFreq,"samplingFreq" ); 
  verifyGreaterZero(numRunConv, "numRunConv"); 
  verifyGreaterZero(numGen, "numGen"); 
  verifyGreaterZero(numCoupledChains, "numCoupledChains"); 
  verifyGreaterZero(printFreq, "printFreq"); 
  // verifyGreaterZero(swapInterval, "swapInterval"); 
  // verifyGreaterZero(tuneFreq, "tuneFreq");  

  if( diagFreq <= nat(samplingFreq)  ) 
    {
      std::cerr << "diagFreq < samplingFreq. Please choose the sampling frequency smaller than the diagnosis frequency.  " << std::endl; 
      exitFunction(-1, true); 
    }
}
