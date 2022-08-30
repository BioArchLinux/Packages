/**
    @file globals.h

    @brief Global variables for ExaBayes.
    
    Notice that there are already a bunch of global variables from axml-variants.  
*/ 


#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <string>
#include <chrono> 
#include <memory>

#include <fstream>
#include "common.h"
#include "config.h"

#include <thread>
#include <mutex>
#include "TeeStream.hpp"

class AdHocIntegrator; 
class TreeIntegrator; 

#define tout (*teeOut) << SyncOut()

class TeeStream;


#ifdef _INCLUDE_DEFINITIONS


std::unique_ptr<TeeStream> teeOut; 
std::unique_ptr<std::ofstream> logStream;
std::mutex mtx;

// std::chrono::system_clock::time_point timeIncrement;  
int debugPrint = 0; 
bool startIntegration = false; 

AdHocIntegrator* ahInt; 
TreeIntegrator* tInt; 

bool isYggdrasil; 

void (*exitFunction)(int code, bool waitForAll); 

std::thread::id _masterThread; 
volatile bool _threadsDie; 

#else 

extern   std::unique_ptr<TeeStream> teeOut; 
extern std::unique_ptr<std::ofstream> logStream;
extern std::mutex mtx;
  

extern std::thread::id _masterThread; 
extern volatile bool  _threadsDie; 

extern void (*exitFunction)(int code , bool waitForAll); 

extern bool isYggdrasil; 
// extern int PLL_NUM_BRANCHES; 

extern bool startIntegration; 
extern AdHocIntegrator* ahInt; 
extern TreeIntegrator* tInt; 
// extern GlobalVariables globals; 
// extern std::chrono::system_clock::time_point timeIncrement;  
extern int debugPrint; 
#endif


#include "SyncOut.hpp"

#endif

