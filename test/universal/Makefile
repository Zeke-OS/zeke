######## Universal unit tests for Zeke project ########

#### Source code under test ####
SRC = $(wildcard ../../src/*.c)

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

# There is a test for following modules
TEST_MODS = $(filter-out test,$(subst _,$(space),$(basename $(notdir $(TEST_SRC)))))

# All source modules
SRC_MODULES = $(filter $(TEST_MODS),$(basename $(notdir $(SRC))))

# Source files of modules that are actually testable
SRC := $(filter $(foreach file,$(TEST_MODS), %/$(file).c), $(SRC))

#### Targets ####
all:  minunit $(OBJ) $(TEST_EXECUTABLES) run
	@echo "'make all' done"

minunit: ../minunit/minunit.c
	@echo "Build unit test framework"
	@echo "=================================================================="
	$(CC) $(IDIR) $(CCFLAGS) -c $^ -o ./$(ODIR)/minunit.o
	@echo "=================================================================="

# Objs under test
$(OBJ): $(SRC)
	$(CC) $(IDIR) $(CCFLAGS) -c $< -o $@

# Tests
bin/%: %.c ./$(ODIR)/minunit.o
	@echo "Build test: $@"
	@echo "=================================================================="
	$(eval TESTS := $(filter-out test,$(subst _,$(space),$(basename $(notdir $@)))))
	$(eval DEPS := $(foreach file,$(addsuffix .o, $(filter $(TESTS), $(SRC_MODULES))),./$(ODIR)/$(file)))
	$(CC) $(CCFLAGS) $(IDIR) $^ $(DEPS) -o ./$@
	@echo "=================================================================="

# Run tests
run: $(TEST_EXECUTABLES)
	@echo "Run all tests"
	@echo "=================================================================="
	@$(foreach exe,$^,echo "$(exe):"; $(exe); )

.PHONY: clean

clean:
	rm -f ./$(ODIR)/*.o ./$(TEST_EXECUTABLES)
