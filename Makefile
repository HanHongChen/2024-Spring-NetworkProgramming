CXX = g++
CXXFLAGS = -std=c++11

all: clean np_simple
	
np_simple: np_simple.cpp
	$(CXX) $(CXXFLAGS) -o np_simple np_simple.cpp

cleanrun: np_simple.cpp
	$(CXX) $(CXXFLAGS) -o np_simple np_simple.cpp -DDEBUG -DPIPE -DNUMPIPE -DCOMMAND -DWAIT -DCLOSEPIPE -DEXEC

run: np_simple
	./np_simple

debug: clean
	$(CXX) $(CXXFLAGS) -g -o np_simple np_simple.cpp

gdb: debug
	gdb np_simple
clean:
	rm -f np_simple