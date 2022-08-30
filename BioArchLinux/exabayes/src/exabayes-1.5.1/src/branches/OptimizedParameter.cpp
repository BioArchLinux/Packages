#include "OptimizedParameter.hpp"
#include "AbstractParameter.hpp"

#include "TreeAln.hpp"

const double OptimizedParameter::zMin = PLL_ZMIN; 
const double OptimizedParameter::zMax = PLL_ZMAX; 


OptimizedParameter::OptimizedParameter( TreeAln& traln, const BranchPlain& branch, AbstractParameter* param, int maxIter)
  : _zPrev{ std::numeric_limits<double>::max() }  
  , _zCur{traln.getBranch(branch,param).getLength().getValue()}
  , _zStep{(1.0 - zMax) * _zCur + zMin }   
  , _coreLZ{0}			// okay???
  , _nrD1{0}
  , _nrD2{0}
  , _iters{maxIter}
  , _curvatOK{true}
  , _param{param}
  , _hasConverged(false)
  , _branch{branch}
{
}


void OptimizedParameter::applyToMask (std::vector<bool> &mask)  const
{
  if(not _curvatOK)
    {
      for(auto &partition : _param->getPartitions())
	mask[partition] = true; 
    }
} 


void OptimizedParameter::extractDerivatives( TreeAln &traln, std::vector<double> &dlnLdlz, std::vector<double> &d2lnLdlz2)
{
  _nrD1 = 0; 
  _nrD2 = 0; 

  for(auto p : _param->getPartitions())
    {
      _nrD1 += dlnLdlz[p]; 
      _nrD2 += d2lnLdlz2[p]; 
    }

  _nrD1 += _param->getPrior()->getFirstDerivative( *_param ); 
}


void OptimizedParameter::shortenBadBranch()
{
  if ((_nrD2 >= 0.0) && (_zCur < zMax))
    {
      _zPrev = _zCur = 0.37 * _zCur + 0.63;  /*  Bad curvature, shorten branch */
#ifdef VERBOSE
      tout << this << " shortening branch" << std::endl; 
#endif
    }
  else
    {
#ifdef VERBOSE
      tout << this  << " curv okay now" << std::endl; 
#endif
      _curvatOK = true;
    }
}


bool OptimizedParameter::hasConvergedNew() const 
{
  return _hasConverged; 
}


bool OptimizedParameter::hasFinished() const 
{
  return hasConvergedNew() || (_iters <= 0) ; 
}


void OptimizedParameter::nrStep()
{
  if (_nrD2 < 0.0)
    {
      double tantmp = - _nrD1 /  _nrD2;
      if (tantmp < 100)
	{
	  _zCur *= std::exp(tantmp);
	  if (_zCur < zMin)
	    _zCur = zMin;

	  if (_zCur > 0.25 * _zPrev + 0.75)
	    _zCur = 0.25 * _zPrev + 0.75;
	}
      else
	_zCur = 0.25 * _zPrev + 0.75;

#ifdef VERBOSE
      tout << this << " step: " << _zCur << std::endl; 
#endif
    }

  if (_zCur > zMax) 
    {
      _zCur = zMax;
#ifdef VERBOSE
      tout  << this << "setting to max" << std::endl; 
#endif
    }
} 


void OptimizedParameter::resetStep()
{
  _curvatOK = false; 
  _zPrev = _zCur;
  _zStep = (1.0 - zMax) * _zCur + zMin;
#ifdef VERBOSE
  tout << this << "set step to "<< _zStep << std::endl; 
#endif
}


void OptimizedParameter::changeSide()
{
  if (_zCur < zMin) 
    _zCur = zMin; 
  else if (_zCur > zMax) 
    _zCur = zMax;
  _coreLZ = log(_zCur); 
#ifdef VERBOSE
  tout << this << " changed side" <<  SHOW(_zCur) <<  std::endl; 
#endif
}


void OptimizedParameter::applyValues(double *values) const 
{
  for( auto &partition : _param->getPartitions() )
    values[partition] = _coreLZ; 
} 


void OptimizedParameter::checkConvergence()  
{
  _hasConverged = (fabs(_zCur - _zPrev)  <= _zStep) || ( _iters <= 0 ); 
#ifdef VERBOSE
  if(_hasConverged)
    tout << this << " has converged" << std::endl; 
#endif
}



void OptimizedParameter::setToTypicalBranch(double typicalAbsLen, TreeAln& traln ) 
{
#if 0   
  auto b = BranchLength(0,0);
  b.setConvertedInternalLength(traln, _param, typicalAbsLen);

  if(BoundsChecker::checkBranch(b))
    BoundsChecker::correctBranch(b); 

  _zCur = b.getLength();
#else 
  _zCur = 0.9; 
#endif
}


BranchLength OptimizedParameter::getOptimizedBranch() const 
{
  // auto result = _branch.toBlDummy();
  // result.setLength(_zCur);

  auto result = BranchLength(_branch, InternalBranchLength(_zCur)); 

  if(not BoundsChecker::checkBranch(result))
    BoundsChecker::correctBranch(result); 

  // tout << "optimized branch: " << _zCur << std::endl; 
  return result; 
}  



