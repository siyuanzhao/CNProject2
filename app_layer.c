#include "app_layer.h"
#include "data_link_layer.h"

void run_command(char *command) {
	DataLinkSend(command, sizeof(command), APPDATA);
}