CXXFLAGS = -g -Wall -pedantic -std=c++11
CXX = clang++
# REQUIRES clang 3.1 or later to compile

SRC = $(wildcard *.cpp)
OBJECTS	= $(patsubst %.cpp,obj/%.o,$(SRC)) 

vc :	$(OBJECTS)
	$(CXX) $(LFLAGS) $(OBJECTS) -o vc
	cp vc ..

$(OBJECTS): | obj

obj:
	mkdir -p $@

obj/%.o : %.cpp *.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean	: 
	rm -f obj/*
	rm -f vc

rem : clean vec
