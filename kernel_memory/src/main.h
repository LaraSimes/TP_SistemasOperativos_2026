#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/hello.h>


t_config* iniciar_config(void);
t_log* iniciar_logger(void);

#endif