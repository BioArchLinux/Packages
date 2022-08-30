#include "RateDirichletProposer.hpp"
#include "BoundsChecker.hpp"
#include "AminoAcidAlphabet.hpp"

RateDirichletProposer::RateDirichletProposer(double minValI, double maxValI)
  : AbstractProposer{true,false, minValI, maxValI}
  , _rateHelper{}
{
}


// RateDirichletProposer::RateDirichletProposer(const RateDirichletProposer& rhs)
//   : AbstractProposer(rhs)
// {
  
// }

// OUTPUT: 
// allNewRates: all rates summing up to 1, the newly proposed, but corrected values
// newRates : all new rates, summing up to 1 and after corection 
std::tuple<std::vector<double>, std::vector<double> > RateDirichletProposer::correctValues(nat whichFreq, nat numFreq, std::vector<double> newRates, std::vector<double> allNewValues)
{
  _rateHelper.insertRates(whichFreq ,numFreq, allNewValues, newRates); 

  _rateHelper.convertRelativeToLast(allNewValues);
  BoundsChecker::correctRevMat(allNewValues);
  _rateHelper.convertToSum1(allNewValues);

  newRates = _rateHelper.extractSomeRates(whichFreq, numFreq, allNewValues);

  _rateHelper.convertToSum1(newRates);  
  return std::make_tuple(newRates, allNewValues);
}



std::vector<double> RateDirichletProposer::proposeValues(std::vector<double> allOldValues, double parameter, Randomness &rand, log_double &hastings) 
{
  // tout << std::setprecision(6) ; 
  // tout << "\n" << std::endl; 

  static const nat numFreq =  20 ; 

  // this is hard-coded for amino acids currently 
  assert(allOldValues.size() == ( numFreq * numFreq - numFreq )  / 2 ); 

  // choose a reference rate 
  auto whichFreq = rand.drawIntegerOpen(numFreq);

  // tout << "FREQ: " << whichFreq << std::endl; ; 
  // tout << "allRates: "  << allOldValues << std::endl; 

  // extract subst rates for one freq
  auto ratesForFreq = _rateHelper.extractSomeRates(whichFreq, numFreq, allOldValues);
  assert(ratesForFreq.size()  == numFreq -1 ); 
  
  // tout << "theseRates: " << ratesForFreq << std::endl; 

  auto oldSumOfPart = _rateHelper.convertToSum1(ratesForFreq);
  auto alphasOld = _rateHelper.getScaledValues(ratesForFreq, parameter); 

  // draw new
  auto newRatesForFreq = rand.drawRandDirichlet(alphasOld);
  
  // scale the rates s.t. they fit back into the original matrix
  _rateHelper.convertToGivenSum(newRatesForFreq, oldSumOfPart);

  // tout << "proposed: "<< newRatesForFreq << std::endl; 
  
  // create the full set of rates we could propose 
  auto allNewValues = std::vector<double> {}; 
  std::tie(newRatesForFreq, allNewValues) = correctValues(whichFreq, numFreq, newRatesForFreq, allOldValues);

  // tout << "correcPart: "  << newRatesForFreq << std::endl; 
  // tout << "allNewCorr: "  << allNewValues << std::endl;  

  auto forP = Density::lnDirichlet(newRatesForFreq,alphasOld); 
  
  // scale new 
  auto alphasNew =  _rateHelper.getScaledValues(newRatesForFreq, parameter); 
  auto backP = Density::lnDirichlet(ratesForFreq, alphasNew); 

  // AbstractProposal::updateHastingsLog(hastings, backP - forP, "dirichletPartial");
  hastings *= backP / forP;

  // tout << "hastings=" << backP - forP << std::endl; 

  return  allNewValues; 
}


AbstractProposer* RateDirichletProposer::clone() const 
{
  return new RateDirichletProposer(*this);
}
