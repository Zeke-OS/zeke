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

space = $(empty) $(empty)
SRC_MODULES = $(basename $(notdir $(SRC)))

#print:
#	echo $(SRC_MODULES)

#### Targets ####
all:  minunit $(OBJ) $(TEST_EXECUTABLES) run
	@echo "'make all' done"

minunit: ../minunit/minunit.c
	$(CC) $(IDIR) $(CCFLAGS) -c $^ -o ./$(ODIR)/minunit.o

# Objs under test
$(OBJ): $(SRC)
	$(CC) $(IDIR) $(CCFLAGS) -c $< -o $@

# Tests
bin/%: %.c ./$(ODIR)/minunit.o
	$(eval TESTS := $(filter-out test,$(subst _,$(space),$(basename $(notdir $@)))))
	$(eval DEPS := $(foreach file,$(addsuffix .o, $(filter $(TESTS), $(SRC_MODULES))),./$(ODIR)/$(file)))
	$(CC) $(CCFLAGS) $(IDIR) $^ $(DEPS) -o ./$@

# Run tests
run: $(TEST_EXECUTABLES)
	@echo "Run all tests"
	@echo "=================================================================="
	@$(foreach exe,$^,echo "$(exe):"; $(exe); )

.PHONY: clean

clean:
	rm -f ./$(ODIR)/*.o ./$(TEST_EXECUTABLES)
