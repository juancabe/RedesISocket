CC = gcc
CFLAGS = -std=c11
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

composeFingerTestJuan: composeFinger.c
	$(CC) $(CFLAGS) -O3 -o composeFingerTestJuan composeFinger.c $(LIBS) && echo "Compiled" %% time ./composeFingerTestJuan i0919688

composeFingerTestAll: composeFinger.c
	$(CC) $(CFLAGS) -O3 -o composeFingerTestAll composeFinger.c $(LIBS) && echo "Compiled" %% time ./composeFingerTestAll
