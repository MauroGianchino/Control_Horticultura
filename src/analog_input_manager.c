//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_adc/adc_oneshot.h"

#include "../include/analog_input_manager.h"
#include "../include/board_def.h"
#include "../include/global_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define ADC_WIDTH       ADC_WIDTH_BIT_12
#define ADC_ATTEN       ADC_ATTEN_DB_0

#define CUENTAS_ADC_100_PER_PWM 4078
#define HISTERESIS_PER_PWM_UPDATE 50 // histeresis para que se envie una actualizacion en la potencia depwmde salida
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------


//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void analog_input_manager_task(void* arg);
static void config_analog_input(void);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------
adc_oneshot_unit_handle_t adc2_handle;

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------
static void config_analog_input(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_2,
    };
    adc_oneshot_new_unit(&init_config, &adc2_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    adc_oneshot_config_channel(adc2_handle, ADC_POTE_INPUT, &config);

}
//------------------------------------------------------------------------------
static void analog_input_manager_task(void* arg)
{
    int adc_read_value[20];
    uint8_t adc_vec_length = (sizeof(adc_read_value) / sizeof(adc_read_value[0]));
    int val = 0, val_ant = 0;
    uint8_t index = 0;
    int per_pwm = 0;

    config_analog_input();

    while(1)
    {
        adc_oneshot_read(adc2_handle, ADC_POTE_INPUT, &adc_read_value[index]);
        //#ifdef DEBUG_MODULE
        //    printf("Valor ADC channel 5: %d \n", adc_read_value[index]);
        //#endif
        index++;
        if(index == adc_vec_length)
        {
            for(index = 0; index < adc_vec_length; index++)
            {
                val += adc_read_value[index];
            }
            val = val / adc_vec_length;
            index = 0;
            
            if((val - val_ant > HISTERESIS_PER_PWM_UPDATE) || (val_ant - val > HISTERESIS_PER_PWM_UPDATE))
            {
                per_pwm = (val*100) / CUENTAS_ADC_100_PER_PWM;
                global_manager_set_pwm_power_value_manual((uint8_t)per_pwm);
                #ifdef DEBUG_MODULE
                    printf("Valor ADC channel 5: %d \n", val);
                #endif
            } 
            val_ant = val;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void analog_input_manager_init(void)
{
    xTaskCreate(analog_input_manager_task, "analog_input_manager_task", 
        configMINIMAL_STACK_SIZE*4, NULL, configMAX_PRIORITIES-2, NULL);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------