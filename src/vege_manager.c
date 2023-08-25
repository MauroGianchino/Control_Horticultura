//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "../include/board_def.h"
#include "../include/vege_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE
#define GPIO_OUTPUT_PIN_SEL  (1ULL << RELE_OUTPUT)
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------


//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void vege_manager_init(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; // desactivar interrupción
    io_conf.mode = GPIO_MODE_OUTPUT; // establecer en modo salida
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; // configurar pin
    io_conf.pull_down_en = 0; // desactivar pull-down
    io_conf.pull_up_en = 0; // desactivar pull-up
    gpio_config(&io_conf);

    gpio_set_level(RELE_OUTPUT, 0);
}
//------------------------------------------------------------------------------
void vege_manager_turn_on(void)
{
    #ifdef DEBUG_MODULE
        printf("TURN ON RELE VEGE \n");
    #endif
    gpio_set_level(RELE_OUTPUT, 1);
}
//------------------------------------------------------------------------------
void vege_manager_turn_off(void)
{
    #ifdef DEBUG_MODULE
        printf("TURN OFF RELE VEGE \n");
    #endif
    gpio_set_level(RELE_OUTPUT, 0);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------