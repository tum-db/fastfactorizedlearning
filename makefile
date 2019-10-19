CFLAGS=-std=c++17 -Wall -Wextra -g -Wno-unknown-pragmas -pedantic -Weffc++ -Wsign-conversion#-O2
PQFLAGS=-lpqxx -lpq
test=test
testDEPS=regression.cpp variableOrder.cpp
P1=testComp
P1DEPS=regressionSSMSComp.cpp variableOrderComp.cpp

$(test): $(test).cpp $(testDEPS)
	g++ $(CFLAGS) $(test).cpp $(testDEPS) -o $(test) 2> err.txt $(PQFLAGS)

$(P1): $(P1).cpp $(P1DEPS)
	g++ $(CFLAGS) $(P1).cpp $(P1DEPS) -o $(P1) 2> err.txt $(PQFLAGS)
