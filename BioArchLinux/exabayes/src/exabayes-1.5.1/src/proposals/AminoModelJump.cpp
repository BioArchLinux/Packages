#include "AminoModelJump.hpp"
#include "BoundsChecker.hpp"
#include "Category.hpp"
#include "AbstractPrior.hpp"


AminoModelJump::AminoModelJump()
    : AbstractProposal(Category::AA_MODEL, "aaMat", 1., 0, 0, true)
    , _savedMod{}
{}

ProtModel                    AminoModelJump::drawProposal(
    Randomness&rand) const
{
    auto primVar  = getPrimaryParameterView();
    auto newModel = _savedMod;

    while (newModel == _savedMod)
    {
        auto content = primVar[0]->getPrior()->drawFromPrior(rand);
        newModel = content.protModel[0];
    }

    return newModel;
}

void                         AminoModelJump::applyToState(
    TreeAln&            traln,
    PriorBelief&        prior,
    log_double&         hastings,
    Randomness&         rand,
    LikelihoodEvaluator&eval)
{
    // save the old fracchanges
    auto                       blParams = getBranchLengthsParameterView();

    auto                       primVar = getPrimaryParameterView();

    assert(primVar.size() == 1);
    const auto&                partitions = primVar[0]->getPartitions();

    std::vector<double>        oldFCs;
    oldFCs.reserve(partitions.size());

    for (auto&blParam : blParams)
        oldFCs.push_back(blParam->getMeanSubstitutionRate());

    _savedMod = traln.getProteinModel(partitions[0]);

    auto                       newModel = drawProposal(rand);

    std::vector<double>        newFCs;
    newFCs.reserve(partitions.size());

    // apply new model
    for (auto p : partitions)
        traln.setProteinModel(p, newModel);

    // update the fracchanges
    for (auto blParam: blParams)
        blParam->updateMeanSubstRate(traln);

    for (auto blParam : blParams)
        newFCs.push_back(blParam->getMeanSubstitutionRate());

    // account for implicit BL multiplier in prior
    for (nat i = 0; i < blParams.size(); ++i)
    {
        prior.addToRatio(blParams[i]->getPrior()->accountForMeanSubstChange(
                             traln,
                             blParams[i],
                             oldFCs.at(i),
                             newFCs.at(i)));
    }

    // account for implicit BL multiplier in Hastings
    for (nat i = 0; i < blParams.size(); ++i)
    {
        auto value =
            traln.getNumberOfBranches()
            * log(newFCs.at(i) / oldFCs.at(i));

        hastings *= log_double::fromLog(value);
    }
}


void                                AminoModelJump::evaluateProposal(
    LikelihoodEvaluator&evaluator,
    TreeAln&            traln,
    const BranchPlain&  branchSuggestion)
{
    evaluator.evaluate(traln, traln.getAnyBranch(), true); // TODO evaluate one
                                                           // partition
}


void                                AminoModelJump::resetState(
    TreeAln&traln)
{
    auto primVar = getPrimaryParameterView();
    assert(primVar.size() == 1);
    auto partitions = primVar[0]->getPartitions();

    for (auto p: partitions)
        traln.setProteinModel(p, _savedMod);

    // necessary, if we have fixed branch lengths
    for (auto&param : getBranchLengthsParameterView())
    {
        if (not param->getPrior()->needsIntegration())
        {
            for (auto&b : traln.extractBranches(param))
            {
                auto content = param->getPrior()->getInitialValue();

                b.setLength(InternalBranchLength::fromAbsolute(
                                content.values[0],
                                param->getMeanSubstitutionRate()));

                if (not BoundsChecker::checkBranch(b))
                    BoundsChecker::correctBranch(b);

                traln.setBranch(b, param);
            }
        }
        else
        {
            // TODO inefficient =/ but cannot use the oldFCs. That
            // would cause trouble in proposal sets
            param->updateMeanSubstRate(traln);
        }
    }
}


void                                AminoModelJump::autotune()
{
    // do nothing
}


AbstractProposal*                   AminoModelJump::clone() const
{
    return new AminoModelJump(*this);
}


std::vector<nat>                    AminoModelJump::getInvalidatedNodes(
    const TreeAln& traln) const
{
    auto result = std::vector<nat>{};

    for (nat i = traln.getNumberOfTaxa() + 1;
         i < traln.getNumberOfNodes() + 1;
         ++i)
    {
        //
        result.push_back(i);
    }


    return result;
}
