LLVM_MODULES = core native

CXXFLAGS = -g -D _DEBUG -Wall -pedantic -std=c++0x `llvm-config --cppflags`
LFLAGS = `llvm-config --ldflags`
LIBS = `llvm-config --libs $(LLVM_MODULES)`
CXX = g++ 

SRC = $(wildcard *.cpp)
OBJECTS	= $(patsubst %.cpp,obj/%.o,$(SRC)) 

vc: objs
	$(CXX) $(OBJECTS) $(LIBS) $(LFLAGS) -o vc
	cp vc ..

$(OBJECTS): | obj

obj:
	mkdir -p $@

objs:
	$(MAKE) $(OBJECTS)

obj/%.o : %.cpp *.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean	: 
	rm -f obj/*
	rm -f vc

rem : clean vec
