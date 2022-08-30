#include "TimeTracker.hpp"

TimeTracker::TimeTracker(size_t numCpu)
  : _timePoint{getTimePoint()}
  , _accWallTime{0.}
  , _accCpuTime{0.}
  , _numCpu{numCpu}
{
}


double TimeTracker::getDuration(CLOCK::system_clock::time_point tp )
{
  return CLOCK::duration_cast<CLOCK::duration<double> > (CLOCK::system_clock::now()- tp   ).count(); 
} 


CLOCK::system_clock::time_point TimeTracker::getTimePoint()
{
  return CLOCK::system_clock::now(); 
} 


void TimeTracker::deserialize( std::istream &in )
{
  _accWallTime = cRead<decltype(_accWallTime)>(in);
  _accCpuTime = cRead<decltype(_accCpuTime)>(in); 
}


void TimeTracker::serialize( std::ostream &out) const 
{
  cWrite<decltype(_accWallTime)>(out, _accWallTime);
  cWrite<decltype(_accCpuTime)>(out, _accCpuTime);
}



double TimeTracker::getRecentElapsed() const 
{
  return getDuration(_timePoint); 
}

 
void TimeTracker::updateTime()
{
  auto wallTimeElapsed = getDuration(_timePoint); 
  _accWallTime += wallTimeElapsed; 
  _accCpuTime += double(_numCpu) * wallTimeElapsed; 
  _timePoint = getTimePoint();
}  


std::array<double,3>
TimeTracker::formatForWatch(double time )  
{
  auto altFormat = std::array<double,3>();

  auto tmp = time; 
  altFormat[0] = int(tmp / 3600); 
  tmp -= altFormat[0] * 3600 ; 
  altFormat[1] = int(tmp / 60); 
  tmp -= altFormat[1] * 60 ; 
  altFormat[2] = tmp ; 

  return altFormat; 
}
