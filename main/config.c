#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"
#ifdef CONFIG_IDF_TARGET_ESP8266
#include "tcpip_adapter.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32
#include "esp_netif.h"
#endif
#include "led_task.h"

static const char *TAG = "config";

bool read_config(){
  FILE *fd = fopen(CONFIG_PATH, "r");
  if (fd == NULL) {
    ESP_LOGE(TAG, "Error opening config file\n");
    return false;
  }

  fseek(fd, 0, SEEK_END); // seek to end of file
  uint16_t size = ftell(fd); // get current file pointer
  fseek(fd, 0, SEEK_SET); // seek back to beginning of file

  char *buffer;
  buffer = malloc(size);

  uint16_t res = fread(buffer, 1, size, fd);
  if (res <= 0) {
      ESP_LOGE(TAG, "Error reading from config file\n");
      free(buffer);
      return false;
  }

  const cJSON *ap = NULL;
  const cJSON *apSSID = NULL;
  const cJSON *apPASS = NULL;
  const cJSON *sta = NULL;
  const cJSON *staSSID = NULL;
  const cJSON *staPASS = NULL;
  const cJSON *hostname = NULL;
  const cJSON *startColor = NULL;
  const cJSON *color = NULL;
  const cJSON *invert = NULL;


  cJSON *config_json = cJSON_Parse(buffer);
  if(config_json == NULL){
    ESP_LOGE(TAG, "Error parsing config\n");
    free(buffer);
    return false;
  }

  startColor = cJSON_GetObjectItemCaseSensitive(config_json, "startColor");
  uint8_t colors[5];
  uint8_t color_pos = 0;
  cJSON_ArrayForEach(color, startColor){
    if(cJSON_IsNumber(color)){
      colors[color_pos++] = color->valueint;
    }
  }
  set_color(colors[0], colors[1], colors[2]);
  set_warm(colors[3]);
  set_cold(colors[4]);

  apSSID = cJSON_GetObjectItemCaseSensitive(config_json, "apSSID");
  if (cJSON_IsString(apSSID) && (apSSID->valuestring != NULL)){
    printf("apSSID: %s\n", apSSID->valuestring);
  }
  apPASS = cJSON_GetObjectItemCaseSensitive(config_json, "apPASS");
  if (cJSON_IsString(apPASS) && (apPASS->valuestring != NULL)){
    printf("apPASS: %s\n", apPASS->valuestring);
  }

  staSSID = cJSON_GetObjectItemCaseSensitive(config_json, "staSSID");
  if (cJSON_IsString(staSSID) && (staSSID->valuestring != NULL)){
    printf("staSSID: %s\n", staSSID->valuestring);
  }
  staPASS = cJSON_GetObjectItemCaseSensitive(config_json, "staPASS");
  if (cJSON_IsString(staPASS) && (staPASS->valuestring != NULL)){
    printf("staPASS: %s\n", staPASS->valuestring);
  }

  hostname = cJSON_GetObjectItemCaseSensitive(config_json, "hostname");
  if (cJSON_IsString(hostname) && (hostname->valuestring != NULL)){
    printf("hostname: %s\n", hostname->valuestring);

#ifdef CONFIG_IDF_TARGET_ESP8266
    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname->valuestring);
#endif

#ifdef CONFIG_IDF_TARGET_ESP32
    esp_netif_t *esp_netif = NULL;
    esp_netif = esp_netif_next(esp_netif);
    esp_netif_set_hostname(esp_netif, hostname->valuestring);
#endif
  }

  invert = cJSON_GetObjectItemCaseSensitive(config_json, "invert");
  if(cJSON_IsTrue(invert)){
    printf("invert: true\n");
  }else if(cJSON_IsFalse(invert)){
    printf("invert: false\n");
  }

  cJSON_Delete(config_json);
  free(buffer);
  return true;
}
