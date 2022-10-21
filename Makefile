CXX=g++
CXXFLAGS:=-Wall -Werror -Wextra -pedantic -std=c++17 -fopenmp
RELEASEFLAGS:=-O3 -fno-rtti
DEBUGFLAGS:=-g

.PHONY: all clean
all: submission

submission: main.o
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -o troons $^

main.o: main.cc
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $^

clean:
	$(RM) *.o troons troons_bonus

debug: main.cc
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -D DEBUG -o troons main.cc

bonus: bonus.o
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -o troons_bonus $^

bonus.o: bonus.cc
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -c $^

