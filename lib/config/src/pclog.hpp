#pragma once
#ifdef NATIVE
#define ESP_LOGD(TAG, ...) fprintf(stderr, __VA_ARGS__)
#define ESP_LOGI(TAG, ...) fprintf(stderr, __VA_ARGS__)
#define ESP_LOGE(TAG, ...) fprintf(stderr, __VA_ARGS__)
#define esp_log_level_get(i) 3
#define ESP_LOG_DEBUG 4
#else
#include <esp_log.h>
#endif