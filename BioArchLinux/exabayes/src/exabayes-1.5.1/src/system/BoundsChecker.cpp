#include <cmath>
#include <cassert>

// #include "Branch.hpp"

#include "BoundsChecker.hpp"
#include "GlobalVariables.hpp"

// here we initialize the max/min values for our various
// parameters. Static const is essentially like a global variable but
// saver. You access it with e.g., BoundsChecker::zmin, since the variable
// does not belong to an instance of the class, but the class in general. 

const double BoundsChecker::zMin = 1.0e-15 ; // 1e-16
const double BoundsChecker::zMax = (1.0 - 1.0e-6) ; // 1-1e-6

//  these values would be okay according to raxml, but it takes things too far ... 
const double BoundsChecker::rateMin = 1e-7; 
const double BoundsChecker::rateMax = 1e6; 

const double BoundsChecker::alphaMin = 2e-2; 
const double BoundsChecker::alphaMax = 1e3; 

const double BoundsChecker::freqMin = 1e-3;


bool BoundsChecker::checkFrequencies( const std::vector<double> &freqs )  
{
  static double dirtyThresh = 1e-6; 

  bool result = true; 
  double sum = 0; 
  for(auto &f : freqs)    
    {
      result &= ( freqMin <= f || fabs(freqMin - f)  <   dirtyThresh) ; 
      sum += f; 
    }
  assert( fabs(sum  - 1.0 ) < 1e-6 ); 
  return result; 
}

bool BoundsChecker::checkBranch( const BranchLength &branch) 
{
  auto v = branch.getLength().getValue() ; 
  return BoundsChecker::zMin <= v && v <= BoundsChecker::zMax ; 
}

bool BoundsChecker::checkBranch( const BranchLengths &branch) 
{
  bool okay = true; 
  for(auto &v : branch.getLengths())
    {
      auto val = v.getValue();
      okay &= BoundsChecker::zMin <= val && val <= BoundsChecker::zMax; 
    }
  return okay; 
}

bool BoundsChecker::checkRevmat( const std::vector<double> &rates) 
{
  static double dirtyThresh = 1e-6;

  bool result = true; 
  for(auto &r : rates)
    {
      auto prob =  ( r <= BoundsChecker::rateMax || fabs(BoundsChecker::rateMax - r ) < dirtyThresh ) 
	&&  ( BoundsChecker::rateMin <= r  || fabs(BoundsChecker::rateMin - r ) < dirtyThresh ) ; 
      
      if(not prob)
      	tout << MAX_SCI_PRECISION << "attention: problem  with " << r << std::endl; 
      result &=  prob ; 
    }

  if( std::fabs(rates.back() - 1. )  > std::numeric_limits<double>::epsilon() )
    {
      tout << "error: last state was "<< MAX_SCI_PRECISION << rates.back() << std::endl; 
      assert(0) ;
    }

  return result; 
}
 
bool BoundsChecker::checkAlpha( double alpha) 
{
  return BoundsChecker::alphaMin <= alpha    && alpha <= BoundsChecker::alphaMax; 
} 


void BoundsChecker::correctAlpha(double &alpha) 
{
  if( alpha < BoundsChecker::alphaMin)
    alpha = BoundsChecker::alphaMin; 
  if(  BoundsChecker::alphaMax < alpha )
    alpha = BoundsChecker::alphaMax; 
}


void BoundsChecker::correctBranch( BranchLengths &branch ) 
{
  auto lengths = branch.getLengths(); 

  for(auto &length : lengths )
    {
      auto val = length.getValue();
      if(val < BoundsChecker::zMin )
	length.setValue(BoundsChecker::zMin); 
      if(BoundsChecker::zMax < val)
	length.setValue(BoundsChecker::zMax); 
    }

  branch.setLengths(lengths);   
} 


void BoundsChecker::correctBranch(BranchLength &branch)
{
  auto length = branch.getLength(); 
  if(length.getValue() < BoundsChecker::zMin )
    length = BoundsChecker::zMin;
  if(BoundsChecker::zMax < length.getValue())
    length = BoundsChecker::zMax; 
  branch.setLength(length);   
}


void BoundsChecker::correctRevMat( std::vector<double> &rates)
{
  // because we have to convert the revmat from relative
  // representation to rate representation, this is more tricky.

  // note: rates are relative to last value here 

  for(auto &r : rates )
    {
      if( r < BoundsChecker::rateMin)
	r = BoundsChecker::rateMin; 
      else if(BoundsChecker::rateMax < r  ) 
	r = BoundsChecker::rateMax; 
    }  
}
 
void BoundsChecker::correctFrequencies( std::vector<double> &frequencies)
{
  // TODO 
  assert(0); 
  // used anywhere? 

  // determine number of freqs that are not okay 
  int numberOkay = 0; 
  auto  sum = double(0);  
  for(auto &f : frequencies)
    {
      if(f < BoundsChecker::freqMin) 
	f = BoundsChecker::freqMin; 
      else 
	  ++numberOkay; 
    }

  sum = 1. - (static_cast<double>(frequencies.size()) - numberOkay )  * BoundsChecker::freqMin; 

  // renormalize again 
  for(auto &f : frequencies)
    if(  std::fabs(f -  BoundsChecker::freqMin) > std::numeric_limits<double>::epsilon() ) 
	f /= sum; 
  
  // check to be sure that they add up to 1 
  sum = 0 ;
  for(auto &f : frequencies)
    sum += f; 
  assert(fabs(sum - 1.0 ) < 1e-6); 
} 

