#include "DivTimeSlider.hpp"


const double DivTimeSlider::defaultWeight = 10.;  

// see DivTimeProposal for why we want a weight here . 

DivTimeSlider::DivTimeSlider(double weight) :
		AbstractProposal(Category::DIVERGENCE_TIMES, "divTimeSlider", weight, 1e-5,
				1e2, false), _savedContent()
{
}

double DivTimeSlider::getNewProposal(double height, double oldHeight, double youngHeight,
		Randomness &rand)
{
	/* find the range for the slider */
	assert(oldHeight > height && height > youngHeight);
	double upperRange = oldHeight - height;
	double lowerRange = height - youngHeight;
	double window = 2 * std::max(upperRange, lowerRange);
	double newValue = rand.drawFromSlidingWindow(height, window);
	if (newValue > oldHeight)
	{
		newValue = 2 * oldHeight - newValue;
	}
	else if (newValue < youngHeight)
	{
		newValue = 2 * youngHeight - newValue;
	}
	assert(oldHeight > newValue && newValue > youngHeight);

	return newValue;
}

NodeAge DivTimeSlider::getNodeAge(TreeAln & traln, nat primaryNode) const
{
	NodeAge result;
	for (int i : _secParamIds)
	{
		if (_allParams->at(i)->extractParameter(traln).nodeAges.size() > 0)
		{
			NodeAge testNode =
					_allParams->at(i)->extractParameter(traln).nodeAges[0];
			if (testNode.getPrimNode() == primaryNode)
			{
				result = testNode;
				break;
			}
		}
	}
	return result;
}

NodeAge DivTimeSlider::getNodeAge(TreeAln & traln, BranchPlain & bp) const
{
	NodeAge result = getNodeAge(traln, bp.getPrimNode());
	assert(bp == result);
	return result;
}

void DivTimeSlider::applyToState(TreeAln &traln, PriorBelief &prior,
		log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval)
{
	AbstractParameter * myDivtimesParameter;

	bool isRootNode;
	double oldHeight, newHeight;

	std::pair<BranchPlain, BranchPlain> descendants;
	std::pair<NodeAge, NodeAge> childrenNodeAge;
	NodeAge parentNodeAge;

	assert(_primParamIds.size() == 1);
	myDivtimesParameter = _allParams->at(_primParamIds[0]);

	_savedContent = myDivtimesParameter->extractParameter(traln);
	NodeAge savedAge = _savedContent.nodeAges[0];
	isRootNode = !(savedAge.getPrimNode() + savedAge.getSecNode());
	oldHeight = savedAge.getHeight();

#ifdef DIVTIME_DEBUG
	std::cout << "DIV-TIMES-DBG: Sliding node " << savedAge << std::endl;
#endif

	/* Get parent and children */
	if (isRootNode)
	{
		descendants.first = traln.getRootBranch();
		descendants.second = traln.getRootBranch().getInverted();
	}
	else
	{
		descendants = traln.getDescendents(savedAge);
		descendants.first = descendants.first.getInverted();
		descendants.second = descendants.second.getInverted();
	}

	if (traln.isTipNode(descendants.first.getPrimNode()))
	{
		childrenNodeAge.first = NodeAge(descendants.first, 0.0);
	}
	else
	{
		childrenNodeAge.first = getNodeAge(traln, descendants.first);
	}

	if (traln.isTipNode(descendants.second.getPrimNode()))
	{
		childrenNodeAge.second = NodeAge(descendants.second, 0.0);
	}
	else
	{
		childrenNodeAge.second = getNodeAge(traln, descendants.second);
	}

	if (!(isRootNode || traln.isRootBranch(savedAge)))
	{
		parentNodeAge = getNodeAge(traln, savedAge.getSecNode());
	}

	if (traln.isRootBranch(savedAge))
	{
		parentNodeAge =
				_allParams->at(_secParamIds[_secParamIds.size() - 1])->extractParameter(
						traln).nodeAges[0];
	}

	double parentAgeValue = (
			isRootNode ?
					(2 * savedAge.getHeight()
							- std::max(childrenNodeAge.first.getHeight(),
									childrenNodeAge.second.getHeight())) :
					parentNodeAge.getHeight());
	double childrenAgeValue = std::max(childrenNodeAge.first.getHeight(),
			childrenNodeAge.second.getHeight());

	/* Propose a new time for the node and a new NodeAge */
	newHeight = getNewProposal(oldHeight, parentAgeValue, childrenAgeValue,
			rand);
	auto newNodeAge = savedAge;
	newNodeAge.setHeight(newHeight);

	/* Add the new age to the content */
	auto content = ParameterContent();
	content.nodeAges.push_back(newNodeAge);

#ifdef DIVTIME_DEBUG
	std::cout << "DIV-TIMES-DBG: Propose " << newNodeAge.getHeight() << " for "
	<< savedAge << std::endl;
#endif

	auto rates = _allParams->at(_secParamIds[0])->extractParameter(traln).values;

	if (isRootNode)
	{
		/* root update */
		auto rootBranch = traln.getRootBranch();
		/* Root is a special case because the computation of the branch length
		 * depends on both root children branches.
		 *
		 * Both times and rates are pushed into the content object and
		 * the Parameter computes the actual branch length.
		 */
		double v = (newHeight - childrenNodeAge.first.getHeight())
				* rates[rootBranch.getPrimNode() - 1]
				+ (newHeight - childrenNodeAge.second.getHeight())
						* rates[rootBranch.getSecNode() - 1];
		double newLength = exp(-v / traln.getTrHandle().fracchange);
		content.values.push_back(0.0); // no parental branch
		content.values.push_back(newLength);
		content.values.push_back(newLength);
	}
	else
	{
		double v, newLength;
		/* add the old time to the content */
		double fracchange =
				(traln.getTrHandle().fracchange > 1e-7) ?
						traln.getTrHandle().fracchange : 1.0;
		if (traln.isRootBranch(savedAge))
		{
			double sisterAgeValue = 0.0;
			if (!traln.isTipNode(savedAge.getSecNode()))
			{
				sisterAgeValue =
						getNodeAge(traln, savedAge.getSecNode()).getHeight();
			}
			v = (parentAgeValue - newHeight) * rates[savedAge.getPrimNode() - 1]
					+ (parentAgeValue - sisterAgeValue)
							* rates[savedAge.getSecNode() - 1];
			newLength = exp(-v / fracchange);
			content.values.push_back(newLength);
		}
		else
		{
			v = (parentAgeValue - newHeight)
					* rates[savedAge.getPrimNode() - 1];
			newLength = exp(-v / fracchange);
			content.values.push_back(newLength);
		}

		if (childrenNodeAge.first.getPrimNode())
		{
			v = (newHeight - childrenNodeAge.first.getHeight())
					* rates[childrenNodeAge.first.getPrimNode() - 1];
			newLength = exp(-v / fracchange);
			content.values.push_back(newLength);
		}
		else
		{
			content.values.push_back(0.0);
		}

		if (childrenNodeAge.second.getPrimNode())
		{
			v = (newHeight - childrenNodeAge.second.getHeight())
					* rates[childrenNodeAge.second.getPrimNode() - 1];
			newLength = exp(-v / fracchange);
			content.values.push_back(newLength);
		}
		else
		{
			content.values.push_back(0.0);
		}
	}

	/* TODO-divtimes: sometimes the age grows too fast! */
	for (double value : content.values)
	{
		if (value
				&& (value < BoundsChecker::zMin || value > BoundsChecker::zMax))
		{
			return;
		}
	}
	myDivtimesParameter->applyParameter(traln, content);

	//double realMultiplier = newHeight / oldHeight;
	//hastings *= log_double::fromAbs(realMultiplier);
	//prior.addToRatio(param->getPrior()->getUpdatedValue(oldAge, newAge, param));
}

void DivTimeSlider::evaluateProposal(LikelihoodEvaluator &evaluator,
		TreeAln &traln, const BranchPlain &branchSuggestion)
{
	auto parts = _allParams->at(_primParamIds[0])->getPartitions();
	evaluator.evaluate(traln, traln.getRootBranch(), true);
}

void DivTimeSlider::resetState(TreeAln &traln)
{
#ifdef DIVTIME_DEBUG
	std::cout << "DIV-TIMES-DBG: Reset state" << std::endl;
#endif

	auto param = _allParams->at(_primParamIds[0]);
	param->applyParameter(traln, _savedContent);
}

void DivTimeSlider::autotune()
{
	/* TODO-divtimes. ? */
}

AbstractProposal* DivTimeSlider::clone() const
{
	return new DivTimeSlider(*this);
}

BranchPlain DivTimeSlider::determinePrimeBranch(const TreeAln &traln,
		Randomness& rand) const
{
	ParameterContent content =
			_allParams->at(_primParamIds[0])->extractParameter(traln);
	if (content.nodeAges.size() == 1)
	{
		return content.nodeAges[0];
	}
	else
	{
		return BranchPlain();
	}
}

std::vector<nat> DivTimeSlider::getInvalidatedNodes(const TreeAln &traln) const
{
	ParameterContent content =
			_allParams->at(_primParamIds[0])->extractParameter(traln);
	assert(content.nodeAges.size() == 1);
	return
	{	content.nodeAges[0].getPrimNode()};
}

std::pair<BranchPlain, BranchPlain> DivTimeSlider::prepareForSetExecution(
		TreeAln &traln, Randomness &rand)
{
	/* TODO-divtimes. Check this */
	return std::pair<BranchPlain, BranchPlain>(
			determinePrimeBranch(traln, rand), BranchPlain(0, 0));
}

void DivTimeSlider::writeToCheckpointCore(std::ostream &out) const
{
}

void DivTimeSlider::readFromCheckpointCore(std::istream &in)
{
}

