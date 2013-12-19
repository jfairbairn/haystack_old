LDFLAGS = -levent

OBJECTS = haystack.o
EXE = haystack

$(EXE):	$(OBJECTS)

clean:
	rm -f $(OBJECTS) $(EXE)