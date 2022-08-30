#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <numeric>
#include "Density.hpp"

#include "common.h"
#include "extensions.hpp"

// bracen copy from mrb
static log_double logGamma (double alp)
{
  double 
    x = alp, f = 0.0, z;
	
  if (x < 7) 
    {
      f = 1.0;
      z = x-1.0;
      while (++z < 7.0)  
	f *= z;
      x = z;   
      f = -log(f);
    }
  z = 1.0 / (x*x);
  return  log_double::fromLog(f + (x-0.5)*log(x) - x + 0.918938533204673 + 
	   (((-0.000595238095238*z+0.000793650793651)*z-0.002777777777778)*z +
	    0.083333333333333)/x);  

}


static log_double betaFunctionLog(const std::vector<double> &alphas)
{
  log_double beta = log_double::fromAbs(1.);
  double sum=0;
  for(auto v : alphas)
    {
      beta *= log_double::fromLog(lgamma(v)); 
      sum+=v ;    
    }
  beta /= log_double::fromLog(lgamma(sum));

  return beta;
}



namespace Density
{
  log_double lnDirichlet(std::vector<double> values, const std::vector<double> &alphas)
  {
    log_double density= log_double::fromAbs(1.);
    density /= betaFunctionLog(alphas);
    
    double sum = std::accumulate(begin(values), end(values), 0. ); 
    if( fabs( sum - 1. )  > 1e-6)
      {
	std::cout << "error: expected sum of 1, got " << sum << std::endl; 
	assert(0); 
      }      

    for(nat i=0; i<values.size(); i++)
      density *= log_double::fromLog((alphas[i] - 1 ) * log(values[i])); 

    return density; 
  }

  // double dirichlet(std::vector<double> values, const std::vector<double> &alphas)
  // {
  //   assert(0); 			// deprecated 
  //   double density=1;  
  //   density /= betaFunction(alphas);  
  //   assert(fabs(std::accumulate(values.begin(), values.end(), 0.) -  1.0 )  < 1e-6); 

  //   for(nat i=0; i< values.size(); i++)
  //     density *= pow( values[i],alphas[i]-1);
  
  //   return density; 
  // }

  
  log_double lnExponential(double value, double lambda)
  {
    return log_double::fromLog(log(lambda) - lambda * value); 
  }

  // double exponential(double value, double lambda)
  // {
  //   assert(0); 			// deprecated 
  //   return lambda * exp(- lambda * value); 
  // }
  

  log_double lnGamma (double x, double alpha, double beta )
  {    
    auto result =  (alpha-1.0) * log(x) + alpha * log(beta) - x * beta - logGamma(alpha).getRawLog();  
    return log_double::fromLog(result); 
  }


  log_double lnWeibull(double x, double lambda, double k) 
  {
    if(x < 0 )
      return log_double::lowest();
    else 
      return log_double::fromLog( log(k / lambda)  + (k-1) * log( x / lambda)  - pow( x / lambda, k)); 
  } 

  double gammaFunction(double alpha)
  {
  
    double lgam = lgamma(alpha);
    return signgam*exp(lgam); 
  }

}  
