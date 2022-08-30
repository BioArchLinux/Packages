#ifndef _PARAMETER_PROPOSALS
#define _PARAMETER_PROPOSALS

#include "TreeAln.hpp"
#include "AbstractProposal.hpp"
#include "AbstractProposer.hpp"
#include "ParameterProposal.hpp"

#include <memory>

///////////////////////////////////////////////////////////////////////////////
//                             PARAMETER PROPOSAL                            //
///////////////////////////////////////////////////////////////////////////////
class ParameterProposal : public AbstractProposal
{
public:
    // _________________________________________________________________________
    ParameterProposal(
        Category                         cat,
        std::string                      _name,
        bool                             modifiesBL,
        std::unique_ptr<AbstractProposer>_proposer,
        double                           parameter,
        double                           weight,
        double                           minTuning,
        double                           maxTuning);
    // _________________________________________________________________________
    ParameterProposal(
        const ParameterProposal&prop);
    // _________________________________________________________________________
    virtual ~ParameterProposal(){}
    // _________________________________________________________________________
    virtual void                                      applyToState(
        TreeAln&             traln,
        PriorBelief&         prior,
        log_double&          hastings,
        Randomness&          rand,
        LikelihoodEvaluator& eval);
    // _________________________________________________________________________
    virtual void                                      evaluateProposal(
        LikelihoodEvaluator&evaluator,
        TreeAln&            traln,
        const BranchPlain&  branchSuggestion);
    // _________________________________________________________________________
    virtual void                                      resetState(
        TreeAln&traln);
    // _________________________________________________________________________
    virtual void                                      autotune();
    // _________________________________________________________________________
    virtual BranchPlain                               determinePrimeBranch(
        const TreeAln&traln,
        Randomness&   rand) const {return BranchPlain(); }
    // _________________________________________________________________________
    virtual AbstractProposal*                         clone()
    const {return new ParameterProposal(*this);   }
    // _________________________________________________________________________
    virtual std::vector<nat>                          getInvalidatedNodes(
        const TreeAln&traln) const;
    // _________________________________________________________________________
    virtual std::pair<BranchPlain,
                      BranchPlain>                    prepareForSetExecution(
        TreeAln&   traln,
        Randomness&rand)
    {return std::make_pair(BranchPlain(0, 0), BranchPlain(0, 0)); }
    // _________________________________________________________________________
    virtual void                                      readFromCheckpointCore(
        std::istream&in);
    // _________________________________________________________________________
    virtual void                                      writeToCheckpointCore(
        std::ostream&out) const;
private:
    bool                              modifiesBL;
    double                            parameter;
    std::unique_ptr<AbstractProposer> proposer;

    ParameterContent                  _savedContent;
    ParameterContent                  _savedBinaryContent;
    std::vector<double>               _oldFCs;
};


#endif
