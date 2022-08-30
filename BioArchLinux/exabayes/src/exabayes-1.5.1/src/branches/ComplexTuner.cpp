#include "ComplexTuner.hpp"
#include <cmath>
#include "GlobalVariables.hpp"


double ComplexTuner::tuneParameter(int batch, double parameter, bool increase ) const 
{
  auto delta = 1. / (sqrt(   (batch + 1))); 

  if(delta > _maxDelta )
    delta = _maxDelta; 

  double val = _logScale ? log(parameter) : parameter; 
  if(increase)
    val += delta;
  else 
    val -= delta; 
  
  if(_logScale)
    val = exp(val); 

  val = fmax(_minBound, val); 
  val = fmin(_maxBound, val); 

  return val; 
} 

void ComplexTuner::tune() 
{
  auto curRatio = _sctr.getRatioInLastInterval();
  auto batch = _sctr.getBatch();

  if( curRatio < _prevSuccess )	
    _tuneUp = not _tuneUp; 
  _prevSuccess = curRatio; 

  auto newParam = tuneParameter(batch, _parameter, _tuneUp);

  _parameter = newParam; 
  _sctr.nextBatch();
}  


void ComplexTuner::deserialize( std::istream &in )   
{
  _parameter = cRead<decltype(_parameter)>(in); 
  _sctr.deserialize(in); 
  
  int tmp = cRead<int>(in); 
  _tuneUp = tmp != 0 ; 

  _prevSuccess = cRead<decltype(_prevSuccess)>(in); 
}

 
void ComplexTuner::serialize( std::ostream &out) const
{
  cWrite(out, _parameter); 
  _sctr.serialize(out);
  
  int tmp = _tuneUp ? 1 : 0 ; 
  cWrite(out, tmp); 

  cWrite(out, _prevSuccess); 
}  
