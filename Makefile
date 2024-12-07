CC = gcc
CCApple = clang
CFLAGS = -O3
CFLAGSApple = -x c
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

run: all
	./servidor &
	./cliente TCP root

all: servidor cliente
apple: servidor_apple cliente_apple

servidor: src/servidor/servidor.c
	$(CC) $(CFLAGS) -o servidor src/servidor/servidor.c $(LIBS)

cliente: src/cliente/cliente.c
	$(CC) $(CFLAGS) -o cliente src/cliente/cliente.c $(LIBS)

servidor_apple: src/servidor/servidor.c
	$(CCApple) $(CFLAGSApple) -o servidor src/servidor/servidor.c $(LIBS)

cliente_apple: src/cliente/cliente.c
	$(CCApple) $(CFLAGSApple) -o cliente src/cliente/cliente.c $(LIBS)


