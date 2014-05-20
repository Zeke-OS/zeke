# Makefile for unifdef

all: unifdef

unifdef: unifdef.c unifdef.h version.h
	${CC} ${CFLAGS} ${LDFLAGS} -o unifdef unifdef.c

clean:
	rm -f unifdef
