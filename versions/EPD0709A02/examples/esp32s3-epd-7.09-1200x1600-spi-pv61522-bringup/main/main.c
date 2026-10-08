/*
 * SPDX-FileCopyrightText: Copyright 2026 OSPTEK
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * https://github.com/osptek
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"
#include "epd709.h"
#include "status.h"

static const char *TAG = "app_main";

static void epd_bringup_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "7.09\" color bar: black blue green red yellow white");
    ESP_LOGI(TAG, "pins RST=%d BUSY=%d DC=%d CS-M=%d CS-S=%d SCL=%d SI0=%d",
             PIN_EPD_RST, PIN_EPD_BUSY, PIN_EPD_DC,
             PIN_EPD_CS0, PIN_EPD_CS1, PIN_EPD_SCK, PIN_EPD_D0);

    if (initEPD() != DONE) {
        ESP_LOGE(TAG, "EPD init stopped; BUSY did not go high");
        vTaskDelete(NULL);
        return;
    }
    if (epdDisplayColorBar() != DONE) {
        ESP_LOGE(TAG, "color bar refresh stopped");
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    xTaskCreate(epd_bringup_task, "epd_bringup", 8192, NULL, 5, NULL);
}
