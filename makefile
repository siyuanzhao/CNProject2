server: main.c app_layer.c data_link_layer.c physical_layer.c
	gcc -o server main.c app_layer.c data_link_layer.c physical_layer.c -I.
#client: client.c socket_utilities.c
	#gcc -o client client.c socket_utilities.c -I.
