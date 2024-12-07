CC = gcc
CFLAGS = -O3
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

run: all
	./servidor &
	./cliente TCP root

all: servidor cliente

servidor: src/servidor/servidor.c
	$(CC) $(CFLAGS) -o servidor src/servidor/servidor.c $(LIBS)

cliente: src/cliente/cliente.c
	$(CC) $(CFLAGS) -o cliente src/cliente/cliente.c $(LIBS)


