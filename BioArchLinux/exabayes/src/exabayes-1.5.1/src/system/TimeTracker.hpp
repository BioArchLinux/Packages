#ifndef TIMETRACKER_H
#define TIMETRACKER_H

#include <chrono> 
#include <ratio>
#include <array>
#define CLOCK std::chrono  

#include "Serializable.hpp"


class TimeTracker : public Serializable
{
public: 
  virtual void deserialize( std::istream &in ); 
  virtual void serialize( std::ostream &out) const;

public: 
  TimeTracker(size_t numCpu);
  TimeTracker(const TimeTracker& rhs) = default; 
  TimeTracker( TimeTracker&& rhs) = default; 
  TimeTracker& operator=(const TimeTracker &rhs)  = default; 
  TimeTracker& operator=( TimeTracker &&rhs)  = default; 

  double getRecentElapsed() const ; 
  void updateTime(); 
  
  double getAccWallTime()const {return _accWallTime; } 
  double getAccCpuTime() const {return _accCpuTime; } 

  static double getDuration(CLOCK::system_clock::time_point tp );
  static CLOCK::system_clock::time_point getTimePoint(); 
  static std::array<double,3> formatForWatch(double time )  ; 

private:
  CLOCK::system_clock::time_point _timePoint; 

  double _accWallTime; 
  double _accCpuTime; 

  size_t  _numCpu; 
}; 


#endif /* TIMETRACKER_H */
