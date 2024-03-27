CXX = g++
CXXFLAGS = -std=c++11

all: clean npshell
	
npshell: npshell.cpp
	$(CXX) $(CXXFLAGS) -o npshell npshell.cpp

cleanrun: npshell.cpp
	$(CXX) $(CXXFLAGS) -o npshell npshell.cpp -DDEBUG -DPIPE -DNUMPIPE -DCOMMAND -DWAIT -DCLOSEPIPE -DEXEC

run: npshell
	./npshell

debug: clean
	$(CXX) $(CXXFLAGS) -g -o npshell npshell.cpp

gdb: debug
	gdb npshell
clean:
	rm -f npshell