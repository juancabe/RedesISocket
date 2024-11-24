CC = gcc
CFLAGS = -std=c11
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

PROGS = servidor clientcp clientudp

all: ${PROGS}

servidor: servidor.c
	${CC} ${CFLAGS} servidor.c -o servidor ${LIBS}
	
clientcp: clientcp.c
	${CC} ${CFLAGS} clientcp.c -o clientcp ${LIBS}

clientudp: clientudp.c
	${CC} ${CFLAGS} clientudp.c -o clientudp ${LIBS}
