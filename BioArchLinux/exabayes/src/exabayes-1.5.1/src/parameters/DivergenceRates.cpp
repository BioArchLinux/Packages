#include "DivergenceRates.hpp"

#include "Category.hpp"

DivergenceRates::DivergenceRates(nat id, nat idOfMyKind,
                                 std::vector<nat> partitions, nat numberOfTaxa) :
  AbstractParameter(Category::DIVERGENCE_RATES, id, idOfMyKind,
                    partitions, 0), _rateAssignments(2 * numberOfTaxa - 2, 0), _rates(
                                                                                      2 * numberOfTaxa - 2, 1.), _directedBranches(
                                                                                                                                   2 * numberOfTaxa - 2)
{
  /*
   * we initialize as many different rate categories as branches in the tree, all to 1.0
   */
  for (int i = 0; i < _rateAssignments.size(); i++)
    {
      _rateAssignments[i] = i;
      _rates[i] = 1;
    }
}



DivergenceRates::DivergenceRates(const DivergenceRates& rhs)
  : AbstractParameter(rhs)
  , _rateAssignments{rhs._rateAssignments}
  , _rates{rhs._rates}    
  , _directedBranches{rhs._directedBranches}
{
} 



void DivergenceRates::initializeParameter(TreeAln& traln,
                                          const ParameterContent &content)
{
  assert(content.branchLengths.size() == traln.getNumberOfNodes());
  for (int i = 0; i < traln.getNumberOfNodes(); i++)
    {
      _directedBranches[i] = content.branchLengths[i];
    }

}

AbstractParameter* DivergenceRates::clone() const
{
  return new DivergenceRates(*this);; 
}



void DivergenceRates::applyParameter(TreeAln& traln,
                                     const ParameterContent &content)
{
  verifyContent(traln, content);

  if (content.rateAssignments != _rateAssignments)
    {
      /* modification in the assignments */
      assert(0);
      /* update assignments */
      _rateAssignments = content.rateAssignments;
      //_rates.resize(_ra)
    }
  else
    {
      /* modification in the rates */
      for (int i = 0; i < _rates.size(); i++)
        {
          if (content.values[i] != _rates[i])
            {
              _rates[i] = content.values[i];
              for (int j = 0; j < _rateAssignments.size(); j++)
                {
                  if (_rateAssignments[j] == i)
                    {
                      traln.setBranch(content.branchLengths[j], this);
                    }
                }
            }
        }
    }
}

ParameterContent DivergenceRates::extractParameter(const TreeAln &traln) const
{
  auto content = ParameterContent();
  content.values = _rates;
  content.rateAssignments = _rateAssignments;
  for (int i = 0; i < _rates.size(); i++)
    {
      content.branchLengths.push_back(
                                      traln.getBranch(_directedBranches[i], this));
    }
  verifyContent(traln, content);
  return content;
}

void DivergenceRates::printSample(std::ostream& fileHandle,
                                  const TreeAln &traln) const
{
  bool isFirst = true;
  for (auto v : _rates)
    {
      fileHandle << (isFirst ? "" : "\t") << v;
      isFirst = false;
    }
}

void DivergenceRates::printAllComponentNames(std::ostream &fileHandle,
                                             const TreeAln &traln) const
{
  bool isFirstG = true;
  for (nat i = 0; i < _rates.size(); ++i)
    {
      fileHandle << (isFirstG ? "" : "\t") << "r{";
      isFirstG = false;

      bool isFirst = true;
      for (auto &p : _partitions)
        {
          fileHandle << (isFirst ? "" : ",") << p;
          isFirst = false;
        }
      fileHandle << "}(" << i << ")";
    }
}

void DivergenceRates::verifyContent(const TreeAln &traln,
                                    const ParameterContent &content) const
{
  /*
   * content must have
   * 1) rateAssignments (nat) for every node
   * 2) branch lengths (BranchLength) for every node
   * 3) rateValues (double) for every rateAssignment
   */
  assert(content.values.size() > 0);
  assert(content.rateAssignments.size() == traln.getNumberOfNodes());
  assert(content.branchLengths.size() == _directedBranches.size());
  for (int i = 0; i < _directedBranches.size(); i++)
    {
      assert(content.branchLengths[i] == _directedBranches[i]);
    }

  /* root branch has the same value for both nodes */
  //	auto rb = traln.getRootBranch();
  //	assert(
  //			content.branchLengths[rb.getPrimNode() - 1]
  //					== content.branchLengths[rb.getSecNode() - 1]);
}

log_double DivergenceRates::getPriorValue(const TreeAln& traln) const
{

  /* TODO-divtimes check this */
  auto result = log_double::fromAbs(1);

  // that only partly makes sense ...
  // result *= _prior->getLogProb( _rates);

  return result;
}

void DivergenceRates::setPrior(const std::unique_ptr<AbstractPrior> &prior)
{
  // meh meh meh

  AbstractParameter::setPrior(prior);
  //_rates.resize(prior->getInitialValue().values.size());
}
