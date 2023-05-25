//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "../include/led_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define BUILT_IN_LED GPIO_NUM_2

//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void config_led_device_up(void);
static void led_manager_task(void* arg);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void config_led_device_up(void)
{
    gpio_set_direction(BUILT_IN_LED, GPIO_MODE_OUTPUT);
}
//------------------------------------------------------------------------------
static void led_manager_task(void* arg)
{
    while(1)
    {
        gpio_set_level(BUILT_IN_LED, 1);
        printf("set_led_pin_on\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(BUILT_IN_LED, 0);
        printf("set_led_pin_off\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void led_manager_init(void)
{
    config_led_device_up();

    xTaskCreate(led_manager_task, "led_manager_task", 2048, NULL, 10, NULL);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------