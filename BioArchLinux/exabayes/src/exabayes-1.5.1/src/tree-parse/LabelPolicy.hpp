#ifndef INTEGER_LABEL_READER
#define INTEGER_LABEL_READER

#include "GlobalVariables.hpp"
#include <unordered_map>
#include <iostream>
#include <cassert>

typedef unsigned int nat; 

class IntegerLabelReader
{
public: 
  nat readLabel(std::istream &in)
  {
    nat result; 
    in >> result; 
    return result; 
  }

void setLabelMap(std::unordered_map<std::string,nat> map) {} 

}; 



class NameLabelReader
{
public: 
  NameLabelReader()
    : _name2id{}
  {
  }

  nat readLabel(std::istream &in)
  {
    auto label = std::string{}; 
    label.reserve( 1 << 10 );
    assert(_name2id.size() > 0 ); 

    // tout << "comparing against elems: " ; 
    // for(auto elem : _name2id)
    //   {
    // 	tout << std::get<0>(elem) << "," << std::get<1>(elem) << std::endl; 
    //   }

    bool foundDelim = false; 
    while(not foundDelim)
      {
	int ch = in.get();
	foundDelim = 
	  ( ch == ':'
	    || ch == ','
	    || ch == ')'
	    ) ; 
	if(not foundDelim)
	  label.push_back(char(ch));
      }
    in.unget();

    if(_name2id.find(label) == _name2id.end()) 
      {
	std::cerr << "Error: while parsing tree, could not find taxon >" << label << "<" << std::endl; 
	exitFunction(-1, false); 
      }

    return _name2id[label]; 
  }

  void setLabelMap(std::unordered_map<std::string,nat> map) { _name2id = map; } 


  std::unordered_map<std::string,nat> _name2id; 
}; 


#endif
