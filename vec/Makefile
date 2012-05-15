CXXFLAGS = -g -Wall

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
