#include "TreeTraverser.hpp"

#define MAX_OPT_ITER 8 

#include "BranchBackup.hpp"


InsertionResult EvalPlain::getResult (LikelihoodEvaluator &eval, TreeAln &traln, const BranchPlain& insertBranch, const BranchPlain& prunedSubtree, const BranchPlain & draggedBranch, const std::vector<AbstractParameter*> &params) const 
{
  eval.evaluate(traln, prunedSubtree, false);
  auto result = InsertionResult (insertBranch, traln.getLikelihood()  , {}  ); 
  return result; 
}


InsertionResult EvalOptDragged::getResult (LikelihoodEvaluator &eval, TreeAln &traln, const BranchPlain& insertBranch, const BranchPlain& prunedSubtree, const BranchPlain & draggedBranch, const std::vector<AbstractParameter*> &params) const 
{
#ifdef PRINT_LIKESPR_INFO 
  eval.evaluate(traln, draggedBranch, false);
  // tout << "init-lnl="  << traln.getLikelihood() << std::endl; 
  auto initLnl = traln.getLikelihood() ; 
#else 
  eval.evaluate(traln, draggedBranch, false);
  // eval.evaluateSubtrees(traln, draggedBranch, false);
#endif

  auto blo = BranchLengthOptimizer(traln, draggedBranch, MAX_OPT_ITER, eval.getParallelSetup().getChainComm(), params);
  blo.optimizeBranches(traln);
  auto optParams = blo.getOptimizedParameters();

  auto backup = BranchBackup{};
  for(auto &optParam : optParams)
    {
      auto b = optParam.getOptimizedBranch(); 
      auto p = optParam.getParam();
      // tout << "optimized: " << b.toPlain() << " => " <<  MAX_SCI_PRECISION << b.getInterpretedLength(traln, p) <<  std::endl; 
      backup.extend(traln, p,b); 
    }

  eval.evaluate(traln, draggedBranch, false);
  auto lnl = traln.getLikelihood(); 

  backup.resetFromBackup(traln); 

#ifdef PRINT_LIKESPR_INFO
  tout << SOME_FIXED_PRECISION << "INSERT " << insertBranch << "\t" << SHOW(initLnl) << "\tnow=" << traln.getLikelihood()<< "\tdiff=" << traln.getLikelihood() / initLnl << MAX_SCI_PRECISION  << " with " << optParams[0].getOptimizedBranch().getInterpretedLength(traln,optParams[0].getParam()) <<" " << optParams[0].getOptimizedBranch().getLength() << std::endl ; 
#endif

  return InsertionResult{insertBranch, lnl , optParams} ; 
}


InsertionResult EvalOptThree::getResult (LikelihoodEvaluator &eval, TreeAln &traln, const BranchPlain& insertBranch, const BranchPlain& prunedSubtree, const BranchPlain & draggedBranch, const std::vector<AbstractParameter*> &params) const 
{
  // tout << SHOW(prunedSubtree) << SHOW(insertBranch) << std::endl; 
  auto branches = std::vector<BranchPlain>{ prunedSubtree, draggedBranch, traln.getThirdBranch(prunedSubtree, draggedBranch ) }; 
  
  bool converged = false; 
  nat ctr = 0; 
  
  auto allOparams = std::vector<OptimizedParameter>{}; 

  // backup 
  auto backup = BranchBackup{}; 
  for(auto &v :branches)
    for(auto param : params)
      backup.extend(traln, param,v);



#ifdef PRINT_LIKESPR_INFO 
  eval.evaluate(traln, draggedBranch, false);
  // tout << "init-lnl="  << traln.getLikelihood() << std::endl; 
  auto initLnl = traln.getLikelihood() ; 
#else 
  eval.evaluateSubtrees(traln, draggedBranch, false);
#endif

// #if 0 
//   eval.evaluate(traln, prunedSubtree, false);
//   tout << "init-lnl=" << traln.getLikelihood() << std::endl; 
// #endif

  // optimize all branches
  while( not converged &&  ctr < 10)
    {
      allOparams.clear(); 
      converged = true; 

      for(auto branch : branches)
	{ 
	  eval.evaluateSubtrees(traln, branch, false);

	  auto blo = BranchLengthOptimizer(traln, branch, MAX_OPT_ITER, eval.getParallelSetup().getChainComm(), params); 
	  blo.optimizeBranches(traln);

	  auto oparams = blo.getOptimizedParameters();

	  for(auto& oParam : oparams)
	    {
	      auto b = oParam.getOptimizedBranch(); 
	      auto p = oParam.getParam();

	      // tout << "OPT: " << b << "\t" << b.getInterpretedLength(traln, p ) << std::endl; 

	      auto oldLen = traln.getBranch(b, p).toMeanSubstitutions( p->getMeanSubstitutionRate()); 
	      auto newLen = b.toMeanSubstitutions(p->getMeanSubstitutionRate()); 
	      
	      converged &=  ( std::fabs((oldLen / newLen )  -1. ) < 1.e-1) ; 

	      traln.setBranch(b,p); 
	    }

	  allOparams.insert(end(allOparams), begin(oparams), end(oparams)); 
	}
      // tout << "---"<< std::endl; 

      ++ctr; 
    }

  eval.evaluate(traln, prunedSubtree, false);
  auto lnl = traln.getLikelihood(); 

#ifdef PRINT_LIKESPR_INFO
  tout << SOME_FIXED_PRECISION << "INSERT " << insertBranch << "\t" << SHOW(initLnl) << "\tnow=" << traln.getLikelihood()<< "\tdiff=" << traln.getLikelihood() / initLnl << MAX_SCI_PRECISION  << std::endl ; 
#endif

  backup.resetFromBackup(traln); 
  return InsertionResult(insertBranch, lnl,  allOparams);
}
