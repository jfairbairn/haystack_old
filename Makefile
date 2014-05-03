CFLAGS = -O2 -g -I/usr/local/include
CXXFLAGS = $(CFLAGS) -std=c++11
LDFLAGS = -lstdc++ -levent -lprotobuf

PB_GENERATED_SOURCES = haystack.pb.h haystack.pb.cc
OBJECTS = timeutil.o haystack.pb.o haystack.o main.o
EXE = haystack

$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXE) $(LDFLAGS)

$(OBJECTS):	$(PB_GENERATED_SOURCES)

$(PB_GENERATED_SOURCES):	haystack.proto
	protoc --cpp_out=. haystack.proto

clean:
	rm -f $(PB_GENERATED_SOURCES) $(OBJECTS) $(EXE)

depend:
	makedepend -Y. -s"# MAKEDEPEND OUTPUT FOLLOWS" *.cpp

#
# MAKEDEPEND OUTPUT FOLLOWS

haystack.o: haystack.h
main.o: haystack.h timeutil.h
timeutil.o: timeutil.h
