#include <memory_stick.h>
#include <conexiones.h>


t_log *logger;
int socket_servidor;
char* puerto_escucha; //hacer que iniciar servidor reciba como parametro el puerto de escucha
t_config* config;
int socket_cpu;


char* ip_kernelMemory;
char* puerto_kernelMemory;
int socket_kernelMemory;




int main(int argc, char* argv[]) {
   logger = log_create("log.log", "MemoryStick", 1, LOG_LEVEL_INFO);
   obtenerConfig();


   socket_kernelMemory= crear_conexion(ip_kernelMemory, puerto_kernelMemory);

   if(socket_kernelMemory == -1){
    log_error(logger, "No se pudo conectar a Kernel Memory");
    return EXIT_FAILURE;
   }

   log_info(logger, "## Conectado a Kernel Memory");

   t_modulo modulo = MOD_MEMORY_STICK;

   send(socket_kernelMemory, &modulo, sizeof(int), 0);

   int ok;
   recv(socket_kernelMemory, &ok, sizeof(int), MSG_WAITALL);

   if(ok != 0){
    log_error(logger, "Error en handshake con Kernel Memory");
    close(socket_kernelMemory);
    return EXIT_FAILURE;
   }

   socket_servidor = iniciar_servidor_lara(puerto_escucha);
   log_info(logger, "Memory Stick listo para recibir CPU");
   socket_cpu = esperar_cliente_lara(socket_servidor, logger);


 


   log_destroy(logger);
   config_destroy(config);


   return 0;
}


void obtenerConfig(){
   config = config_create("memory_stick.config");


   if (config == NULL) {
       // Usamos printf porque si el logger falla también, no verías nada
       printf("ERROR: No se pudo encontrar el archivo memory_stick.config en la ruta actual.\n");
       printf("Ruta actual de ejecución: ");
       system("pwd"); // Esto te dirá dónde está parado el programa realmente
       exit(EXIT_FAILURE);
   }


   puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");


   ip_kernelMemory = config_get_string_value(config, "IP_KERNELMEMORY");
   puerto_kernelMemory = config_get_string_value(config, "PUERTO_KERNELMEMORY");
}
