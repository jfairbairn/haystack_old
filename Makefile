CFLAGS = -O2 -g -I/usr/local/include
CXXFLAGS = $(CFLAGS) -std=c++11
LDFLAGS = -lstdc++ -levent

OBJECTS = timeutil.o haystack.o main.o
EXE = haystack

$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXE) $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(EXE)

depend:
	makedepend -Y. -s"# MAKEDEPEND OUTPUT FOLLOWS" *.cpp

#
# MAKEDEPEND OUTPUT FOLLOWS

haystack.o: haystack.h
main.o: haystack.h timeutil.h
timeutil.o: timeutil.h
