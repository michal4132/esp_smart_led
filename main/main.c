#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_log.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_spiffs.h"

#include <sys/param.h>
#include "string.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h" // TODO: Replace with custom AP/STA handler

#include "httpd_task.h"
#include "config.h"

#ifdef CONFIG_IDF_TARGET_ESP8266
#include "led_task.h"
#endif

static const char *TAG = "led";

void init_spiffs(){
  ESP_LOGI(TAG, "Initializing SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
    .base_path = BASE_PATH,
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true
  };

  // Use settings defined above to initialize and mount SPIFFS filesystem.
  // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
}

void app_main(void){
  ESP_ERROR_CHECK(nvs_flash_init());
  init_spiffs();
  #ifdef CONFIG_IDF_TARGET_ESP8266
    xTaskCreate(led_task, "led_task", 1024 * 2, NULL, 5, NULL);
  #endif
  read_config();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(example_connect());

  httpd_task_init();
  esp_wifi_set_ps(WIFI_PS_NONE);
}
