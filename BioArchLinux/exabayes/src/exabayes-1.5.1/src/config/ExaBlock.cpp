#include "ExaBlock.hpp"

#include <cassert>


bool ExaBlock::convertToBool(NxsString &string) const 
{
  if(string.EqualsCaseInsensitive("true"))
    return true ; 
  else if (string.EqualsCaseInsensitive("false"))
    return false; 
  else 
    {
      cerr << "ERROR while parsing boolean value: expected either \"true\" or \"false\"" << endl; 
      assert(0); 
      return false; 
    }  
}



double ExaBlock::parseScientificDouble(NxsToken& token)  const 
{
  double result =  0.0 ;
  auto str = token.GetToken(); 
  token.GetNextToken();
  
  bool sawE =  ( str.back()  == 'e' || str.back() == 'E') ; 
  
  if(token.GetToken().EqualsCaseInsensitive("e"))
    {
      sawE = true; 
      str += token.GetToken(); 
      token.GetNextToken();
    }
  
  if(sawE &&  ( token.GetToken().EqualsCaseInsensitive("+") || token.GetToken().EqualsCaseInsensitive("-") ))
    {
      str += token.GetToken(); 
      token.GetNextToken();
    }
  
  if(sawE)
    {
      str += token.GetToken(); 
      token.GetNextToken(); 
    }

  auto &&iss = std::istringstream{str}; 
  iss >> result ; 
  return result; 
} 







