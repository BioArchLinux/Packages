################################################################
# GENENRAL 

AUTOMAKE_OPTIONS= foreign subdir-objects

EXTRA_DIST= src/releaseDate.hpp lib/Random123-1.08  LICENSE.txt lib/pll/avxLikelihood.c examples
noinst_LIBRARIES=libbayes.a libncl.a libcommdummy.a libpll.a libpll_model.a 
bin_PROGRAMS = yggdrasil parser-exabayes sdsf postProcParam credibleSet extractBips consense
dist_doc_DATA = ./CHANGELOG  ./README.md ./COPYING

# shared/doc/manual
docmanualdir=$(docdir)/manual
dist_docmanual_DATA= \
./manual/manual.pdf ./manual/css/org.css ./manual/img/banner.png \
./manual/img/banner2.png ./manual/manual.html ./manual/library2.html

# shared/doc/examples
docexamplesdir=$(docdir)/examples
dist_docexamples_DATA = examples/configFile-all-options.nex

# shared/doc/examples/dna-unpartitioned
docexamples1dir=$(docdir)/examples/dna-unpartitioned
dist_docexamples1_DATA=examples/dna-unpartitioned/call-parallel.sh \
examples/dna-unpartitioned/call.sh examples/dna-unpartitioned/config.nex

# shared/doc/examples/aa
docexamplesdir2dir=$(docdir)/examples/aa
dist_docexamplesdir2_DATA=examples/aa/call-parallel.sh examples/aa/call.sh \
examples/aa/config.nex

# shared/doc/examples/aa-revmat
docexamplesdir3dir=$(docdir)/examples/aa-revmat
dist_docexamplesdir3_DATA=examples/aa-revmat/call-parallel.sh \
examples/aa-revmat/call.sh \
examples/aa-revmat/aln.phy examples/aa-revmat/config.nex

# shared/doc/examples/dna-partitioned
docexamplesdir4dir=$(docdir)/examples/dna-partitioned
dist_docexamplesdir4_DATA=examples/dna-partitioned/call-parallel.sh \
examples/dna-partitioned/call.sh examples/dna-partitioned/aln.phy \
examples/dna-partitioned/aln.part examples/dna-partitioned/config.nex

# shared/doc/examples/binary
docexamplesdir5dir=$(docdir)/examples/aa
dist_docexamplesdir5_DATA= examples/binary/aln.model \
examples/binary/call-parallel.sh examples/binary/call.sh \
examples/binary/aln.phy examples/binary/config.nex

.PHONY: my-update-src doc man date dist


################################################################
# FLAGS  & preparations  

# enforce, once sanity is back: -Wsign-compare -Wtype-limits

WARN_OFF= -Wno-unused-parameter -Wno-sign-conversion -Wno-unknown-pragmas \
-Wno-inline
GCC_WARN= -Wcast-align -Wchar-subscripts -Wformat-nonliteral -Wformat-y2k \
-Wimport  -Wmissing-field-initializers -Wmissing-format-attribute  -Wpacked \
-Wredundant-decls -Wstack-protector -Wswitch-default -Wswitch-enum \
-Wunreachable-code -Wvariadic-macros -Wvolatile-register-var -Wunused-label \
-Wunused-value -Wint-to-pointer-cast

# the intel compiler way overdoes it with the warnings 
# GCC_WARN+=-Weffc++   

GCC_WARN_OFF= -Wno-suggest-attribute=noreturn -Wno-unknown-warning-option \
-Wno-unknown-warning -Wno-missing-noreturn
WARN_FLAGS=-Wall -Wextra -pedantic -Wcast-qual -Wcomment -Wconversion \
-Wdisabled-optimization -Wfloat-equal  -Wformat  -Wformat=2 -Wformat-security \
-Winit-self -Winline -Winvalid-pch -Wmissing-braces -Wmissing-include-dirs \
-Wparentheses -Wpointer-arith -Wreturn-type -Wsequence-point -Wshadow \
-Wsign-compare -Wstrict-aliasing -Wstrict-aliasing=2 -Wswitch -Wtrigraphs \
-Wuninitialized -Wunknown-pragmas  -Wwrite-strings -Wunused  -Wunused-function \
-Wunused-variable -Wreturn-type -Wmissing-declarations -Wundef  $(WARN_OFF)

if WITH_INTEL
else 
  WARN_FLAGS+=$(GCC_WARN) $(GCC_WARN_OFF)
endif

RAX_FEATURE=-D_GNU_SOURCE -fomit-frame-pointer -funroll-loops -D__STRICT_ANSI__ 

AM_CXXFLAGS=$(RAX_FEATURE) $(WARN_FLAGS) $(PTHREAD_CFLAGS)
AM_CFLAGS=$(RAX_FEATURE) $(WARN_FLAGS) -std=c99
AM_LDFLAGS=$(PTHREAD_LDFLAGS) $(AM_LDLFLAGS)

if WITH_INTEL
else 
 AM_CXXFLAGS+=-Weffc++
endif 


# clarify vector flags 
if WITH_MIC
VECTOR_FLAGS=-DMANUAL_SSE_OVERRIDE -DMANUAL_AVX_OVERRIDE -mmic -D__MIC_NATIVE -opt-streaming-cache-evict=0
else 
if NO_SSE
VECTOR_FLAGS=-DMANUAL_SSE_OVERRIDE -DMANUAL_AVX_OVERRIDE -mno-avx -mno-sse3 # $(SIMD_FLAGS) -march=native -mtune=generic -mno-avx -mno-sse3
else 
if NO_AVX
VECTOR_FLAGS=-DMANUAL_AVX_OVERRIDE -msse3 -mno-avx  # $(SIMD_FLAGS) -march=native -mtune=native -mno-avx -m
else 
VECTOR_FLAGS= $(SIMD_FLAGS) -march=native -mtune=native
endif
endif
endif

AM_CFLAGS+=$(VECTOR_FLAGS)
AM_CXXFLAGS+=$(VECTOR_FLAGS)

folders=$(shell find $(top_srcdir)/src -type d | grep -v '.deps' | grep -v comm | grep -v apps)

SRCINCLUDE=$(shell echo $(folders) | tr ' ' '\n' |  sed "s/\(.*\)/-I\1/" | tr '\n' ' ')
SRCINCLUDE+= -I$(top_srcdir)/src/comm/ -I$(top_srcdir)/src/comm/threads

RAND123_INCLUDES=-isystem $(top_srcdir)/lib/Random123-1.08/include/Random123 -isystem $(top_srcdir)/lib/Random123-1.08/include -isystem $(top_srcdir)/lib/Random123-1.08/examples
NCL_INCLUDES=-isystem $(top_srcdir)/lib/ncl-2.1.18
BAYES_COMMON= $(RAND123_INCLUDES) $(NCL_INCLUDES) $(SRCINCLUDE)  
BAYES_INCLUDES=$(BAYES_COMMON) -I$(top_srcdir)/src/comm/dummy 
BAYES_INCLUDES_PARA= $(BAYES_COMMON)-I$(top_srcdir)/src/comm/mpi 

seqlibs= libncl.a libbayes.a libcommdummy.a  libpll.a libpll_model.a libncl.a  
paralibs= libbayes.a libcommmpi.a libpll.a libpll_model.a libncl.a


################################################################ 
# auto generated sources 

commFolds=./src/comm/ ./src/comm/threads
mpiFolds= ./src/comm/mpi  $(commFolds)
dummyFolds= ./src/comm/dummy $(commFolds)

my-update-src:
	@echo -n "" > src.am 
	@echo "libbayes_a_SOURCES=" $(shell echo "$(top_srcdir)/src/common.h $(top_srcdir)/src/pll.h")    $(shell find $(folders)  -maxdepth 1   -name "*.[cht]pp" ) >> src.am
	@echo "libcommdummy_a_SOURCES=" $(shell find ${dummyFolds} -maxdepth 1 -name "*.[cht]pp" ) >> src.am
	@echo "libcommmpi_a_SOURCES=" $(shell find ${mpiFolds}  -maxdepth 1 -name "*.[cht]pp" ) >> src.am
	@echo "libncl_a_SOURCES=" $(shell find $(top_srcdir)/lib/ncl-2.1.18/ncl -name "*.[cht]") $(shell find $(top_srcdir)/lib/ncl-2.1.18/ncl -name "*.[cht]pp") >> src.am	

include src.am 

################################################################
# libcomdummy 

libcommdummy_a_CXXFLAGS=-O3 $(AM_CXXFLAGS)  $(BAYES_INCLUDES)  


################################################################
# lib comm mpi 
if WITH_MPI 
bin_PROGRAMS+=exabayes 
noinst_LIBRARIES+=libcommmpi.a
libcommmpi_a_CXXFLAGS=$(AM_CXXFLAGS) -O3  $(MPI_CXXFLAGS) $(BAYES_INCLUDES_PARA)    
endif 


################################################################
#  libbayes 

libbayes_a_CXXFLAGS= $(AM_CXXFLAGS) -O3 $(BAYES_INCLUDES)      
libbayes_a_CFLAGS=-O2 $(BAYES_INCLUDES)   


################################################################
# libpll 

libpll_a_SOURCES=./lib/pll/alignment.c ./lib/pll/evaluateGenericSpecial.c \
./lib/pll/globalVariables.h ./lib/pll/lexer.c ./lib/pll/mem_alloc.h \
./lib/pll/genericParallelization.h  ./lib/pll/newick.h ./lib/pll/pllInternal.h \
./lib/pll/recom.c ./lib/pll/stack.h ./lib/pll/utils.c \
./lib/pll/evaluatePartialGenericSpecial.c ./lib/pll/lexer.h \
./lib/pll/newviewGenericSpecial.c ./lib/pll/pll-renamed.h \
./lib/pll/restartHashTable.c ./lib/pll/topologies.c ./lib/pll/bipartitionList.c \
./lib/pll/fastDNAparsimony.c ./lib/pll/makenewzGenericSpecial.c \
./lib/pll/optimizeModel.c ./lib/pll/queue.c ./lib/pll/searchAlgo.c \
./lib/pll/trash.c ./lib/pll/cycle.h ./lib/pll/hash.c ./lib/pll/parsePartition.c \
./lib/pll/queue.h ./lib/pll/ssort.c ./lib/pll/treeIO.c ./lib/pll/errcodes.h \
./lib/pll/hash.h  ./lib/pll/newick.c ./lib/pll/parsePartition.h \
./lib/pll/randomTree.c ./lib/pll/stack.c ./lib/pll/treeIO.h \
./src/avxConditional.c

libpll_model_a_SOURCES= ./lib/pll/models.c 
libpll_model_a_CFLAGS=$(AM_CFLAGS) -w  $(SRCINCLUDE) -O1 $(MIC_FLAGS)

libpll_a_CFLAGS=$(AM_CFLAGS) -w  -O2 $(SRCINCLUDE)   $(MIC_FLAGS)

if WITH_MIC
libpll_a_SOURCES+=./lib/pll/mic_native_aa.c ./lib/pll/mic_native_dna.c ./lib/pll/mic_native.h 
endif


################################################################
# exabayes
exabayes_SOURCES = ./src/apps/exabayes.cpp
exabayes_CXXFLAGS=   $(AM_CXXFLAGS) -O3 $(BAYES_INCLUDES)
exabayes_LDFLAGS=$(MPI_CXXLDFLAGS)
exabayes_LDADD= $(paralibs) $(MPI_CXXLDFLAGS)

################################################################
# yggdrasil 
yggdrasil_SOURCES = ./src/apps/exabayes.cpp
yggdrasil_CXXFLAGS=$(AM_CXXFLAGS) -O3 $(BAYES_INCLUDES)  -D_IS_YGG
yggdrasil_LDADD=$(seqlibs)

################################################################
# SDSF
sdsf_SOURCES= ./src/apps/sdsf.cpp
sdsf_CXXFLAGS= $(AM_CXXFLAGS) -O3 $(BAYES_INCLUDES)    
sdsf_LDADD= $(seqlibs)

################################################################
# the parser 
parser_exabayes_SOURCES=./src/apps/parser.cpp
parser_exabayes_CXXFLAGS=-O3 $(AM_CXXFLAGS) $(BAYES_INCLUDES) -Wno-deprecated-writable-strings
parser_exabayes_LDADD=$(seqlibs)

################################################################
# extractBips
extractBips_SOURCES=./src/apps/extractBips.cpp
extractBips_CXXFLAGS=-O3 $(AM_CXXFLAGS)  $(BAYES_INCLUDES)
extractBips_LDADD=$(seqlibs)

################################################################
# consense
consense_SOURCES=./src/apps/consense.cpp
consense_CXXFLAGS=-O3 $(AM_CXXFLAGS)  $(BAYES_INCLUDES)  
consense_LDADD=$(seqlibs)

################################################################
# credibleSet
credibleSet_SOURCES=./src/apps/credibleSet.cpp
credibleSet_CXXFLAGS=-O3 $(AM_CXXFLAGS)  $(BAYES_INCLUDES)   
credibleSet_LDADD= $(seqlibs)

################################################################
# postProcParam
postProcParam_SOURCES=./src/apps/postProcParam.cpp
postProcParam_CXXFLAGS=-O3 $(AM_CXXFLAGS)  $(BAYES_INCLUDES)   
postProcParam_LDADD=$(seqlibs)

################################################################
# ncl 
libncl_a_CXXFLAGS=  -w $(AM_CXXFLAGS) -O3 $(NCL_INCLUDES) -D__STRICT_ANSI__  

################################################################
# exatest 

if WITH_TEST
 bin_PROGRAMS+=exatest
endif 

exatest_SOURCES = ./tests/exatest.cpp
exatest_CXXFLAGS=$(AM_CXXFLAGS) -O3  $(BAYES_INCLUDES)  -D_IS_YGG
exatest_LDFLAGS=-L/usr/local/lib -lgtest -lpthread 
exatest_LDADD= $(seqlibs) 

if WITH_MPI
 exatest_CXXFLAGS+=-D_WITH_MPI	
 # teststr="mpirun -np 2 ./exatest"
 teststr="./exatest"
else 
 teststr="./exatest"
endif

test : exatest
	$(RM) ExaBayes_*test-*
	eval $(teststr)

################################################################

doc:
	doxygen 

man:
	./manual/process.sh ./manual

date:
	./utils/updateDate.sh

dist-hook:
	make date
