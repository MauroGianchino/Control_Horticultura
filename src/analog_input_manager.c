//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "esp_adc/adc_oneshot.h"

#include "../include/analog_input_manager.h"
#include "../include/board_def.h"
#include "../include/nv_flash_manager.h"

//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define ADC_WIDTH       ADC_BITWIDTH_9
#define ADC_ATTEN       ADC_ATTEN_DB_0

#define CUENTAS_ADC_100_PER_PWM 509
#define HISTERESIS_PER_PWM_UPDATE 8 // histeresis para que se envie una actualizacion en la potencia depwmde salida

#define QUEUE_ELEMENT_QUANTITY 20
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    CHANGE_PWM_MODE = 1,
    MAX_POTE_REFERENCE = 2,
    CALIBRATE_POTE = 3,
}adc_cmds_t;

typedef struct{
    adc_cmds_t cmd;
    output_mode_t pwm_mode;
    uint16_t max_pote_reference;
}adc_data_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------


//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void analog_input_manager_task(void* arg);
static void config_analog_input(void);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------
static QueueHandle_t adc_data_queue;

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
    adc_data_t adc_data_ev;
    output_mode_t pwm_mode = AUTOMATIC;

    int adc_read_value[5];
    uint8_t adc_vec_length = (sizeof(adc_read_value) / sizeof(adc_read_value[0]));
    int val = 0;
    uint8_t index = 0;
    int per_pwm = 0;
    adc_read_value[index] = 0;
    esp_err_t ret;
    uint16_t max_pote_reference = CUENTAS_ADC_100_PER_PWM;
    uint8_t calibrate_pote_status_on = false;
    config_analog_input();

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    while(1)
    {
        if(xQueueReceive(adc_data_queue, &adc_data_ev, 30 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch(adc_data_ev.cmd)
            {
                case CMD_UNDEFINED:
                    break;
                case CHANGE_PWM_MODE:
                    pwm_mode = adc_data_ev.pwm_mode;
                    break;
                case MAX_POTE_REFERENCE:
                    max_pote_reference = adc_data_ev.max_pote_reference;
                    break;
                case CALIBRATE_POTE:
                    calibrate_pote_status_on = true;
                    break;
            }
        }
        else
        {
            if((pwm_mode == MANUAL_ON) && (calibrate_pote_status_on == false))
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

                        per_pwm = (val*100) / max_pote_reference;
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
            }
            else if(calibrate_pote_status_on == true)
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

                        write_parameter_on_flash_uint32(POTE_MAX_REFERENCE_KEY, (uint32_t)val);
                        vTaskDelay(4000 / portTICK_PERIOD_MS);
                        esp_restart();

                        #ifdef DEBUG_MODULE
                            printf("Nuevo valor pote al 100: %d \n", val);
                        #endif
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
            }  
        }
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void analog_input_manager_init(void)
{
    adc_data_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(adc_data_t));

    xTaskCreate(analog_input_manager_task, "analog_input_manager_task", 
        configMINIMAL_STACK_SIZE*10, NULL, configMAX_PRIORITIES-2, NULL);
}
//------------------------------------------------------------------------------
void analog_input_send_pwm_mode(output_mode_t pwm_mode)
{
    adc_data_t ev;
    ev.cmd = CHANGE_PWM_MODE;
    ev.pwm_mode = pwm_mode;
    xQueueSend(adc_data_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void analog_input_set_max_pote_reference(uint16_t max_pote_reference)
{
    adc_data_t ev;
    ev.cmd = MAX_POTE_REFERENCE;
    ev.max_pote_reference = max_pote_reference;
    xQueueSend(adc_data_queue, &ev, 10);
}
//------------------------------------------------------------------------------
void analog_input_calibrate_pote(void)
{
    adc_data_t ev;
    ev.cmd = CALIBRATE_POTE;
    xQueueSend(adc_data_queue, &ev, 10);
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------