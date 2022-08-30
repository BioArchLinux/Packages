#ifndef _OPTIMIZED_PARAMETER_HPP
#define _OPTIMIZED_PARAMETER_HPP


class TreeAln; 

#include "AbstractParameter.hpp"

// #include "Branch.hpp"
#include "BranchPlain.hpp"
#include "DistributionProposer.hpp"



class OptimizedParameter
{
public: 
  OptimizedParameter( TreeAln& traln, const BranchPlain& branch, AbstractParameter* param, int maxIter ); 
  OptimizedParameter(const OptimizedParameter& rhs) = default; 
  OptimizedParameter& operator=(const OptimizedParameter &rhs) = default; 

  template<class T> 
  auto getProposerDistribution(TreeAln &traln, double convParameter, double nonConvParameter) const 
    -> DistributionProposer<T> ; 

  void applyToMask (std::vector<bool> &mask)  const; 
  void applyValues(double *values) const ; 

  BranchLength getOptimizedBranch() const ;  

  AbstractParameter* getParam() const {return _param; }

  bool isCurvatureOk() const {return _curvatOK; }

  bool hasConvergedNew() const ; 
  bool hasFinished() const ; 

  void resetStep(); 
  void changeSide(); 

  void decrIter() {--_iters; }

  void shortenBadBranch(); 
  void nrStep(); 

  void setToTypicalBranch(double typicalAbsLen, TreeAln& traln ); 

  void extractDerivatives( TreeAln &traln, std::vector<double> &dlnLdlz, std::vector<double> &d2lnLdlz2); 

  /** @brief check if we ar done for partition i or if we need to
      adapt the branch length again */
  void doInitStep(); 

  static const double zMin ; 
  static const double zMax; 

  double getFirstDerivative() const {return _nrD1; }
  double getSecondDerivative() const {return _nrD2; }
  double getOptimum() const {return _zCur;  }

  void checkConvergence() ; 

  BranchPlain getBranch() const 
  {
    return _branch; 
  }

  
  friend std::ostream& operator<<(std::ostream& out, const OptimizedParameter& rhs)
  {
    out << rhs.getOptimum() << "," << rhs.getFirstDerivative() << ","  << rhs.getSecondDerivative() ; 
    return out ; 
  }

private: 
  double _zPrev; 
  double _zCur; 
  double _zStep; 
  double _coreLZ; 

  double _nrD1; 
  double _nrD2; 
  
  int _iters; 
  
  bool _curvatOK; 
  AbstractParameter* _param; 

  bool _hasConverged; 
  BranchPlain _branch; 
}; 


template<class T> 
DistributionProposer<T> 
OptimizedParameter::getProposerDistribution(TreeAln &traln, double convParameter, double nonConvParameter) const   
{
  // tout << "init with factor "  << factor << std::endl; 
  auto bl =  BranchLength(BranchPlain(1,2));
  bl.setLength(_zCur); 
  auto realLen = bl.toMeanSubstitutions( _param->getMeanSubstitutionRate());
  return DistributionProposer<T>(realLen, _nrD1, _nrD2, convParameter, nonConvParameter ); 
}



#endif


