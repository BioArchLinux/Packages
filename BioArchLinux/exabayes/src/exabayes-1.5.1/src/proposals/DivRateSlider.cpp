#include "DivRateSlider.hpp"

DivRateSlider::DivRateSlider() :
		AbstractProposal(Category::DIVERGENCE_TIMES, "divRateSlider", 10, 1e-5,
				1e2, false), _savedContent
		{ }
{
}

DivRateSlider::~DivRateSlider()
{
}

double DivRateSlider::getNewProposal(double oldRate, Randomness &rand)
{
	/* the absolute value here ensures a positive value */
	return std::abs(rand.drawFromSlidingWindow(oldRate, 0.1));
}

void DivRateSlider::applyToState(TreeAln &traln, PriorBelief &prior,
		log_double &hastings, Randomness &rand, LikelihoodEvaluator& eval)
{
	nat proposalMode = PROP_MODE_RATES;
	nat numberOfRates, maxCategory;

	auto primParam = _allParams->at(_primParamIds[0]);
	_savedContent = primParam->extractParameter(traln);
	auto content = _savedContent;
	numberOfRates = _savedContent.rateAssignments.size();

	maxCategory = 0;
	for (nat i = 0; i < numberOfRates; i++)
	{
		if (_savedContent.rateAssignments[i] > maxCategory)
		{
			maxCategory = _savedContent.rateAssignments[i];
		}
	}

	if (proposalMode == PROP_MODE_CATEGORIES)
	{
		nat assignOperation = ASSING_OP_MOVE;
		switch (assignOperation)
		{
		case ASSING_OP_MERGE:
		{

			break;
		}
		case ASSING_OP_SPLIT:
		{

			break;
		}
		case ASSING_OP_MOVE:
		{

			break;
		}
		default:
			assert(0);
		}

		/* merge cate */
	}
	else if (proposalMode == PROP_MODE_RATES)
	{
		/* Select category at random */
		nat categoryToChange = rand.drawIntegerClosed(maxCategory);
		/* Propose a new value */
		double newRate = getNewProposal(_savedContent.values[categoryToChange],
				rand);

		/* TODO-divtimes update root children if necessary! */
		auto rootBranch = traln.getRootBranch();
		if (_savedContent.rateAssignments[rootBranch.getPrimNode() - 1]
				== categoryToChange
				|| _savedContent.rateAssignments[rootBranch.getSecNode() - 1]
						== categoryToChange)
		{
			std::pair<double, double> rootRates =
					{
							(_savedContent.rateAssignments[rootBranch.getPrimNode()
									- 1] == categoryToChange) ?
									newRate :
									content.values[_savedContent.rateAssignments[rootBranch.getPrimNode()
											- 1]],
							(_savedContent.rateAssignments[rootBranch.getSecNode()
									- 1] == categoryToChange) ?
									newRate :
									content.values[_savedContent.rateAssignments[rootBranch.getSecNode()
											- 1]] };
			double parentAgeValue = _allParams->at(
					_secParamIds[_secParamIds.size() - 1])->extractParameter(
					traln).nodeAges[0].getHeight();
			double child1AgeValue =
					traln.isTipNode(rootBranch.getPrimNode()) ?
							0.0 :
							(_allParams->at(
									_secParamIds[rootBranch.getPrimNode()
											- traln.getNumberOfTaxa() - 1])->extractParameter(
									traln).nodeAges[0].getHeight());
			double child2AgeValue =
					traln.isTipNode(rootBranch.getSecNode()) ?
							0.0 :
							(_allParams->at(
									_secParamIds[rootBranch.getSecNode()
											- traln.getNumberOfTaxa() - 1])->extractParameter(
									traln).nodeAges[0].getHeight());
			double v = (parentAgeValue - child1AgeValue) * rootRates.first
					+ (parentAgeValue - child2AgeValue) * rootRates.second;
			double branchLength = exp(-v / traln.getTrHandle().fracchange);

			/* TODO-divtimes: discard if branch length is out of bounds! */
			if (branchLength
					&& (branchLength < BoundsChecker::zMin
							|| branchLength > BoundsChecker::zMax))
			{
#ifdef DIVTIME_DEBUG
				std::cout << "DIV-RATES-DBG: Propose new rate for root out of bounds..."
#endif
				return;
			}

#ifdef DIVTIME_DEBUG
			std::cout << "DIV-RATES-DBG: Propose new rate for root C1: "
			<< content.values[_savedContent.rateAssignments[rootBranch.getPrimNode()
			- 1]] << " to " << newRate << std::endl;
			std::cout << "DIV-RATES-DBG:                           C2: "
			<< content.values[_savedContent.rateAssignments[rootBranch.getSecNode()
			- 1]] << " to " << newRate << std::endl;
			std::cout << "DIV-RATES-DBG: - - Branch changes from "
			<< content.branchLengths[rootBranch.getPrimNode() - 1].getLength().getValue()
			<< " to " << branchLength << std::endl;
#endif

			content.branchLengths[rootBranch.getPrimNode() - 1].setLength(
					InternalBranchLength(branchLength));
			content.branchLengths[rootBranch.getSecNode() - 1].setLength(
					InternalBranchLength(branchLength));
		}

		for (nat nodeToChange = 0; nodeToChange < numberOfRates; nodeToChange++)
		{
			if (_savedContent.rateAssignments[nodeToChange] == categoryToChange)
			{
				if (traln.isRootChild(nodeToChange + 1))
				{
					/* TODO-divtimes SKIP ROOT NODES */
					continue;
				}
				else
				{
					/* Update branch length */
					double branchLength =
							content.branchLengths[nodeToChange].getLength().getValue();
					branchLength = exp(
							log(branchLength) * newRate
									/ _savedContent.values[categoryToChange]);

					/* TODO-divtimes: discard if branch length is out of bounds! */
					if (branchLength
							&& (branchLength < BoundsChecker::zMin
									|| branchLength > BoundsChecker::zMax))
					{
#ifdef DIVTIME_DEBUG
						std::cout << "DIV-RATES-DBG: Propose new rate out of bounds..."
#endif
						return;
					}

					content.branchLengths[nodeToChange].setLength(
							InternalBranchLength(branchLength));
				}
			}
		}
#ifdef DIVTIME_DEBUG
		std::cout << "DIV-RATES-DBG: Propose new rate for " << nodeToChange << " --> " << _savedContent.values[nodeToChange] << " to " << newRate << std::endl;
		std::cout << "DIV-RATES-DBG: - - Branch changes from " << content.branchLengths[nodeToChange].getLength().getValue() << " to " << branchLength << std::endl;
#endif
		content.values[categoryToChange] = newRate;
	}
	else
	{
		assert(0);
	}

	primParam->applyParameter(traln, content);
}

void DivRateSlider::evaluateProposal(LikelihoodEvaluator &evaluator,
		TreeAln &traln, const BranchPlain &branchSuggestion)
{
	auto prts = _allParams->at(_primParamIds[0])->getPartitions();
	evaluator.evaluatePartitionsWithRoot(traln, branchSuggestion, prts, true);
}

void DivRateSlider::resetState(TreeAln &traln)
{
#ifdef DIVTIME_DEBUG
	std::cout << "DIV-RATES-DBG: Reset rate proposal " << std::endl;
#endif
	_allParams->at(_primParamIds[0])->applyParameter(traln, _savedContent);
}

void DivRateSlider::autotune()
{
}

AbstractProposal* DivRateSlider::clone() const
{
	return new DivRateSlider(*this);
}

BranchPlain DivRateSlider::determinePrimeBranch(const TreeAln &traln,
		Randomness& rand) const
{
	return BranchPlain();
}

std::vector<nat> DivRateSlider::getInvalidatedNodes(const TreeAln &traln) const
{
	return
	{};
}

std::pair<BranchPlain, BranchPlain> DivRateSlider::prepareForSetExecution(
		TreeAln &traln, Randomness &rand)
{
	return std::make_pair(BranchPlain(0, 0), BranchPlain(0, 0));
}

void DivRateSlider::writeToCheckpointCore(std::ostream &out) const
{
}

void DivRateSlider::readFromCheckpointCore(std::istream &in)
{
}

