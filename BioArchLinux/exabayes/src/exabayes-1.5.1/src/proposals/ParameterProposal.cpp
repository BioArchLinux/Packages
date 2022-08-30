#include "ParameterProposal.hpp"
#include "AbstractPrior.hpp"
#include "BoundsChecker.hpp"

ParameterProposal::ParameterProposal(
    Category                         cat,
    std::string                      name,
    bool                             modBL,
    std::unique_ptr<AbstractProposer>_proposer,
    double                           p,
    double                           weight,
    double                           minTuning,
    double                           maxTuning)
    : AbstractProposal(cat, name, weight, minTuning, maxTuning, true)
    , modifiesBL(modBL)
    , parameter(p)
    , proposer(std::move(_proposer))
    , _savedContent{}
    , _savedBinaryContent{}
    , _oldFCs{}
{}


ParameterProposal::ParameterProposal(
    const ParameterProposal&rhs)
    : AbstractProposal(rhs)
    , modifiesBL(rhs.modifiesBL)
    , parameter(rhs.parameter)
    , proposer(std::unique_ptr<AbstractProposer>(rhs.proposer->clone()))
    , _savedContent{rhs._savedContent}
    , _savedBinaryContent{rhs._savedBinaryContent}
    , _oldFCs{rhs._oldFCs}
{}


void                    ParameterProposal::applyToState(
    TreeAln&             traln,
    PriorBelief&         prior,
    log_double&          hastings,
    Randomness&          rand,
    LikelihoodEvaluator& eval)
{
    auto blParams = getBranchLengthsParameterView();

    assert(_primParamIds.size() == 1);  // we only have one parameter to
                                        // integrate over
    // this parameter proposal works with any kind of parameters (rate
    // heterogeneity, freuqencies, revmat ... could also be extended to
    // work with AA)


    // extract the parameter (a handy std::vector<double> that for
    // instance contains all the frequencies)
    auto primParam = _allParams->at(_primParamIds[0]);
    auto content = primParam->extractParameter(traln);
    _savedContent = content;

    // for aa revmat + statefreqs, we need to be able to restore the exact
    // value
    _savedBinaryContent = primParam->extractParameterRaw(traln);

    // nasty, we have to correct for the fracchange
    // auto oldFCs = std::vector<double>{};
    _oldFCs.clear();
    auto newFCs = std::vector<double>{};

    for (auto&param : blParams)
        _oldFCs.push_back(param->getMeanSubstitutionRate());

    // we have a proposer object, that does the proposing (check out
    // ProposalFunctions.hpp) It should take care of the hastings as
    // well (to some degree)

    auto newValues = proposer->proposeValues(content.values, parameter, rand,
                                             hastings);

    // tout << "proposing "  << newValues << std::endl;
    assert(newValues.size() == content.values.size());

    // create a copy
    auto newContent = ParameterContent{};
    newContent.values = newValues;
    // use our parameter object to set the frequencies or revtmat rates
    // or what ever (for all partitions)
    primParam->applyParameter(traln, newContent);

    if (modifiesBL)
    {
        // update the frac changes
        for (auto&param : blParams)
            param->updateMeanSubstRate(traln);


        for (auto&param : blParams)
            newFCs.push_back(param->getMeanSubstitutionRate());

        auto ctr = 0;

        // account for implicit BL multiplier in prior
        for (auto&param: blParams)
        {
            prior.addToRatio(param->getPrior()->accountForMeanSubstChange(
                                 traln,
                                 param,
                                 _oldFCs.at(ctr),
                                 newFCs.at(ctr)));
            ++ctr;
        }

        // account for proposal density ratio
        for (nat i = 0; i < blParams.size(); ++i)
        {
            auto value = traln.getNumberOfBranches() * log(newFCs.at(i)
                                                           / _oldFCs.at(i));
            hastings *= log_double::fromLog(value);
        }
    }

    // a generic prior updates the prior rate
    auto pr = primParam->getPrior();
    prior.addToRatio(pr->getLogProb(newValues) / pr->getLogProb(
                         _savedContent.values));
}


void                    ParameterProposal::evaluateProposal(
    LikelihoodEvaluator&evaluator,
    TreeAln&            traln,
    const BranchPlain&  branchSuggestion)
{
    auto prts = _allParams->at(_primParamIds[0])->getPartitions();
#ifdef PRINT_EVAL_CHOICE
    tout << "EVAL-CHOICE "  << branchSuggestion << std::endl;
#endif

    evaluator.evaluatePartitionsWithRoot(traln, branchSuggestion, prts, true);
}


void                    ParameterProposal::resetState(
    TreeAln&traln)
{
    assert(_primParamIds.size() == 1);
    _allParams->at(_primParamIds[0])->applyParameter(traln, _savedContent);

    // for aa gtr revmat  + statefreq, we need to reset the exact binary value
    _allParams->at(
        _primParamIds[0])->applyParameterRaw(traln, _savedBinaryContent);

    // for a fixed bl parameter, we have to re-scale the branch lengths after
    // rejection again.
    // NOTICE: this is very inefficient
    if (modifiesBL)
    {
        for (auto&param : getBranchLengthsParameterView())
        {
            if (not param->getPrior()->needsIntegration())
            {
                auto prior = param->getPrior();

                for (auto&b : traln.extractBranches(param))
                {
                    auto content = prior->getInitialValue();
                    // b.setConvertedInternalLength(param, content.values[0]);
                    b.setLength(InternalBranchLength::fromAbsolute(
                                    content.values[0],
                                    param->getMeanSubstitutionRate()));

                    if (not BoundsChecker::checkBranch(b))
                        BoundsChecker::correctBranch(b);

                    traln.setBranch(b, param);
                }
            }
            else
                param->updateMeanSubstRate(traln);
        }
    }
}


void                                ParameterProposal::autotune()
{
    if (not proposer->isTune())
        return;

    double newParam = tuneParameter(_sctr.getBatch(),
                                    _sctr.getRatioInLastInterval(), parameter,
                                    not proposer->isTuneup());

    // tout << *this << "" << std::endl;
    // printShort (tout) ;
    // tout << "\t" << parameter << " -> "<< newParam << "\t" <<
    //  _sctr.getBatch() << std::endl;

    parameter = newParam;
    _sctr.nextBatch();
}


void                                ParameterProposal::readFromCheckpointCore(
    std::istream&in)
{
    parameter = cRead<decltype(parameter)>(in);
}


void                                ParameterProposal::writeToCheckpointCore(
    std::ostream&out)  const
{
    cWrite<decltype(parameter)>(out, parameter);
}


std::vector<nat>                    ParameterProposal::getInvalidatedNodes(
    const TreeAln&traln) const
{
    auto result = std::vector<nat>{};

    for (nat i = traln.getNumberOfTaxa() + 1; i < traln.getNumberOfNodes() + 1;
         ++i)
        result.push_back(i);

    return result;
}
