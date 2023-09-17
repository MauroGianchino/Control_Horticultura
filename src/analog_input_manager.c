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

#define ADC_WIDTH       ADC_BITWIDTH_9
#define ADC_ATTEN       ADC_ATTEN_DB_0

#define CUENTAS_ADC_100_PER_PWM 509
#define HISTERESIS_PER_PWM_UPDATE 8 // histeresis para que se envie una actualizacion en la potencia depwmde salida
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
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&init_config, &adc2_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_WIDTH,
        .atten = ADC_ATTEN,
    };
    adc_oneshot_config_channel(adc2_handle, ADC_POTE_INPUT, &config);

}
//------------------------------------------------------------------------------
static void analog_input_manager_task(void* arg)
{
    int adc_read_value[5];
    uint8_t adc_vec_length = (sizeof(adc_read_value) / sizeof(adc_read_value[0]));
    int val = 0;
    uint8_t index = 0;
    int per_pwm = 0;
    adc_read_value[index] = 0;
    esp_err_t ret;
    config_analog_input();

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    while(1)
    {
        ret = adc_oneshot_read(adc2_handle, ADC_POTE_INPUT, &adc_read_value[index]);
        if(ret == ESP_OK)
        {
            index++;
            if(index == adc_vec_length)
            {

                for(index = 0; index < adc_vec_length; index++)
                {
                    val += adc_read_value[index];
                }
                val = val / adc_vec_length;
                index = 0;

                per_pwm = (val*100) / CUENTAS_ADC_100_PER_PWM;
                global_manager_set_pwm_power_value_manual((uint8_t)per_pwm);
                //#ifdef DEBUG_MODULE
                //    printf("Valor ADC channel 5: %d \n", val);
                //#endif
            }
        }
        else if(ret == ESP_ERR_INVALID_ARG)
        {
            #ifdef DEBUG_MODULE
                printf("ESP_ERR_INVALID_ARG \n");
            #endif
        }
        else if(ret == ESP_ERR_TIMEOUT)
        {
            #ifdef DEBUG_MODULE
                printf("ESP_ERR_TIMEOUT \n");
            #endif
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void analog_input_manager_init(void)
{
    //xTaskCreate(analog_input_manager_task, "analog_input_manager_task", 
        //configMINIMAL_STACK_SIZE*10, NULL, configMAX_PRIORITIES-3, NULL);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------