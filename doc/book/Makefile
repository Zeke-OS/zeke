SRC_PLOTS = $(wildcard plots/*.p)
PLOTS = $(patsubst %.p, %.eps, $(SRC_PLOTS))

GV_FILES := $(wildcard gfx/*.gv)
DOT_OUT := $(patsubst %.gv, %.eps, $(GV_FILES))

PIC_FILES := $(wildcard gfx/*.pic)
PIC_OUT := $(patsubst %.pic, %.eps, $(PIC_FILES))

define dot-command
	dot -Teps $*.gv -o $@
endef

define pic-command
	pic2plot -T ps $*.pic | ps2eps -- > $@
endef

.SUFFIXES:
.SUFFIXES: .tex .gv .pic .eps .pdf

.PHONY: book.tex all clean

all: book.pdf

$(PLOTS): $(SRC_PLOTS)
	cd plots; \
		gnuplot $(notdir $(basename $@).p)

$(DOT_OUT): $(GV_FILES)
	$(dot-command)

$(PIC_OUT): $(PIC_FILES)
	$(pic-command)

book.pdf: book.tex $(PLOTS) $(DOT_OUT) $(PIC_OUT)
	latexmk -bibtex -dvi -use-make $<
	dvipdf $*.dvi $@

clean:
	latexmk -C
	$(RM) $(PLOTS) $(DOT_OUT) $(PIC_OUT)
