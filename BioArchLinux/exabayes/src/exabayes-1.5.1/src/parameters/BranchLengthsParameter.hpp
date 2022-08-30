#ifndef _BRANCH_LENGTHS_PARAMETER
#define _BRANCH_LENGTHS_PARAMETER

#include "AbstractParameter.hpp"
#include "ComplexTuner.hpp"

#include "Category.hpp"

class BranchLengthsParameter : public AbstractParameter
{
public:
    // inherited from SERIALIZABLE
    // _________________________________________________________________________
    virtual void                                 deserialize(
        std::istream&in);
    // _________________________________________________________________________
    virtual void                                 serialize(
        std::ostream&out) const;
public:
    // INHERITED METHODS
    // _________________________________________________________________________
    virtual void                                 applyParameter(
        TreeAln&               traln,
        const ParameterContent&content);
    // _________________________________________________________________________
    virtual ParameterContent                     extractParameter(
        const TreeAln&traln)  const;
    // _________________________________________________________________________
    virtual AbstractParameter*                   clone() const
    {
        return new BranchLengthsParameter(*this);
    }
    // _________________________________________________________________________
    virtual void                                 printSample(
        std::ostream& fileHandle,
        const TreeAln&traln) const {}
    // _________________________________________________________________________
    virtual void                                 printAllComponentNames(
        std::ostream& fileHandle,
        const TreeAln&traln) const {}
    // _________________________________________________________________________
    virtual void                                 verifyContent(
        const TreeAln&         traln,
        const ParameterContent&content) const;
    // _________________________________________________________________________
    virtual bool                                 priorIsFitting(
        const AbstractPrior&prior,
        const TreeAln&      traln) const;
    // _________________________________________________________________________
    virtual ParamAttribute                       getAttributes() const
    {
        return {_convTuner, _nonConvTuner};
    }
    // _________________________________________________________________________
    virtual void                                 setAttributes(
        ParamAttribute attr)
    {
        _convTuner = attr._convTuner;
        _nonConvTuner = attr._nonConvTuner;
    }
    // _________________________________________________________________________
    virtual double                               getMeanSubstitutionRate()
    const;
    // _________________________________________________________________________
    virtual void                                 updateMeanSubstRate(
        const TreeAln& traln);
    // _________________________________________________________________________
    virtual void                                 setMeanSubstitutionRate(
        double fac){_fracChange = fac; }
    // _________________________________________________________________________
    virtual log_double                           getPriorValue(
        const TreeAln& traln) const
    {
        assert(0);
        return log_double::fromAbs(1);
    }
public:
    // METHODS
    // _________________________________________________________________________
    BranchLengthsParameter(
        nat             id,
        nat             idOfMyKind,
        std::vector<nat>partitions);
private:
    // ATTRIBUTES
    ComplexTuner _convTuner; // convergence parameter for distribution
                             // proposals
    ComplexTuner _nonConvTuner; // non-convergence parameter for the
                                // distribution proposals
    double       _fracChange;
};


#endif
