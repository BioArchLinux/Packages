/**
   @file densities.h
   
   @brief contains densities and distributions
*/ 


#ifndef _DENSITIES_H
#define _DENSITIES_H

#include <vector>
#include "extensions.hpp"

double exponentialDensity(double value, double lambda); 


/** @brief density for dirichlet distribution with parameters "alphas" at point "values" */ 
/* double densityDirichlet(double *values, double *alphas, int length);  */


/** @brief the gamma function */ 
// double gammaFunction(double alpha); 

namespace Density
{
  log_double lnDirichlet(std::vector<double> values, const std::vector<double> &alphas); 
  log_double lnExponential(double value, double lambda); 
  log_double lnGamma(double x, double alpha, double beta ); 
  log_double lnWeibull(double x, double lambda, double k) ; 
  double gammaFunction(double alpha); 
} 
#endif
