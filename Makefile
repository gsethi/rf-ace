COMPILER = g++
CFLAGS = -O2 -std=c++0x -Wall -Wextra -pedantic -Isrc/
TFLAGS = -pthread
SOURCEFILES = src/progress.cpp src/statistics.cpp src/math.cpp src/stochasticforest.cpp src/rootnode.cpp src/node.cpp src/treedata.cpp src/datadefs.cpp src/utils.cpp src/distributions.cpp
STATICFLAGS = -static-libgcc -static
TESTFILES = test/distributions_test.hpp test/argparse_test.hpp test/datadefs_test.hpp test/stochasticforest_test.hpp test/utils_test.hpp test/math_test.hpp test/rootnode_test.hpp test/node_test.hpp test/treedata_test.hpp
TESTFLAGS = -std=c++0x -L${HOME}/lib/ -lcppunit -ldl -pedantic -I${HOME}/include/ -Itest/ -Isrc/
.PHONY: all test clean  # Squash directory checks for the usual suspects

all: rf-ace

rf-ace: $(SOURCEFILES)
	$(COMPILER) $(CFLAGS) src/rf_ace.cpp $(SOURCEFILES) $(TFLAGS) -o bin/rf-ace

rf-ace-i386: $(SOURCEFILES)
	$(COMPILER) $(CFLAGS) -m32 src/rf_ace.cpp $(SOURCEFILES) $(TFLAGS) -o bin/rf-ace-i386

rf-ace-amd64: $(SOURCEFILES)
	$(COMPILER) $(CFLAGS) -m64 src/rf_ace.cpp $(SOURCEFILES) $(TFLAGS) -o bin/rf-ace-amd64

debug: $(SOURCEFILES)
	$(COMPILER) $(CFLAGS) src/rf_ace.cpp $(SOURCEFILES) $(TFLAGS) -o bin/rf-ace -ggdb

static: $(SOURCEFILES)
	$(COMPILER) $(CFLAGS) src/rf_ace.cpp $(STATICFLAGS) $(SOURCEFILES) $(TFLAGS) -o bin/rf-ace

GBT_benchmark: test/GBT_benchmark.cpp $(SOURCEFILES)
	$(COMPILER) $(CFLAGS) test/GBT_benchmark.cpp $(SOURCEFILES) $(TFLAGS) -o bin/GBT_benchmark

test: $(SOURCEFILES) $(TESTFILES)
	rm -f bin/test; $(COMPILER) $(TESTFLAGS) test/run_tests.cpp $(SOURCEFILES) $(TFLAGS) -o bin/test -ggdb; ./bin/test

clean:
	rm -rf bin/rf-ace bin/benchmark bin/GBT_benchmark bin/test bin/*.dSYM/ src/*.o
