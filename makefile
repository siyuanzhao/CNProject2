server2: main.c app_layer.c data_link_layer.c physical_layer.c
	gcc physical_layer.c data_link_layer.c app_layer.c main.c -o server2 -lrt -L/opt/lib.
#client: client.c socket_utilities.c
	#gcc -o client client.c socket_utilities.c -I.
