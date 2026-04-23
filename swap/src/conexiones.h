#include <commons/log.h>
#include <commons/config.h>

#include <stdio.h> 
#include <stdlib.h> 
#include "utils/general.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "utils/cliente.h"

int liberar(t_log *log, t_config *config, int conexion);
void handshakear (int socket_cliente, t_log *log);