SRC_PLOTS = $(wildcard plots/*.p)
PLOTS = $(patsubst %.p, %.eps, $(SRC_PLOTS))

.PHONY: book.tex all clean

all: book.pdf

$(PLOTS): $(SRC_PLOTS)
	cd plots; \
		gnuplot $(notdir $(basename $@).p)

book.pdf: book.tex $(PLOTS)
	latexmk -bibtex -dvi -use-make $<
	dvipdf book.dvi book.pdf

clean:
	latexmk -C
