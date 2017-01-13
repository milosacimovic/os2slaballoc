INCLUDE = ./include
OBJFOLDER =obj
CC=g++
CFLAGS=-Wall -I$(INCLUDE) -std=c++11 -pthread -g

_DEPS = init.h list.h buddy.h slab.h test.h
_OBJ = test.o memory.o list.o buddy.o slab.o main.o

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
