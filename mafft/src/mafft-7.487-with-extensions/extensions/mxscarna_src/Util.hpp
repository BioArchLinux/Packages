#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <cctype>
#include <climits>
#include <cfloat>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <numeric>
#include <string>
#include <sstream>
#include <stack>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
//#include <pthread.h> // mingw de error, by katoh
#include <signal.h>

namespace ProbCons {
    
const double IMPOSSIBLE = -FLT_MAX + 1000;
const double IMPOSSIBLEDBL = -DBL_MAX + 10000;

namespace MXSCARNA {
template <typename T>
inline bool
IsPossible(const T& v) { 
    return (v > (IMPOSSIBLE + 1.0e-5));
}

template <typename T>
inline T
logSum(const T& a, const T& b)
{
    if (a >= b) {
	return a + log(1.0 + exp(b - a));
    } else {
	return b + log(1.0 + exp(a - b));
    }
}

template <typename T>
inline T
logSub(const T&a, const T& b)
{
  if(a > b) {
    return log(exp(a) - exp(b));
  }
  else {
    return log(exp(a) - exp(b));
  }
}

template <typename T>
inline T
logSum(const T& a, const T& b, const T& c)
{
    if (a >= b) {
	if( a >= c ) {
	    return a + log(1.0 + (exp(b - a) + exp(c - a)));
	}
	else {
	    if( b >= c) {
		return b + log(exp(a - b) + 1.0 + exp(c - b));
	    }
	}
    }
    return c + log(exp(a - c) + exp(b - c) + 1.0);
}

}

template <typename T>
inline T
logSumExp(const T& x, const T& y)
{
    if(x == y) return x + 0.69314718055;
    double vmin = std::min(x, y);
    double vmax = std::max(x, y);
    
    if (vmax > vmin + 50) {
	return vmax;
    }
    else {
	return vmax + std::log (std::exp (vmin - vmax) + 1.0);
    }
}
}
#endif /* UTIL_H */
