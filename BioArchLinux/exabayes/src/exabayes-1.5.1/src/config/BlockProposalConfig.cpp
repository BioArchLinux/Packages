#include <cassert>

#include "BlockProposalConfig.hpp"

extern void genericExit(int code); 




BlockProposalConfig::BlockProposalConfig()
  : userValue{}
  , etbrStopProb(0.5)
  , esprStopProp(0.5)    
  , parsimonyWarp(0.1)
  , _likeSprMaxRadius{-1}
  , parsSPRRadius(-1)
  , _likeSprWarp(1.) 
  , _moveOptMode{MoveOptMode::NONE}
  , _useMultiplier{false}
{
  NCL_BLOCKTYPE_ATTR_NAME = "PROPOSALS"; 
}


void BlockProposalConfig::Read(NxsToken &token)
{   
  DemandEndSemicolon(token, "PROPOSALS");
  
  auto ps = ProposalTypeFunc::getAllProposals(); 

  while(true)
    {
      token.GetNextToken();
      NxsBlock::NxsCommandResult res = HandleBasicBlockCommands(token); 
   
      if (res == NxsBlock::NxsCommandResult(STOP_PARSING_BLOCK))
	return;
      if (res != NxsBlock::NxsCommandResult(HANDLED_COMMAND))
	{
	  NxsString key = token.GetToken(false);
	  token.GetNextToken(); 
	  NxsString value = token.GetToken(false); 	    

	  if(ProposalTypeFunc::isValidName(key))
	    {
	      double val = value.ConvertToDouble(); 
	      auto t = ProposalTypeFunc::getTypeFromConfigString(key); 
	      if(userValue.find(t) != userValue.end())
		{
		  std::cerr << "encountered the value " << key << "twice in the config file" << std::endl; 
		  exitFunction(-1, true); 
		}
	      else 
		userValue[t] = val; 
	    }
	  else if(key.EqualsCaseInsensitive("esprstopprob"))	    
	    esprStopProp = value.ConvertToDouble();	  
	  else if(key.EqualsCaseInsensitive("moveoptmode"))
	    {
	      auto val =  value.ConvertToInt(); 
	      assert(0 <= val && val < 5 ); // no better way to check =/ 
	      _moveOptMode = MoveOptMode(val); 
	    }
	  else if(key.EqualsCaseInsensitive("etbrstopprob"))
	    etbrStopProb = value.ConvertToDouble(); 	  
	  else if(key.EqualsCaseInsensitive("likesprmaxradius"))
	    _likeSprMaxRadius  = value.ConvertToInt();
	  else if(key.EqualsCaseInsensitive("likesprwarp"))
	    _likeSprWarp = value.ConvertToDouble();
	  else if(key.EqualsCaseInsensitive("parsimonyWarp"))	    
	    parsimonyWarp = value.ConvertToDouble();
	  else if(key.EqualsCaseInsensitive("usemultiplier"))
	    _useMultiplier = convertToBool(value); 
	  else if(key.EqualsCaseInsensitive("parssprradius"))
	    {
	      parsSPRRadius = value.ConvertToInt();
	      // tout << "\n\nfound spr radius " << parsSPRRadius << "\n\n" << std::endl ;
	    }
	  else 	      
	    {
	      cerr << "WARNING: ignoring unknown value >"  << key << "< and >" << value <<  "<" << endl; 
	      assert(0);
	    }
	}
    }
}


void BlockProposalConfig::verify()
{
  if(not (0.01 < esprStopProp && esprStopProp <= 1 ))
    {
      tout << "Error: >esprStopProp< must be in the range (0.01,1]" << std::endl; 
      exitFunction(-1, true); 
    }

  if(not (0.01 < esprStopProp && esprStopProp <= 1 ))
    {
      tout << "Error: >etbrStopProb< must be in the range (0.01,1]" << std::endl; 
      exitFunction(-1,true); 
    }
  
  if(not (0.001 < parsimonyWarp && parsimonyWarp < 10))
    {
      tout << "Error: >parsimonyWarp< must be in the range (0.001,10)" << std::endl; 
      exitFunction(-1, true); 
    }
  
  if(parsSPRRadius != -1 &&  (parsSPRRadius <= 1 ))
    {
      tout << "Error: >parsSPRRadius< must be in the range (1,inf)" << std::endl; 
      exitFunction(-1, true); 
    }
}

// NOTICE 

