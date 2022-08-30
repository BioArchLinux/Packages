#ifndef DIVERGENCERATE_H
#define DIVERGENCERATE_H

#include "AbstractParameter.hpp"

class DivergenceRates: public AbstractParameter
{
public:
	DivergenceRates(nat id, nat idOfMyKind, std::vector<nat> partitions,
			nat numberOfTaxa);

        DivergenceRates(const DivergenceRates& rhs);  
  
	virtual ~DivergenceRates()
	{
	}

	void initializeParameter(TreeAln& traln, const ParameterContent &content);
	virtual void applyParameter(TreeAln& traln,
			const ParameterContent &content);
	virtual ParameterContent extractParameter(const TreeAln &traln) const;
	virtual void printSample(std::ostream& fileHandle,
			const TreeAln &traln) const;
	virtual void printAllComponentNames(std::ostream &fileHandle,
			const TreeAln &traln) const;
	virtual void verifyContent(const TreeAln &traln,
			const ParameterContent &content) const;

	virtual void setPrior(const std::unique_ptr<AbstractPrior> &prior);

	void setRates(std::vector<double> rates)
	{
		_rates = rates;
	}

	virtual AbstractParameter* clone() const;

	virtual log_double getPriorValue(const TreeAln& traln) const;

protected:
	std::vector<nat> _rateAssignments;
	std::vector<double> _rates;
	std::vector<BranchPlain> _directedBranches;
};

#endif /* DIVERGENCERATE_H */
