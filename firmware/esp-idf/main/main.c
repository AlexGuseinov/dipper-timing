/* Dipper timing-leakage test, ESP-IDF application.
 * The measuring task is pinned to the last core (core 1 on the ESP32, core 0
 * on the single-core C6); each measurement runs inside a critical section
 * (interrupts off on that core). WIFI_LOAD=1 builds start Wi-Fi in station
 * mode on core 0 with continuous active scans as background radio load
 * (no access point or credentials needed). */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_clk_tree.h"
#include "esp_idf_version.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include "dudect.h"
#include "platform.h"
#if WIFI_LOAD
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#endif

#ifndef CFG_SEED
#define CFG_SEED 1
#endif
#ifndef CFG_N
#define CFG_N 1000000
#endif
#ifndef CFG_NCAL
#define CFG_NCAL 10000
#endif
#ifndef CFG_NRAW
#define CFG_NRAW 131072
#endif
#ifndef CFG_VARIANTS
#define CFG_VARIANTS 0x1F
#endif
#ifndef CFG_TESTS
#define CFG_TESTS 3
#endif
#ifndef WIFI_LOAD
#define WIFI_LOAD 0
#endif

portMUX_TYPE plat_mux = portMUX_INITIALIZER_UNLOCKED;
static char name_buf[160];
const char *plat_name_str = name_buf;

void plat_print(const char *line){
    fputs(line, stdout);
    fputs("\r\n", stdout);
    fflush(stdout);
}

#if WIFI_LOAD
static void wifi_scan_task(void *arg){
    (void)arg;
    wifi_scan_config_t sc = { 0 };
    for (;;) {
        esp_wifi_scan_start(&sc, true);        /* blocking active scan */
        uint16_t n = 0; esp_wifi_scan_get_ap_num(&n);
        esp_wifi_clear_ap_list();
    }
}
static void wifi_start(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_init();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    xTaskCreatePinnedToCore(wifi_scan_task, "scan", 4096, NULL, 5, NULL, 0);
}
#endif

static void test_task(void *arg){
    (void)arg;
    dudect_cfg_t cfg = { .seed = CFG_SEED, .n = CFG_N, .ncal = CFG_NCAL, .nraw = CFG_NRAW,
                         .variants = CFG_VARIANTS, .tests = CFG_TESTS };
    for (;;) {                                 /* repeat forever; the log keeps the first */
        dudect_run_all(&cfg);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void){
    esp_chip_info_t ci; esp_chip_info(&ci);
    uint32_t cpu_hz = 0;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &cpu_hz);
    uint8_t mac[6]; esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(name_buf, sizeof name_buf,
             "%s-rev%d.%d-cores%d-cpu%luMHz-idf%s-wifi%d-mac%02x%02x%02x%02x%02x%02x-core%d",
             CONFIG_IDF_TARGET, ci.revision / 100, ci.revision % 100, ci.cores,
             (unsigned long)(cpu_hz / 1000000), esp_get_idf_version(), WIFI_LOAD,
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], portNUM_PROCESSORS - 1);
    vTaskDelay(pdMS_TO_TICKS(2000));           /* let the serial reader attach */
#if WIFI_LOAD
    wifi_start();
#endif
    xTaskCreatePinnedToCore(test_task, "dudect", 16384, NULL, 10, NULL, portNUM_PROCESSORS - 1);
}
