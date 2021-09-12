#ifndef __HTTPD_TASK_H__
#define __HTTPD_TASK_H__

#define DEFAULT_WIFI_MODE     WIFI_MODE_AP
#define DEFAULT_WIFI_STA_SSID CONFIG_EXAMPLE_WIFI_SSID
#define DEFAULT_WIFI_STA_PASS CONFIG_EXAMPLE_WIFI_PASSWORD
#define LISTEN_PORT           80
#define MAX_CONNECTIONS       6
#define FILE_PATH_MAX         100
#define BASE_PATH             "/spiffs"

//protocol messages
#define SET_RGB               0x01
#define SET_WARM              0x02
#define SET_COLD              0x03
#define GET_STATUS            0x04
#define FADE_RGB              0x05
#define SET_ALL               0x06

typedef struct {
	uint32_t stringPos;
} LongStringState;

void httpd_task_init();
#endif
