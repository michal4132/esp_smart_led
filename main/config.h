#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <stdint.h>
#include <stdbool.h>

#define CONFIG_PATH           "/spiffs/config.json"

bool read_config();

#endif
