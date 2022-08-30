#include "BasicTreeReader.hpp"	// 
// #include "Branch.hpp"
#include "LabelPolicy.hpp"

#include <tuple>
#include <cassert>


template<class LABEL_READER,class BL_READER>
BasicTreeReader<LABEL_READER,BL_READER>::BasicTreeReader( nat numTax )
  : _highestInner{numTax + 1 }
  , lr{}
  , br{}
{
}

template<class LABEL_READER,class BL_READER>
inline void BasicTreeReader<LABEL_READER, BL_READER>::expectChar(std::istream &iss, int ch)
{
  int got = iss.get(); 
  if(ch != got)
    {
      std::cerr << "expected >" << char(ch) << "<, got >"  <<  char(got) << "<" << std::endl; 
      exitFunction(-1, false); 
    }
}


template<class LABEL_READER, class BL_READER>
std::tuple<nat,double> BasicTreeReader<LABEL_READER,BL_READER>::parseElement(std::istream &iss)
{
  auto bl = double{ nan("") };  
  auto label = lr.readLabel(iss);

  int ch = iss.peek(); 
  if(ch == ':')
    {
      expectChar(iss,':');
      bl = br.readBranchLength(iss);
    }
  else 
    {
      std::cerr << "did not find bl for label " << label << std::endl; 
    }

  return std::make_tuple(label, bl);
}


template<class LABEL_READER,class BL_READER>
void BasicTreeReader<LABEL_READER, BL_READER>::addBranch(nat label, std::tuple<nat,double> subtree, std::vector<BranchLength> &branches) const 
{
  auto label2 = 0u;
  auto len = 0.;

  std::tie(label2,len) = subtree;

  assert(label2 != 0u && label != 0u);

  branches.emplace_back(BranchPlain(label, label2),len);
}


template<class LABEL_READER,class BL_READER>
std::tuple<nat,double> BasicTreeReader<LABEL_READER, BL_READER>::parseSubTree(std::istream &iss, std::vector<BranchLength> &branches, bool toplevel) 
{
  int ch = iss.peek(); 
  auto label = nat{0}; 
  auto bl = double{nan("")};  

  if(ch != '(')			// must be a taxon label  
    {
      return parseElement(iss);
    }
  else 				// true tree 
    {
      expectChar(iss,'(');

      label = _highestInner; 
      ++_highestInner;

      auto res  = parseSubTree(iss,branches, false);
      addBranch(label, res, branches); 
      expectChar(iss,',');
      res = parseSubTree(iss,branches, false);
      addBranch(label, res, branches); 

      if(toplevel)
	{
	  expectChar(iss,','); 
	  res = parseSubTree(iss,branches, false); 
	  addBranch(label, res, branches); 
	}

      expectChar(iss,')'); 
      
      if(iss.peek() == ':'  )
	{
	  expectChar(iss,':');
	  bl = br.readBranchLength(iss);
	}
    }

  return std::make_tuple(label,bl); 
}


template<class LABEL_READER,class BL_READER>
std::vector<BranchLength> BasicTreeReader<LABEL_READER, BL_READER>::extractBranches(std::istream &iss ) 
{
  auto result = std::vector<BranchLength>{}; 

  parseSubTree(iss, result, true);
  expectChar(iss,';');

  return result; 
}


template class BasicTreeReader<IntegerLabelReader,ReadBranchLength>; 
template class BasicTreeReader<IntegerLabelReader,IgnoreBranchLength>; 
template class BasicTreeReader<NameLabelReader,ReadBranchLength>; 
template class BasicTreeReader<NameLabelReader,IgnoreBranchLength>; 
