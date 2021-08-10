################################################################################
# Makefile for probcons
################################################################################

################################################################################
# 1) Choose C++ compiler.
################################################################################

CXX = g++

################################################################################
# 2) Set C++ flags.
#    a) DEBUG mode -- no optimizations, enable SafeVector checking, no inlining
#    b) PROFILE mode -- for gprof
#    c) RELEASE mode
################################################################################
BIG_INLINE_LIMIT = 20000

PROBCONS = ./probconsRNA
VIENNA   = ./vienna

# no -Wall option for warning
#CXXFLAGS = -O3 -funroll-loops -finline-limit=$(BIG_INLINE_LIMIT)
CXXFLAGS = -O3  # by katoh
OFLAGS = -DNDEBUG -DNumInsertStates=1 -DVERSION="2.0" $(CXXFLAGS) $(CFLAGS1) -std=c++98
LIBS = -L$(PROBCONS)  -L./
INCL = -I$(PROBCONS) -I$(VIENNA) -I./

################################################################################
# 3) Dependencies
################################################################################

TARGETS = mxscarna 

OBJS = Main.o McCaskill.o vienna/energy_param.o seq2scs.o Globaldp.o postProcessings.o AlifoldMEA.o

.PHONY : all
all : $(TARGETS) 

mxscarna : $(OBJS)
	$(CXX) $(LIBS) $(OFLAGS) -lm -o $@ $(OBJS)

rfold:  
	cd $(RFOLD); \
	make; \
	cd .. \
	cd ..
probcons:
	cd $(PROBCONS); \
	make; \
	cd .. 

#probcons : MultiSequence.h ProbabilisticModel.h ScoreType.h Sequence.h FileBuffer.h SparseMatrix.h EvolutionaryTree.h Defaults.h SafeVector.h Main.cc
#	$(CXX) -lm -o probcons $(OBJS)
#Main.cc 

Main.o :  $(PROBCONS)/SafeVector.h $(PROBCONS)/FileBuffer.h $(PROBCONS)/Sequence.h $(PROBCONS)/MultiSequence.h $(PROBCONS)/EvolutionaryTree.h scarna.hpp BPPMatrix.hpp StemCandidate.hpp Globaldp.hpp AlifoldMEA.h Main.cc
	$(CXX) $(INCL) $(OFLAGS) -c Main.cc -o Main.o
McCaskill.o: McCaskill.hpp $(VIENNA)/energy_param.hpp Util.hpp Beta.hpp ScoreType.hpp McCaskill.cpp
	$(CXX) $(INCL) $(OFLAGS) -c McCaskill.cpp -o McCaskill.o
$(VIENNA)/energy_param.o: $(VIENNA)/energy_param.hpp $(VIENNA)/energy_param.cpp
	$(CXX) $(INCL) $(OFLAGS) -c $(VIENNA)/energy_param.cpp -o $(VIENNA)/energy_param.o
seq2scs.o: $(PROBCONS)/SafeVector.h StemCandidate.hpp $(PROBCONS)/Sequence.h $(PROBCONS)/MultiSequence.h BPPMatrix.hpp nrutil.h seq2scs.cpp
	$(CXX) $(INCL) $(OFLAGS) -c seq2scs.cpp -o seq2scs.o
Globaldp.o: Globaldp.hpp nrutil.h Util.hpp Beta.hpp scarna.hpp StemCandidate.hpp $(PROBCONS)/MultiSequence.h $(PROBCONS)/Sequence.h BPPMatrix.hpp
	$(CXX) $(INCL) $(OFLAGS) -c Globaldp.cpp -o Globaldp.o
GlobalParameters.o: scarna.hpp
	$(CXX) $(INCL) $(OFLAGS) -c GlobalParameters.cpp -o GlobalParameters.o
postProcessings.o: StemCandidate.hpp scarna.hpp
	$(CXX) $(INCL) $(OFLAGS) -c postProcessings.cpp -o postProcessings.o
AlifoldMEA.o: nrutil.h Util.hpp Beta.hpp BPPMatrix.hpp $(PROBCONS)/MultiSequence.h $(PROBCONS)/Sequence.h $(PROBCONS)/SafeVector.h
	$(CXX) $(INCL) $(OFLAGS) -c AlifoldMEA.cpp -o AlifoldMEA.o

.PHONY : clean
clean:
	rm -f $(TARGETS) *.o *.h~ *.hpp~ *.cpp~ *.cc~ $(VIENNA)/*.o 
	$(MAKE) -C $(PROBCONS) clean
