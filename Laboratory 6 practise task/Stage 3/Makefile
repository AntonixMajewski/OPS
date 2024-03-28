CC=gcc
C_FLAGS= -std=gnu99 -Wall -Werror -pedantic -g
L_FLAGS=-lm -lrt

CLIENT=client
CLIENT_FILES=client.o

SERVER=server
SERVER_FILES=server.o

.PHONY: all clean

all: ${CLIENT} ${SERVER}

${CLIENT}: ${CLIENT_FILES}
	${CC} -o ${CLIENT} ${CLIENT_FILES} ${L_FLAGS}

client.o:
	${CC} -c client.c -o client.o ${C_FLAGS}

${SERVER}: ${SERVER_FILES}
	${CC} -o ${SERVER} ${SERVER_FILES} ${L_FLAGS}

server.o:
	${CC} -c server.c -o server.o ${C_FLAGS}

clean:
	-rm -f ${CLIENT} ${CLIENT_FILES} \
		${SERVER} ${SERVER_FILES}
