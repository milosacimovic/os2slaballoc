INCLUDE = ./include
OBJFOLDER =obj
CC=g++
CFLAGS=-Wall -I$(INCLUDE)

_DEPS =
_OBJ = test.o

DEPS = $(patsubst %,$(INCLUDE)/%,$(_DEPS))
OBJ = $(patsubst %,$(OBJFOLDER)/%,$(_OBJ))

$(OBJFOLDER)/%.o: %.cpp $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)
test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
.PHONY: clean
clean:
	rm -f $(OBJFOLDER)/*.o
	rm -f test
