CFLAGS = -O2 -g -fno-limit-debug-info
CXXFLAGS = $(CFLAGS) -std=c++11
LDFLAGS = -lc++ -levent -lprotobuf

PB_GENERATED_SOURCES = haystack.pb.h haystack.pb.cc
OBJECTS = timeutil.o haystack.pb.o haystack.o main.o
EXE = haystack

$(EXE): $(OBJECTS)

depend:

$(OBJECTS):	$(PB_GENERATED_SOURCES)

$(PB_GENERATED_SOURCES):	haystack.proto
	protoc --cpp_out=. haystack.proto

clean:
	rm -f $(PB_GENERATED_SOURCES) $(OBJECTS) $(EXE)
