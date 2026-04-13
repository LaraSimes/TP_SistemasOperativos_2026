#include <utils/hello.h>
#define PUERTO "8080" // agregue esto para que no me tire error de compilacion


void saludar(char* quien) {
   printf("Hola desde %s!!\n", quien);
}


int iniciar_servidor(void)
{
   int err;
   int socket_servidor;
   t_log* logger;
  
   struct addrinfo hints, *servinfo;


   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;


  
   err= getaddrinfo(NULL, PUERTO, &hints, &servinfo);


   // Creamos el socket de escucha del servidor
   socket_servidor = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);
   // Asociamos el socket a un puerto
   err = setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
   err = bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
   // Escuchamos las conexiones entrantes
   err = listen(socket_servidor, SOMAXCONN);


   freeaddrinfo(servinfo);
   log_trace(logger, "Listo para escuchar a mi cliente");


   return socket_servidor;
}
int esperar_cliente(int socket_servidor)
{
   // Aceptamos un nuevo cliente
   t_log* logger; // agregue esto para que no me tire error de compilacion
   int socket_cliente = accept(socket_servidor,NULL,NULL);
   log_info(logger, "Se conecto un cliente!");
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
  hints.ai_flags = AI_PASSIVE;




  getaddrinfo(ip, puerto, &hints, &server_info);




  // Ahora vamos a crear el socket.
  //int socket_cliente = 0;
  int socket_cliente = socket(server_info->ai_family,
                       server_info->ai_socktype,
                       server_info->ai_protocol);




  // Ahora que tenemos el socket, vamos a conectarlo




  connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);








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
