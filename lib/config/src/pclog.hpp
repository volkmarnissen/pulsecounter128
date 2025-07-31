#pragma once
#ifdef NATIVE
#define ESP_LOGI(TAG, ...) fprintf(stderr, __VA_ARGS__)
#define ESP_LOGE(TAG, ...) fprintf(stderr, __VA_ARGS__)
#else
#include <esp_log.h>
#endif