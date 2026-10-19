/* Platform layer for ESP-IDF (ESP32 Xtensa LX6, ESP32-C6 RISC-V).
 * Timer: esp_cpu_get_cycle_count() -- CCOUNT on Xtensa, the custom mpccr
 * performance counter on the C6 HP core. mpccr saturates instead of wrapping,
 * so on the C6 it is reset to 0 before every timed region (outside it). */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include "esp_cpu.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"

extern portMUX_TYPE plat_mux;
extern const char *plat_name_str;
#define PLAT_NAME  plat_name_str
#if CONFIG_IDF_TARGET_ESP32C6
#define PLAT_TIMER "esp_cpu_get_cycle_count(mpccr,reset-before-each)"
#else
#define PLAT_TIMER "esp_cpu_get_cycle_count(CCOUNT)"
#endif

typedef uint32_t plat_t;
static inline void plat_irq_off(void){
    portENTER_CRITICAL(&plat_mux);
#if CONFIG_IDF_TARGET_ESP32C6
    esp_cpu_set_cycle_count(0);
#endif
}
static inline void plat_irq_on(void){ portEXIT_CRITICAL(&plat_mux); }
static inline plat_t plat_now(void){ return (plat_t)esp_cpu_get_cycle_count(); }
static inline uint32_t plat_delta(plat_t t0, plat_t t1){ return t1 - t0; }
void plat_print(const char *line);
#endif
