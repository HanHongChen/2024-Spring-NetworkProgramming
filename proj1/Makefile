CXX = g++
CXXFLAGS = -std=c++11

all: clean npshell.out
	
npshell.out: npshell.cpp
	$(CXX) $(CXXFLAGS) -o npshell.out npshell.cpp

run: npshell.out
	./npshell.out

debug: clean run
	$(CXX) $(CXXFLAGS) -o npshell.out npshell.cpp 

clean:
	rm -f npshell.out