#include "app_layer.h"
#include "data_link_layer.h"
#include <string.h>

void run_command(char *command) {
	DataLinkSend(command, strlen(command)+1, APPDATA);
}