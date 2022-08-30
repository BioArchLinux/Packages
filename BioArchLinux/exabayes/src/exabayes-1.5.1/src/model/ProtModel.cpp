#include "ProtModel.hpp"	
#include <algorithm>
#include <cctype>

#include <tuple>
#include <iostream>

std::ostream& operator<<(std::ostream &out, const ProtModel &rhs)
{
  out << ProtModelFun::getName(rhs); 
  return out; 
}




namespace ProtModelFun
{
  std::tuple<bool,ProtModel> getModelFromStringIfPossible(const std::string & modelString)
  {
    auto lower = [](const char c){ return tolower(c)  ; }; 
    auto lowInputModel = std::string{};
    std::transform(modelString.begin(), modelString.end(), std::back_inserter(lowInputModel),  lower);  
    
    for(auto model : getAllModels())
      {
	auto modelStringHere = getName(model);
	auto lowModel = std::string{}; 
	std::transform(modelStringHere.begin(), modelStringHere.end(), std::back_inserter(lowModel), lower);
	
	if( lowModel.compare(lowInputModel) == 0 )
	  return std::make_tuple(true, model); 
      }

    return std::make_tuple(false, ProtModel::FLU_T); 
  }
  

  std::vector<ProtModel> getAllModels()
  {
    return {ProtModel::DAYHOFF_T,
	ProtModel::DCMUT_T,
	ProtModel::JTT_T,
	ProtModel::MTREV_T,
	ProtModel::WAG_T,
	ProtModel::RTREV_T,
// 	ProtModel::LG4_T,
	ProtModel::CPREV_T,
	ProtModel::VT_T,
	ProtModel::BLOSUM62_T,
	ProtModel::MTMAM_T,
	ProtModel::LG_T,
	ProtModel::MTART_T,
	ProtModel::MTZOA_T,
	ProtModel::PMB_T,
	ProtModel::HIVB_T,
	ProtModel::HIVW_T,
	ProtModel::JTTDCMUT_T,
	ProtModel::FLU_T } ; 
  }


  std::string getName(ProtModel mod)
  {
    switch(mod)
      {
      case ProtModel::DAYHOFF_T:
	return "DAYHOFF";
      case ProtModel::DCMUT_T:
	return "DCMUT";
      case ProtModel::JTT_T:
	return "JTT";
      case ProtModel::MTREV_T:
	return "MTREV";
      case ProtModel::WAG_T:
	return "WAG";
      case ProtModel::RTREV_T:
	return "RTREV";
      case ProtModel::CPREV_T:
	return "CPREV";
      case ProtModel::VT_T:
	return "VT";
      case ProtModel::BLOSUM62_T:
	return "BLOSUM62";
      case ProtModel::MTMAM_T:
	return "MTMAM";
      case ProtModel::LG_T:
	return "LG";
      case ProtModel::LG4_T: 
	return "LG4"; 
      case ProtModel::MTART_T:
	return "MTART";
      case ProtModel::MTZOA_T:
	return "MTZOA";
      case ProtModel::PMB_T:
	return "PMB";
      case ProtModel::HIVB_T:
	return "HIVB";
      case ProtModel::HIVW_T:
	return "HIVW";
      case ProtModel::JTTDCMUT_T:
	return "JTTDCMUT";
      case ProtModel::FLU_T:
	return "FLU";
      default: 
      	{
      	  std::cerr << "error: encountered unknown protein model with internal value "<< int(mod) << std::endl; 
      	  assert(0); 
      	}
      }
  }
}
