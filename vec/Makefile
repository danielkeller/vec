CPPFLAGS = -g -Wall

SRC = $(wildcard *.cpp)
OBJECTS	= $(patsubst %.cpp,obj/%.o,$(SRC)) 

vc :	$(OBJECTS)
	$(CXX) $(LFLAGS) $(OBJECTS) -o vc
	cp vc ..

$(OBJECTS): | obj

obj:
	mkdir -p $@

obj/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean	: 
	rm -f obj/*
	rm -f vc

rem : clean vec