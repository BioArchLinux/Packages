#ifndef BRANCH_LENGTH_POLICY
#define BRANCH_LENGTH_POLICY

#include <limits>
#include <iosfwd>
#include <cmath>

class IgnoreBranchLength
{
public: 
  double readBranchLength(std::istream &iss)
  {
    bool foundNext = false; 
    while(not foundNext)
      {
	int ch = iss.get(); 

	foundNext = ( ch == ')'
		      || ch == ';'
		      || ch == ',' ) ; 
      }
    iss.unget();
    return nan("");
  }
};


class ReadBranchLength
{
public: 
  double readBranchLength(std::istream &iss)
  {
    auto result = double{0.};

    iss.precision(std::numeric_limits<double>::digits10 + 2);

    iss >> result; 
    return result; 
  }
  
};

#endif
