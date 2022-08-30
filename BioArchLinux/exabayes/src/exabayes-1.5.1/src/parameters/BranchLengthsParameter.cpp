#include "BranchLengthsParameter.hpp"
#include "BoundsChecker.hpp"
#include "Arithmetics.hpp"

BranchLengthsParameter::BranchLengthsParameter(
    nat             id,
    nat             idOfMyKind,
    std::vector<nat>partitions)
    : AbstractParameter(
        Category::BRANCH_LENGTHS,
        id,
        idOfMyKind,
        partitions,
        0)
    , _convTuner{1.61, 0.01, 10, 0.1, false}
    , _nonConvTuner{2., 0.1, 10, 0.1, false}
    , _fracChange{1}
{
    _printToParamFile = false;
}


void                    BranchLengthsParameter::applyParameter(
    TreeAln&                traln,
    const ParameterContent& content)
{
    for (auto&b : content.branchLengths)
        traln.setBranch(b, this);
}

auto                    BranchLengthsParameter::extractParameter(
    const TreeAln&traln)  const
    ->ParameterContent
{
    auto result = ParameterContent{};
    result.branchLengths = traln.extractBranches(this);
    return result;
}


void                    BranchLengthsParameter::verifyContent(
    const TreeAln&         traln,
    const ParameterContent&content) const
{
    for (auto&bl : content.branchLengths)
    {
        if (not BoundsChecker::checkBranch(bl))
        {
            tout << "observed invalid branch " << bl
                 << " that must not be there." << std::endl;
            assert(0);
        }
    }
}


bool                    BranchLengthsParameter::priorIsFitting(
    const AbstractPrior&prior,
    const TreeAln&      traln) const
{
    auto content = prior.getInitialValue();
    return content.values.size() < 2;
}


void                    BranchLengthsParameter::deserialize(
    std::istream&in)
{
    _nonConvTuner.deserialize(in);
    _convTuner.deserialize(in);
    _fracChange = cRead<double>(in);
}

void                    BranchLengthsParameter::serialize(
    std::ostream&out) const
{
    _nonConvTuner.serialize(out);
    _convTuner.serialize(out);
    cWrite<double>(out, _fracChange);
}


auto                    BranchLengthsParameter::getMeanSubstitutionRate() const
    ->double
{
    // tout << MAX_SCI_PRECISION << SHOW(_fracChange) << std::endl;
    return _fracChange;
}


void                    BranchLengthsParameter::updateMeanSubstRate(
    const TreeAln& traln)
{
    // for proposal sets, it may be worth it, to only the value... with
    // the fracchange for the respective partitions

    // summing in ascending order in order to keep
    auto fcs = std::vector<double>{};

    for (auto&p :_partitions)
    {
        const auto& partition =  traln.getPartition(p);
        fcs.push_back(partition.getFracChange()
                      * partition.getPartitionContribution());
        assert(partition.getPartitionContribution() > 0);
        assert(partition.getFracChange() > 0);
    }

    std::sort(begin(fcs), end(fcs));

    auto result = Arithmetics::getKahanSum(fcs);

    _fracChange = result;
}
