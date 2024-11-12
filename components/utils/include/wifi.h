#ifndef WIFI_H
#define WIFI_H


#include <stdio.h>
#include <sys/time.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void wifi_init_sta(void);

#endif // WIFI_H