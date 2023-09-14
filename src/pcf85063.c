//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2c.h"
#include "sdkconfig.h"

#include "../include/pcf85063.h"
#include "../include/board_def.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 

#define QUEUE_ELEMENT_QUANTITY 20

#define I2C_SCL_PIN 22 // SCL Pin
#define I2C_SDA_PIN 21 // SDA Pin
#define I2C_FREQ_HZ 400000 // Frecuencia de I2C, 100KHz en este ejemplo

#define I2C_MASTER_NUM   I2C_NUM_0  // I2C port number
#define PCF85063_ADDRESS 0x51

#define TIMEOUT_MS 2000
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    SET_CURRENT_TIME = 1,
    GET_CURRENT_TIME = 2,
}pcf85063_cmds_t;

typedef struct{
    pcf85063_cmds_t cmd;
    struct tm current_time;
}pcf85063_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t pcf85063_queue, response_queue;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void pcf85063_task(void* arg);
static void i2c_master_init(void);
uint8_t dec_to_bcd(uint8_t val);
uint8_t bcd_to_dec(uint8_t val);
void pcf85063_write_register(uint8_t reg, uint8_t value);
uint8_t pcf85063_read_register(uint8_t reg);


static void pcf85063_get_current_time(void);
static uint8_t pcf85063_wait_get_current_time_response(struct tm *current_time); 
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Function to convert decimal to BCD
uint8_t dec_to_bcd(uint8_t val)
{
    return ((val / 10 * 16) + (val % 10));
}
//------------------------------------------------------------------------------
// Function to convert BCD to decimal
uint8_t bcd_to_dec(uint8_t val)
{
    return ((val / 16 * 10) + (val % 16));
}
//------------------------------------------------------------------------------
// Write a byte to a register of the RTC
void pcf85063_write_register(uint8_t reg, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF85063_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 2000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}
//------------------------------------------------------------------------------
// Read a byte from a register of the RTC
uint8_t pcf85063_read_register(uint8_t reg)
{
    uint8_t value;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF85063_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF85063_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &value, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 2000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return value;
}
//------------------------------------------------------------------------------
void set_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    pcf85063_write_register(0x04, dec_to_bcd(seconds));
    pcf85063_write_register(0x05, dec_to_bcd(minutes));
    pcf85063_write_register(0x06, dec_to_bcd(hours));
}
//------------------------------------------------------------------------------
void read_time(struct tm *current_time)
{
    struct tm time_aux;

    time_aux.tm_sec = bcd_to_dec(pcf85063_read_register(0x04) & 0x7F);
    time_aux.tm_min = bcd_to_dec(pcf85063_read_register(0x05));
    time_aux.tm_hour = bcd_to_dec(pcf85063_read_register(0x06));

    uint8_t osc_status = bcd_to_dec(pcf85063_read_register(0x04) & 0x80);
    
#ifdef DEBUG_MODULE
    printf("Time: %02d:%02d:%02d\n", time_aux.tm_hour, time_aux.tm_min, time_aux.tm_sec);
    printf("Oscilator status: %02d\n", osc_status);
#endif
    *current_time = time_aux;
}
//------------------------------------------------------------------------------
static void i2c_master_init(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA_PIN;
    conf.scl_io_num = I2C_SCL_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_FREQ_HZ;
    
    i2c_param_config(I2C_MASTER_NUM, &conf);

    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}
//------------------------------------------------------------------------------
static void pcf85063_task(void* arg)
{
    pcf85063_event_t ev;
    i2c_master_init();

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    while(1)
    {
        if(xQueueReceive(pcf85063_queue, &ev, 1000 / portTICK_PERIOD_MS) == pdTRUE)
        {
            switch(ev.cmd)
            {
                case CMD_UNDEFINED:
                    break;
                case SET_CURRENT_TIME:
                    set_time(ev.current_time.tm_hour, ev.current_time.tm_min, ev.current_time.tm_sec);
                    break;
                case GET_CURRENT_TIME:
                    read_time(&ev.current_time);
                    xQueueSend(response_queue, &ev, 10);
                    break;
                default:
                    break;
            }
        }
        else
        {
        }   
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void pcf85063_init(void)
{
    
    pcf85063_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(pcf85063_event_t));
    response_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(pcf85063_event_t));

    xTaskCreate(pcf85063_task, "pcf85063_task", configMINIMAL_STACK_SIZE*5, 
        NULL, configMAX_PRIORITIES-2, NULL);
}
//------------------------------------------------------------------------------
void pcf85063_set_current_time(struct tm current_time)
{
    pcf85063_event_t ev;

    ev.cmd = SET_CURRENT_TIME;
    ev.current_time = current_time;
    xQueueSend(pcf85063_queue, &ev, 10);
}
//------------------------------------------------------------------------------
static void pcf85063_get_current_time(void)
{
    pcf85063_event_t ev;

    ev.cmd = GET_CURRENT_TIME;
    xQueueSend(pcf85063_queue, &ev, 10);
}
static uint8_t pcf85063_wait_get_current_time_response(struct tm *current_time) 
{
    pcf85063_event_t resp_ev;

    if(xQueueReceive(response_queue, &resp_ev, TIMEOUT_MS / portTICK_PERIOD_MS)) 
    {
        *current_time = resp_ev.current_time;
        return 1;
    } 
    else 
    {
        #ifdef DEBUG_MODULE
            printf("Error al recibir la respuesta de pwm\n");
        #endif
        return 0;
    }
}

uint8_t pcf85063_get_current_time_info(struct tm *current_time)
{
    pcf85063_get_current_time();
    if(pcf85063_wait_get_current_time_response(current_time))
    {
        return(1);
    }
    return(0);
}

//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------