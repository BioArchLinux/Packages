#include "ProposalSet.hpp"


ProposalSet::ProposalSet(double relWeight, std::vector<std::unique_ptr<AbstractProposal> > _proposals)
  : Serializable()
  , relativeWeight(relWeight)
  , proposals{}
{
  for(auto &p : _proposals)
    proposals.emplace_back(p->clone()); 

  for(auto &p : proposals)
    p->setInSetExecution(true);
} 


ProposalSet::ProposalSet(const ProposalSet &rhs)
  : Serializable(rhs)
  , relativeWeight(rhs.relativeWeight)
  , proposals{}
{
  for(auto &p : rhs.proposals)
    proposals.emplace_back(p->clone());

  for(auto &p : rhs.proposals)
    p->setInSetExecution(true);
}


ProposalSet& ProposalSet::operator=(const ProposalSet &rhs)
{
  auto&& result = ProposalSet(rhs); 
  std::swap(result, *this); 
  return *this; 
}


std::vector<AbstractProposal*> ProposalSet::getProposalView() const
{
  auto result = std::vector<AbstractProposal*>{}; 
  for(auto &p : proposals)
    result.push_back(p.get());
  return result; 
}  


void ProposalSet::printVerboseAbbreviated(std::ostream &out, double sum) const
{
  out << relativeWeight / sum * 100 <<   "%\tSET(totalNumber=" << proposals.size() << "):" << std::endl;
  for(auto &p : proposals)
    {
      out << "\t"; 
      out << p->getId() << "\t"; 
      p->printShort(out); 
      out << std::endl; 
    }
}


nat ProposalSet::numerateProposals(nat ctr)
{
  for(auto &p : proposals)
    {
      p->setId(ctr); 
      ++ctr; 
    }

  return ctr; 
} 


void ProposalSet::deserialize( std::istream &in )
{
  for(auto &p : proposals)
    p->deserialize(in); 
}


void ProposalSet::serialize( std::ostream &out) const
{
  for(auto &p : proposals)
    p->serialize(out); 
}   


std::ostream& operator<<(std::ostream& out, const ProposalSet &rhs)
{
  out << "SET("; 
  
  std::unordered_set<std::string> pNames; 
  for(auto &p : rhs.proposals)
    pNames.insert(p->getName()); 

  out << pNames << ")"; 

  return out; 
}


bool ProposalSet::needsFullTraversal()
{
  bool result = true; 
  for(auto &p : proposals)
    result &= p->isNeedsFullTraversal();
  return result; 
}


void ProposalSet::setParameterListPtr(ParameterList* pPtr)
{
  for(auto &elem : proposals)
    elem->setParams(pPtr );
} 
