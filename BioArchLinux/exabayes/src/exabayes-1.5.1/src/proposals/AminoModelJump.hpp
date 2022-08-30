#ifndef _AAMODEL_JUMP_H
#define _AAMODEL_JUMP_H

#include "AbstractProposal.hpp"
#include "ProtModel.hpp"

#include <vector>

///////////////////////////////////////////////////////////////////////////////
//                             AMINO MODEL JUMP                              //
///////////////////////////////////////////////////////////////////////////////
class AminoModelJump : public AbstractProposal
{
public:
    // _________________________________________________________________________
    AminoModelJump();
    // _________________________________________________________________________
    virtual BranchPlain                               determinePrimeBranch(
        const TreeAln& traln,
        Randomness&    rand) const {return BranchPlain(0, 0); }
    // _________________________________________________________________________
    virtual void                                      readFromCheckpointCore(
        std::istream&in){}                                    // disabled
    // _________________________________________________________________________
    virtual void                                      writeToCheckpointCore(
        std::ostream&out) const {}                                 // disabled
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
    virtual AbstractProposal*                         clone()
    const;
    // _________________________________________________________________________
    virtual std::vector<nat>                          getInvalidatedNodes(
        const TreeAln& traln) const;
    // _________________________________________________________________________
    virtual std::pair<BranchPlain,
                      BranchPlain>                    prepareForSetExecution(
        TreeAln&   traln,
        Randomness&rand)
    {
        return std::make_pair(BranchPlain(0, 0), BranchPlain(0, 0));
    }
private:
    // _________________________________________________________________________
    ProtModel                                         drawProposal(
        Randomness&rand) const;
private:
    ProtModel _savedMod;
};


#endif
