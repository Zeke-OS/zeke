SRC_PLOTS = $(wildcard plots/*.p)
PLOTS = $(patsubst %.p, %.eps, $(SRC_PLOTS))

GV_FILES := $(wildcard gfx/*.gv)
DOT_OUT := $(patsubst %.gv, %.eps, $(GV_FILES))

define dot-command
	dot -Teps $*.gv -o $@
endef

.SUFFIXES:
.SUFFIXES: .tex .gv .eps .pdf

.PHONY: book.tex all clean

all: book.pdf

$(PLOTS): $(SRC_PLOTS)
	cd plots; \
		gnuplot $(notdir $(basename $@).p)

$(DOT_OUT): $(GV_FILES)
	$(dot-command)

book.pdf: book.tex $(PLOTS) $(DOT_OUT)
	latexmk -bibtex -dvi -use-make $<
	dvipdf $*.dvi $@

clean:
	latexmk -C
	$(RM) $(DOT_OUT) $(PLOTS)
