#ifndef _PARAMETER_SET_HPP 
#define _PARAMETER_SET_HPP 

#include "AbstractParameter.hpp"
#include "BranchLengthsParameter.hpp"

/** 
    @brief represents a set of parameters, that we integrate over 
    
    this object owns the respective parameters   
 */ 
class ParameterList
{
public: 
  ParameterList(std::vector< std::unique_ptr<AbstractParameter> > params  = std::vector< std::unique_ptr<AbstractParameter> >{}) ; 
  ~ParameterList(){}
  ParameterList(ParameterList&& rhs) = default; 
  ParameterList(const ParameterList& rhs)  ; 

  ParameterList& operator=(ParameterList rhs); 

  friend void swap(ParameterList& lhs, ParameterList& rhs); 

  // boring stuff to make it behave like a container 
  const AbstractParameter* at(int num) const { return _paramView.at(num); }
  AbstractParameter* at(int num) {return _paramView.at(num); }
  std::vector<AbstractParameter*>::const_iterator begin() const { return _paramView.cbegin(); }
  std::vector<AbstractParameter*>::const_iterator end() const { return _paramView.cend(); }
  std::vector<AbstractParameter*>::iterator begin() { return _paramView.begin(); }
  std::vector<AbstractParameter*>::iterator end() { return _paramView.end(); }
  AbstractParameter* operator[] (int num) { return _paramView.at(num); }
  const AbstractParameter* operator[] (int num) const { return _paramView.at(num); }
  
  friend std::ostream& operator<<(std::ostream& s, const ParameterList& c) ; 

  
  std::vector<AbstractParameter*> getViewByCategory(Category cat) const ; 

private: 
  std::vector< std::unique_ptr<AbstractParameter> > _params; 
  std::vector<AbstractParameter*> _paramView; 
};  



#endif
