CXX = g++
CXXFLAGS = -std=c++11

all: clean npshell.out
	
npshell.out: npshell.cpp
$(CXX) $(CXXFLAGS) -o npshell.out npshell.cpp -DDEBUG -DPIPE -DNUMPIPE -DCOMMAND -DWAIT

run: npshell.out
	./npshell.out

debug: clean
	$(CXX) $(CXXFLAGS) -g -o npshell.out npshell.cpp

gdb: debug
	gdb npshell.out
clean:
	rm -f npshell.out