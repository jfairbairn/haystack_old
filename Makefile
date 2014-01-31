CFLAGS = -O0 -g -fno-limit-debug-info
CXXFLAGS = $(CFLAGS) -std=c++11
LDFLAGS = -levent -lc++

OBJECTS = haystack.o main.o
EXE = haystack

$(EXE):	$(OBJECTS)

depend: 

clean:
	rm -f $(OBJECTS) $(EXE)
