# lanzaServidor.sh
# Lanza el servidor que es un daemon y varios clientes
./servidor
./cliente TCP &
./cliente TCP David &
./cliente TCP @localhost &
./cliente TCP zxcvb@localhost &
./cliente TCP p1777001@localhost &
./cliente UDP &
./cliente UDP david &
./cliente UDP @localhost &
./cliente UDP zxcvb@localhost &
./cliente UDP p1777001@localhost &