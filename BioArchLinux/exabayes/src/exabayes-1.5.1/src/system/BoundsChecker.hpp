#ifndef _BOUNDS_CHECKER
#define _BOUNDS_CHECKER

#include <vector>
// #include "Branch.hpp"
#include "BranchLength.hpp"
#include "BranchLengths.hpp"


class BoundsChecker
{
public: 
  static const double zMin, zMax, 
    rateMax, rateMin, 
    alphaMin, alphaMax,
    freqMin; 

  static bool checkFrequencies( const std::vector<double> &freqs )  ; 
  static bool checkBranch( const BranchLength &branch ) ; 
  static bool checkBranch( const BranchLengths &branch ) ; 
  static bool checkRevmat( const std::vector<double> &rates) ; 
  static bool checkAlpha( double alpha) ; 
  

  static void correctAlpha(double &alpha) ;   
  static void correctBranch( BranchLengths &branch ) ; 
  static void correctBranch(BranchLength &branch); 
  static void correctRevMat( std::vector<double> &rates); 
  static void correctFrequencies( std::vector<double> &frequencies); 

}
 ; 


#endif
