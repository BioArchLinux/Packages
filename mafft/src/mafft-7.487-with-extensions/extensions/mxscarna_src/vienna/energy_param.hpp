/* 
    Current free energy parameters are summarized in:

    D.H.Mathews, J. Sabina, M. ZUker, D.H. Turner
    "Expanded sequence dependence of thermodynamic parameters improves
    prediction of RNA secondary structure"
    JMB, 288, pp 911-940, 1999

    Enthalpies taken from:
    
    A. Walter, D Turner, J Kim, M Lyttle, P M"uller, D Mathews, M Zuker
    "Coaxial stckaing of helices enhances binding of oligoribonucleotides.."
    PNAS, 91, pp 9218-9222, 1994
    
    D.H. Turner, N. Sugimoto, and S.M. Freier.
    "RNA Structure Prediction",
    Ann. Rev. Biophys. Biophys. Chem. 17, 167-192, 1988.

    John A.Jaeger, Douglas H.Turner, and Michael Zuker.
    "Improved predictions of secondary structures for RNA",
    PNAS, 86, 7706-7710, October 1989.
    
    L. He, R. Kierzek, J. SantaLucia, A.E. Walter, D.H. Turner
    "Nearest-Neughbor Parameters for GU Mismatches...."
    Biochemistry 1991, 30 11124-11132

    A.E. Peritz, R. Kierzek, N, Sugimoto, D.H. Turner
    "Thermodynamic Study of Internal Loops in Oligoribonucleotides..."
    Biochemistry 1991, 30, 6428--6435

    
*/

#ifndef ENERGY_PARAM_H
#define ENERGY_PARAM_H

#include <iostream>
#include <cstdlib> // by katoh

using namespace std;
namespace MXSCARNA {
class energy_param {

    static const int    INF;
    static const int    NST;
    static const int    DEF;

    static const double lxc37; /* parameter for logarithmic loop
                             energy extrapolation            */
    static const int stack37[8][8];
    static const int enthalpies[8][8];
    static const int oldhairpin37[31];
    static const int hairpin37[31];
    static const int oldbulge37[31];
    static const int bulge37[31];
    static const int oldinternal_loop37[31];
    static const int internal_loop37[31];
    static const int mismatchI37[8][5][5];
    static const int mismatchH37[8][5][5];
    static const int mism_H[8][5][5];
    static const int dangle5_37[8][5];
    static const int dangle3_37[8][5];
    static const int dangle3_H[8][5];
    static const int dangle5_H[8][5];
    static const int ML_BASE37;
    static const int ML_closing37;
    static const int ML_intern37;
    static const int MAX_NINIO;
    static const int F_ninio37[5];
    static const char Tetraloops[1400];
    static const char Tetrastrings[30][7];
    static const int TETRA_ENERGY37[200];
    static const int TETRA_ENTH37;
    static const int TerminalAU;
    static const int DuplexInit;

    static const int int11_37[8][8][5][5];
    static const int int11_H[8][8][5][5];
    static const int int21_37[8][8][5][5][5];
    static const int int21_H[8][8][5][5][5];
    static const int int22_37[8][8][5][5][5][5];
    static const int int22_H[8][8][5][5][5][5];

    int convertPairType(int type);
    int convertBaseType(int type);
    int convertLeftType(int type);
    int convertRightType(int type);
 
 public:
    int getMaxNinio();
    int getNinio(int i);
    int getDangle5(int i, int j);
    int getDangle3(int i, int j);
    int getHairpin(int i);
    int getInternalLoop(int i);
    int getBulge(int i);
    const char *getTetraLoop(int i);
    int getTetraLoopEnergy(int i);
    int getStack(int i, int j);
    int getTstackH(int i, int j);
    int getTstackI(int i, int j);
    int getInt11(int i, int j, int k);
    int getInt21(int i, int j, int k, int l);
    int getInt22(int i, int j, int k, int l);
    int getMLintern();
    int getMLBase();
    int getMLclosing();
    int getTerminalAU();
};
}
#endif
