CC=gcc
CFLAGS= -std=gnu99 -Wall -fsanitize=address
L_FLAGS=-lm -lrt
TARGET=main
FILES=main.o

${TARGET}: ${FILES}
	${CC} -o ${TARGET} ${FILES} ${L_FLAGS}

main.o: main.c
	${CC} -o main.o -c main.c ${C_FLAGS}

.PHONY: clean

clean:
	-rm -f ${FILES} ${TARGET}