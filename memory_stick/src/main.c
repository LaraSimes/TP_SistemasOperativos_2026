#include <utils/hello.h>

int main(int argc, char* argv[]) {
    logger = log_create("log.log", "MemoryStick", 1, LOG_LEVEL_INFO);
    int server_fd = iniciar_servidor();
    log_info(logger, "Memory Stick listo para recibir CPU");
    int cliente_fd = esperar_cliente(server_fd);


    saludar("memory_stick");
    return 0;
}


