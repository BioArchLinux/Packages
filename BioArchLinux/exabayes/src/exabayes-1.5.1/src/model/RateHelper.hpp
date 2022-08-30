/** 
    @file RateHelper.hpp

    @brief various functions helping to deal with rates and substition matrices

    @notice using a class for this may appear over-engineered. But
    conversion between rate formats is not unproblematic, probably a
    Kahan summation may come in handy at some point.
    
 */ 


#ifndef ABSTRACT_RATE_PROPOSER
#define ABSTRACT_RATE_PROPOSER

#include <vector>
#include <algorithm>
#include "common.h"

class RateHelper
{
public: 
  RateHelper(){} 

  static void convertRelativeToLast(std::vector<double> &values) ; 
  static double convertToSum1(std::vector<double> &values); 
  static void convertToGivenSum(std::vector<double> &values, double givenSum); 
  static std::vector<double> getScaledValues(std::vector<double> values, double scParameter); 
  static std::vector<nat> extractIndices(nat num, nat numRates, const std::vector<double>  &rates) ; 
  static void insertRates(nat num, nat numRates, std::vector<double> &rates, std::vector<double> &partRates); 
  static std::vector<double> extractSomeRates(nat num, nat numRates, std::vector<double> &rates); 
  static nat numStateToNumInTriangleMatrix(int numStates)  ; 
}; 



#endif

