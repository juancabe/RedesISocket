CC = gcc
CCApple = clang
CFLAGS = -O3 -DSEND_BIG_CHUNK -DDEBUG 
CFLAGSApple = -x c -O3 -DDEBUG
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =
SPATH = src/servidor/
CPATH = src/cliente/
SERVIDOR_DEPS = $(SPATH)servidor.c $(SPATH)common_server.h $(SPATH)compose_finger.h $(SPATH)parse_client_request.h $(SPATH)server_TCP.h $(SPATH)server_UDP.h

CLIENTE_DEPS = $(CPATH)cliente.c $(CPATH)client_tcp.h $(CPATH)client_udp.h $(CPATH)common_client.h src/common.h src/common_TCP.h


run: all
	./servidor &
	./cliente TCP root

all: servidor cliente
apple: servidor_apple cliente_apple

servidor: $(SERVIDOR_DEPS)
	$(CC) $(CFLAGS) -o servidor src/servidor/servidor.c $(LIBS)

cliente: $(CLIENTE_DEPS)
	$(CC) $(CFLAGS) -o cliente src/cliente/cliente.c $(LIBS)

servidor_apple: $(SERVIDOR_DEPS)
	$(CCApple) $(CFLAGSApple) -o servidor src/servidor/servidor.c $(LIBS)

cliente_apple: $(CLIENTE_DEPS)
	$(CCApple) $(CFLAGSApple) -o cliente src/cliente/cliente.c $(LIBS)


