/*
 *
 * Beta.hpp
 *
 */
#ifndef BETA_HPP
#define BETA_HPP
using namespace std;

struct Beta {
  static const int NBASE = 4;// A, C, G, U
  static const int NBASEG = (NBASE + 1);     // A, C, G, U, Gap
  static const int NBASENG = (NBASE + 2);    // A, C, G, U, N, Gap
  static const int NPBASE = (NBASE * NBASE);
  static const int NPBASEG = (NBASEG * NBASEG);
  static const int NPBASENG = (NBASENG * NBASENG);
  static const int NCODE = 7;
  static const int BASEBIT = 2;
  static const int BASEMSK = ~(~0 << BASEBIT);
  enum BaseCode {
    A_CODE       = 0,
    C_CODE       = 1,
    G_CODE       = 2,
    U_CODE       = 3,
    N_CODE       = 4,
    GAP_CODE     = 5,
    INVALID_CODE = 16
  };
  enum PairCode {
    AA_CODE =  0, AC_CODE =  1, AG_CODE =  2, AU_CODE =  3,
    CA_CODE =  4, CC_CODE =  5, CG_CODE =  6, CU_CODE =  7,
    GA_CODE =  8, GC_CODE =  9, GG_CODE = 10, GU_CODE = 11,
    UA_CODE = 12, UC_CODE = 13, UG_CODE = 14, UU_CODE = 15,
    INVALID_PAIR_CODE = 16
  };
  enum ReducedPairCode {
    REDUCED_NPBASE = 7,
    REDUCED_AU_CODE = 0,
    REDUCED_CG_CODE = 1,
    REDUCED_GC_CODE = 2,
    REDUCED_GU_CODE = 3,
    REDUCED_UA_CODE = 4,
    REDUCED_UG_CODE = 5,
    REDUCED_MM_CODE = 6,
    REDUCED_INVALID_PAIR_CODE = 16
  };

  static const int N_CANONICAL = 6;
  static const int canonicalPairs[N_CANONICAL];
  static const int N_NON_CANONICAL = 10;
  static const int nonCanonicalPairs[N_NON_CANONICAL];
  static const int i2nt[NCODE];
  static const int nt2i[256];
  static const bool isCanonicalBasePair[NPBASE];

  static bool IsValidCode(const int& c) {return A_CODE <= c && c <= GAP_CODE;}
  static bool IsBaseCode(const int& c) {return (A_CODE <= c && c <= U_CODE);}
  static bool IsBasePairCode(const int& c){return (AA_CODE <= c && c <= UU_CODE);}
  static bool IsReducedBasePairCode(const int& c) {
    return (REDUCED_AU_CODE <= c && c <= REDUCED_MM_CODE);
  }
  static bool IsAmbiguousCode(const int& c) {return c == N_CODE;}
  static bool IsGapCode(const int& c) {return c == GAP_CODE;}
  static bool IsValidChar(const unsigned char& c) {
    return IsValidCode(nt2i[c]);
  }
  static bool IsBaseChar(const int& c) {return IsBaseCode(nt2i[c]);}
  static bool IsAmbiguousChar(const int& c) {return IsAmbiguousCode(nt2i[c]);}
  static bool IsGapChar(const int& c) {return IsGapCode(nt2i[c]);}
  static int nt2code(const int& nt) {
    if (0 <= nt && nt < 256) {
      return nt2i[nt];
    } else {
      return INVALID_CODE;
    }
  }
  static int getPairCode(const int& c, const int& c1) {
    return (IsBaseCode(c) && IsBaseCode(c1) 
	    ? ((c << BASEBIT) | c1)
	    : INVALID_PAIR_CODE);
  }
  static bool isValidPairCode(const int& pairCode) {
    return (0 <= pairCode && pairCode < NPBASE);
  }
  static void pair2Bases(const int& pairCode, int& c, int& c1) {
      //Assert(IsBasePairCode(pairCode));
    c1 = pairCode & BASEMSK;
    c = (pairCode >> BASEBIT) & BASEMSK;
  }
  static int getReducedPairCode(const int& c, const int& c1) {
    return reducePairCode(getPairCode(c, c1));
  }
  static int reducePairCode(const int& pairCode) {
    static const int table[NPBASE] = {
      REDUCED_MM_CODE, REDUCED_MM_CODE, REDUCED_MM_CODE, REDUCED_AU_CODE,
      REDUCED_MM_CODE, REDUCED_MM_CODE, REDUCED_CG_CODE, REDUCED_MM_CODE,
      REDUCED_MM_CODE, REDUCED_GC_CODE, REDUCED_MM_CODE, REDUCED_GU_CODE,
      REDUCED_UA_CODE, REDUCED_MM_CODE, REDUCED_UG_CODE, REDUCED_MM_CODE,
    };
    return (IsBasePairCode(pairCode)
	    ? table[pairCode]
	    : REDUCED_INVALID_PAIR_CODE);
  }
  static bool isValidReducedPairCode(const int& pairCode) {
    return (0 <= pairCode && pairCode < REDUCED_NPBASE);
  }
  static bool isCanonicalReducedPairCode(const int& pairCode) {
    return (REDUCED_AU_CODE <= pairCode
	    && pairCode <= REDUCED_UG_CODE);
  }
  static int flipReducedPairCode(const int& reducedPairCode) {
    static const int table[REDUCED_NPBASE + 1] = {
      REDUCED_UA_CODE,
      REDUCED_GC_CODE,
      REDUCED_CG_CODE,
      REDUCED_UG_CODE,
      REDUCED_AU_CODE,
      REDUCED_GU_CODE,
      REDUCED_MM_CODE,
      REDUCED_INVALID_PAIR_CODE
    };
    return (IsReducedBasePairCode(reducedPairCode)
	    ? table[reducedPairCode] : table[REDUCED_NPBASE]);
  }
  static void seq2i(char* s, const char* t, int len) {
    const char* const s_end = s + len;
    while (s < s_end) *s++ = nt2i[(unsigned) (*t++)];
  }
  static void i2seq(char* s, const char* t, int len) {
    const char* const s_end = s + len;
    while (s < s_end) *s++ = i2nt[(unsigned) (*t++)];
  }
  static void i2seq(ostream& fo, const char* t, int len) {
    const char* const t_end = t + len;
    while (t < t_end) fo << (char) i2nt[(unsigned) (*t++)];
  }
  static char* wd2str(unsigned int wdSize, unsigned int wd) {
    const unsigned int MAX_WD_SIZE = (sizeof(unsigned int) * 8 / BASEBIT);
    static char buf[MAX_WD_SIZE + 1] = {};
    //Assert(wdSize <= MAX_WD_SIZE);

    char* s = buf + wdSize;
    *s = '\0';
    do {
      *(--s) = Beta::i2nt[wd & BASEMSK];
      wd >>= BASEBIT;
    } while (s > buf);
    return buf;
  }
  static void printWd(ostream& fo, unsigned int wdSize, unsigned int wd) {
    fo << wd2str(wdSize, wd);
  }
  static const char* code2str(const int& code) {
    static const char table[NBASENG+1][2] = {
      "A", "C", "G", "U", "N", ".", "?"
    };
    return ((A_CODE <= code && code <= GAP_CODE)
	    ? table[code] : table[NBASENG]);
  }
  static const char* pairCode2str(const int& pairCode) {
    static const char table[NPBASE+1][3] = {
      "AA", "AC", "AG", "AU",
      "CA", "CC", "CG", "CU",
      "GA", "GC", "GG", "GU",
      "UA", "UC", "UG", "UU",
      "??"
    };
    return (IsBasePairCode(pairCode) ? table[pairCode] : table[NPBASE]);
  }
  static const char* reducedPairCode2str(const int& reducedPairCode) {
    static const char table[REDUCED_NPBASE+1][3] = {
      "AU", "CG", "GC", "GU", "UA", "UG", "MM", "??"
    };
    return (IsReducedBasePairCode(reducedPairCode)
	    ? table[reducedPairCode] : table[REDUCED_NPBASE]);
  }
  static char nt2Code(const char& c){
      if (!IsValidChar(c)) { cerr << "character " <<  c << " is not valid"; }

    return (char) nt2i[(int) c];
  }
  static char code2char(const int& c) {
     static const char table[NBASENG+1] = {
      'A', 'C', 'G', 'U', 'N', '.', '?'
    };
    return ((A_CODE <= c && c <= GAP_CODE)
	    ? table[(int) c] : table[NBASENG]);
  }
    /*
  static string generateRandomRNA(const int& len) {
    static const char nt[5] = "ACGU";
    string rna(len, '\0');
    for (int i = 0; i < len; i++) {
      rna[i] = nt[Rand(4)];
    }
    return rna;
  }
    */
};

#endif
