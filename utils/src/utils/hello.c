#include <utils/hello.h>


void saludar(char* quien) {
   printf("Hola desde %s!!\n", quien);
}



int iniciar_servidor(char* puerto) {
    int socket_servidor;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(NULL, puerto, &hints, &servinfo);
    if (err != 0) {
        return -1;
    }

    socket_servidor = socket(servinfo->ai_family,
                             servinfo->ai_socktype,
                             servinfo->ai_protocol);

    if (socket_servidor == -1) {
        freeaddrinfo(servinfo);
        return -1;
    }

    int opt = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        freeaddrinfo(servinfo);
        close(socket_servidor);
        return -1;
    }

    if (listen(socket_servidor, SOMAXCONN) == -1) {
        freeaddrinfo(servinfo);
        close(socket_servidor);
        return -1;
    }

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
   int socket_cliente = accept(socket_servidor,NULL,NULL);
   return socket_cliente;
}


int iniciar_servidor_lara(char* puerto)
{
   int socket_servidor;
   struct addrinfo hints, *servinfo;


   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;


   // getaddrinfo ahora recibe el puntero al string directamente
   getaddrinfo(NULL, puerto, &hints, &servinfo);


   socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  
   int yes = 1;
   setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));


   bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
   listen(socket_servidor, SOMAXCONN);


   freeaddrinfo(servinfo);
   return socket_servidor;
}


int crear_conexion(char *ip, char* puerto)
{
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, puerto, &hints, &server_info) != 0) {
        return -1;
    }

    int socket_cliente = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);

    if (socket_cliente == -1) {
        freeaddrinfo(server_info);
        return -1;
    }

    if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        close(socket_cliente);
        freeaddrinfo(server_info);
        return -1;
    }

    freeaddrinfo(server_info);

    return socket_cliente;
}


int esperar_cliente_lara(int socket_servidor, t_log* logger)
{
   int socket_cliente = accept(socket_servidor,NULL,NULL);
   if(socket_cliente != -1) {
       log_info(logger, "Se conecto un cliente!");
   }
   return socket_cliente;
}
