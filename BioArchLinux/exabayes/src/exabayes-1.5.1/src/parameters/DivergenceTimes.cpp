#include "DivergenceTimes.hpp"
#include "Category.hpp"

DivergenceTimes::DivergenceTimes(nat id, nat idOfMyKind,
		std::vector<nat> partitions, NodeAge age) :
		AbstractParameter(Category::DIVERGENCE_TIMES, id, idOfMyKind,
				partitions, 0), 
                _rootNode(false), _nodeAge { age }
{
}

void DivergenceTimes::initializeParameter(TreeAln& traln,
		const ParameterContent &content, bool root)
{

	if (root)
	{
		/* root case */
		/*
		 * in this case the nodeAges vector contains the set of "root children" ages and
		 * one additional age for the root itself.
		 */
		_nodeAge.setHeight(
				content.nodeAges[content.nodeAges.size() - 1].getHeight());
		/* update the root branch */
		double time = 2 * _nodeAge.getHeight() - content.nodeAges[0].getHeight()
				- content.nodeAges[1].getHeight();
		if (content.nodeAges.size() == 2)
		{
			/* in case one of the root children is a tip, its height is zero */
			time += _nodeAge.getHeight();
		}
		/* we assume that the current rate at initialization is 1.0 */
		double newLength = exp(-time / traln.getTrHandle().fracchange);
		auto newBranch = BranchLength(traln.getRootBranch(), newLength);
		traln.setBranch(newBranch, this);
	}
	else
	{
		_nodeAge.setHeight(content.nodeAges[0].getHeight());
		_nodeAge.setPrimNode(content.nodeAges[0].getPrimNode());
		_nodeAge.setSecNode(content.nodeAges[0].getSecNode());

		/* initialize parental branch length */
		if (!traln.isRootBranch(_nodeAge))
		{
			auto bl = traln.getBranch(_nodeAge, this);
			auto length = bl.getLength();

			double time = content.nodeAges[1].getHeight()
					- _nodeAge.getHeight();

			/* we assume that the current rate at initialization is 1.0 */
			double newLength = exp(-time / traln.getTrHandle().fracchange);

			auto newBranch = BranchLength(_nodeAge, newLength);
			traln.setBranch(newBranch, this);
		}

		/* initialize descendant branch length */
		auto descendants = traln.getDescendents(_nodeAge);
		if (traln.isTipBranch(descendants.first))
		{
			double time = _nodeAge.getHeight();
			double newLength = exp(-time / traln.getTrHandle().fracchange);
			BranchLength newBranch = BranchLength(descendants.first, newLength);
			traln.setBranch(newBranch, this);
		}
		if (traln.isTipBranch(descendants.second))
		{
			double time = _nodeAge.getHeight();
			double newLength = exp(-time / traln.getTrHandle().fracchange);
			BranchLength newBranch = BranchLength(descendants.second,
					newLength);
			traln.setBranch(newBranch, this);
		}
	}

	_rootNode = !(_nodeAge.getPrimNode() + _nodeAge.getSecNode());
}


void DivergenceTimes::applyParameter(TreeAln& traln,
		const ParameterContent &content)
{
	BranchLength newBranch;

	verifyContent(traln, content);
	if (!content.nodeAges.size())
		return;

#ifdef DIVTIME_DEBUG
	std::cout << "DIV-TIMES-DBG: - - Time shift from " << _nodeAge.getHeight() << " to "
	<< content.nodeAges[0].getHeight() << " - delta " << content.nodeAges[0].getHeight() - _nodeAge.getHeight() << std::endl;
#endif

	_nodeAge.setHeight(content.nodeAges[0].getHeight());
	if (_rootNode)
	{
		newBranch = BranchLength(traln.getRootBranch(), content.values[1]);
#ifdef DIVTIME_DEBUG
		std::cout << "DIV-TIMES-DBG: - - Set branch (r) " << content.values[1] << std::endl;
#endif
		traln.setBranch(newBranch, this);
	}
	else
	{
		newBranch = BranchLength(_nodeAge, content.values[0]);
#ifdef DIVTIME_DEBUG
		std::cout << "DIV-TIMES-DBG: - - Set branch (p) " << content.values[0] << std::endl;
#endif
		traln.setBranch(newBranch, this);
		auto descendants = traln.getDescendents(_nodeAge);
		if (content.values[1] > 0.0)
		{ // traln.isTipBranch(descendants.first)) {
			newBranch = BranchLength(descendants.first, content.values[1]);
#ifdef DIVTIME_DEBUG
			std::cout << "DIV-TIMES-DBG: - - Set branch (c1) " << content.values[1] << std::endl;
#endif
			traln.setBranch(newBranch, this);
		}
		if (content.values[2] > 0.0)
		{ // traln.isTipBranch(descendants.second)) {
			newBranch = BranchLength(descendants.second, content.values[2]);
#ifdef DIVTIME_DEBUG
			std::cout << "DIV-TIMES-DBG: - - Set branch (c2) " << content.values[1] << std::endl;
#endif
			traln.setBranch(newBranch, this);
		}
	}
}

ParameterContent DivergenceTimes::extractParameter(const TreeAln &traln) const
{
	auto content = ParameterContent();
	content.nodeAges.push_back(_nodeAge);

	if (_rootNode)
	{
		content.values.push_back(0.0); // no parental branch
		content.values.push_back(
				traln.getBranch(traln.getRootBranch(), this).getLength().getValue());
		content.values.push_back(
				traln.getBranch(traln.getRootBranch(), this).getLength().getValue());
	}
	else
	{
		content.values.push_back(
				traln.getBranch(_nodeAge, this).getLength().getValue());
		auto descendants = traln.getDescendents(_nodeAge);
		content.values.push_back(
				traln.getBranch(descendants.first, this).getLength().getValue());
		content.values.push_back(
				traln.getBranch(descendants.second, this).getLength().getValue());
	}
	verifyContent(traln, content);
	return content;
}

void DivergenceTimes::printSample(std::ostream& fileHandle,
		const TreeAln &traln) const
{
	fileHandle << _nodeAge.getHeight();
}

void DivergenceTimes::printAllComponentNames(std::ostream &fileHandle,
		const TreeAln &traln) const
{
	fileHandle << "t{";

	bool isFirst = true;
	for (auto &p : _partitions)
	{
		fileHandle << (isFirst ? "" : ",") << p;
		isFirst = false;
	}
	fileHandle << "}(" << _nodeAge << ")";
}

void DivergenceTimes::verifyContent(const TreeAln &traln,
		const ParameterContent &content) const
{
	if (!content.nodeAges.size())
		return;

	/*
	 * content must have
	 * 1) new nodeAge
	 * 2) z_p, z_c1 and z_c2
	 */
	assert(content.nodeAges.size() == 1);
	assert(content.values.size() == 3);
}

log_double DivergenceTimes::getPriorValue(const TreeAln& traln) const
{
	// TODO should extract all branches and evaluate the prior ... not doing this here. ..
	return log_double::fromAbs(1.);
}

std::ostream& DivergenceTimes::printShort(std::ostream& out) const
{
	out << CategoryFuns::getShortName(_cat) << _idOfMyKind << "{";

	formatRange(out, _partitions);

	out << "}";
	return out;
}
