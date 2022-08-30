#ifndef PROT_MODEL_HPP_
#define PROT_MODEL_HPP_

#include <vector>
#include <iostream>
#include <string>
#include <cassert>

#include "pll.h"

enum class ProtModel : int 
{
  DAYHOFF_T =    PLL_DAYHOFF,
    DCMUT_T =      PLL_DCMUT,
    JTT_T =        PLL_JTT,
    MTREV_T =      PLL_MTREV,
    WAG_T =        PLL_WAG,
    RTREV_T =      PLL_RTREV,
    CPREV_T =      PLL_CPREV,
    VT_T =         PLL_VT,
    BLOSUM62_T =   PLL_BLOSUM62,
    MTMAM_T =      PLL_MTMAM,
    LG_T =         PLL_LG,
    MTART_T =      PLL_MTART,
    MTZOA_T =      PLL_MTZOA,
    PMB_T =        PLL_PMB,
    HIVB_T =       PLL_HIVB,
    HIVW_T =       PLL_HIVW,
    JTTDCMUT_T =   PLL_JTTDCMUT,
    FLU_T =        PLL_FLU, 
    LG4_T =        PLL_LG4
}; 


namespace ProtModelFun
{
  std::string getName(ProtModel mod); 
  std::vector<ProtModel> getAllModels();
  std::tuple<bool,ProtModel> getModelFromStringIfPossible(const std::string & modelString); 
}

std::ostream& operator<<(std::ostream &out, const ProtModel &rhs); 

namespace std
{
  template<> struct less<ProtModel> {
    bool operator()(const ProtModel& a, const ProtModel& b)  const 
    {
      return int(a) < int(b);
    } 
  }; 


  template<> struct hash<ProtModel>
  {
    size_t operator() (const ProtModel& rhs) const 
    {
      return std::hash<size_t>()(size_t(rhs));
    }
  }; 
}


#endif
