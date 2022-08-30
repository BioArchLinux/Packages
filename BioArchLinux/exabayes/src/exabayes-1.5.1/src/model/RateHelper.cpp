#include "RateHelper.hpp"

#include "Arithmetics.hpp"


#include <cmath>
#include <iostream>




nat RateHelper::numStateToNumInTriangleMatrix(int numStates)  
{  
  return (  numStates * numStates - numStates) / 2 ; 
}


std::vector<nat> RateHelper::extractIndices(nat num, nat numRates, const std::vector<double>  &rates) 
{
  // 1 2 3 4 5 X
  //   6 7 8 9 Y
  //     A B C Z
  //       D E F
  //         F F
  //           Y

  auto indices = std::vector<nat> {}; 
  if(num == 0)
    {
      for(nat i = 0; i < numRates-1; ++i)
	indices.push_back(i);
    }
  else 
    {
      --num;
      nat prevRows = 0; 
      for(nat i = 0; i < num+1 ; ++i)
	{
	  nat index = prevRows + num - i; 
	  indices.push_back(index);
	  prevRows += numRates - i - 1 ; 
	}

      for(nat i = 0 ; i < numRates - num - 2   ; ++i)
	indices.push_back(prevRows + i );
    }

  return indices; 
}


std::vector<double> RateHelper::extractSomeRates(nat num, nat numRates, std::vector<double> &rates)
{
  auto partRates = std::vector<double>{}; 
  auto indices = extractIndices(num, numRates, rates);

  for(auto index : indices)
    partRates.push_back(rates.at(index));  
  
  return partRates; 
}


void RateHelper::insertRates(nat num, nat numRates, std::vector<double> &rates, std::vector<double> &partRates)   
{
  auto indices = extractIndices(num, numRates, rates);

  nat ctr = 0; 
  for(auto index : indices)
    rates[index] = partRates.at(ctr++);
}


void RateHelper::convertRelativeToLast(std::vector<double> &values) 
{
  auto last = values.back();
  std::for_each(begin(values), end(values), [=](double &v ){v /= last; });
}


double RateHelper::convertToSum1(std::vector<double> &values) 
{
  // sorting reduces the error
  auto cpy = values; 
  std::sort(begin(cpy), end(cpy)); 

  double sum = Arithmetics::getKahansSum2(cpy) ; 

  //  we do not loose precision here 
  for (auto &v : values) 
    v /= sum; 
  
  return sum; 
}


void RateHelper::convertToGivenSum(std::vector<double> &values, double givenSum) 
{
  std::for_each(begin(values), end(values), [=](double &v){ v *= givenSum ;});
}


std::vector<double> RateHelper::getScaledValues(std::vector<double> values, double scParameter) 
{
  double scaler = scParameter * double(values.size()) ;  
  std::for_each(begin(values), end(values), [=](double &v){ v *= scaler; });
  return values; 
}

