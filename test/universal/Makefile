#### Source code under test ####
SRC = ../../src/heap.c

#### Tests ####
TEST_SRC = $(wildcard *.c)

IDIR = -I../minunit -I../../include -I../../src -I../../config
ODIR = obj
BDIR = bin
CC = gcc
CCFLAGS = -Wall -pedantic -MMD -DMU_TEST_BUILD

OBJ = $(patsubst %,./$(ODIR)/%,$(notdir $(SRC:.c=.o)))
TEST_EXECUTABLES = $(TEST_SRC:%.c=$(BDIR)/%)

#### Targets ####
all:  minunit $(OBJ) $(TEST_EXECUTABLES) run
	@echo "'make all' done"

# Run tests
run: $(TEST_EXECUTABLES)
	@echo "Run all tests"
	@echo "==============================================================================="
	@$(foreach exe,$^,echo "$(exe):"; $(exe); ) 

# Objs
$(OBJ): $(SRC)
	$(CC) $(IDIR) $(CCFLAGS) -c $< -o $@

minunit: ../minunit/minunit.c
	$(CC) $(IDIR) $(CCFLAGS) -c $^ -o ./$(ODIR)/minunit.o

# Tests
bin/%: %.c ./$(ODIR)/minunit.o $(OBJ)
	$(CC) $(CCFLAGS) $(IDIR) $^ -o ./$@

.PHONY: clean

clean:
	rm -f ./$(ODIR)/*.o ./$(TEST_EXECUTABLES)
