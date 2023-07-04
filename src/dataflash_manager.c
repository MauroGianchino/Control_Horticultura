//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "freertos/queue.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "../include/dataflash_manager.h"
#include "../include/board_def.h"
#include "../include/global_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define DEBUG_MODULE 1

#define QUEUE_ELEMENT_QUANTITY 20

#define NVS_NAMESPACE "device"
#define MAX_VALUE_LENGTH 20
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    CONFIG_FLASH = 1,
    READ_FLASH = 2,
    WRITE_FLASH = 3,
}dataflash_event_cmds_t;

typedef struct {
    char key[MAX_VALUE_LENGTH];
    char value[MAX_VALUE_LENGTH];
}operation_info_t;

typedef struct{
    dataflash_event_cmds_t cmd;
    operation_info_t operation_info;
}dataflash_event_t;
//------------------- DECLARACION DE DATOS LOCALES -----------------------------
//------------------------------------------------------------------------------
static QueueHandle_t dataflash_manager_queue;
static QueueHandle_t dataflash_response_queue;
//------------------- DECLARACION DE FUNCIONES LOCALES -------------------------
//------------------------------------------------------------------------------
static void dataflash_manager_task(void* arg);
//------------------- DEFINICION DE DATOS LOCALES ------------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE DATOS GLOBALES -----------------------------
//------------------------------------------------------------------------------

//------------------- DEFINICION DE FUNCIONES LOCALES --------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static void dataflash_manager_task(void* arg)
{
    dataflash_event_t ev;
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    size_t len;

    while(1)
    {
        if(xQueueReceive(dataflash_manager_queue, &ev, portMAX_DELAY) == pdTRUE)
        {
            switch(ev.cmd)
            {
                case CMD_UNDEFINED:
                    printf("Operación no válida\n");
                    break;
                case CONFIG_FLASH:
                    ret = nvs_flash_init();
                    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
                    {
                        ESP_ERROR_CHECK(nvs_flash_erase());
                        ret = nvs_flash_init();
                    }
                    if(ret != ESP_OK) 
                    {
                        printf("Error al inicializar el subsistema NVS\n");
                    } 
                    else 
                    {
                        printf("Subsistema NVS inicializado correctamente\n");
                    }
                    break;
                case READ_FLASH:
                    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
                    if(ret != ESP_OK) 
                    {
                        printf("Error al abrir el espacio de almacenamiento NVS\n");
                    } 
                    else 
                    {
                        char value[MAX_VALUE_LENGTH];
                        ret = nvs_get_str(nvs_handle, ev.operation_info.key, value, &len);
                        if(ret == ESP_OK) 
                        {
                            printf("Valor de %s: %s\n", ev.operation_info.key, value);
                            strncpy( ev.operation_info.value, value, MAX_VALUE_LENGTH);
                            xQueueSend(dataflash_response_queue, &ev, 0);  // Envía la respuesta a la cola de respuesta
                        } 
                        else if(ret == ESP_ERR_NVS_NOT_FOUND) 
                        {
                            printf("Clave %s no encontrada\n", ev.operation_info.key);
                        } 
                        else 
                        {
                            printf("Error al leer del NVS\n");
                        }
                        nvs_close(nvs_handle);
                    }
                    break;
                case WRITE_FLASH:
                    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
                    if(ret != ESP_OK) 
                    {
                        printf("Error al abrir el espacio de almacenamiento NVS\n");
                    } 
                    else 
                    {
                        ret = nvs_set_str(nvs_handle, ev.operation_info.key, ev.operation_info.value);
                        if(ret == ESP_OK) 
                        {
                            printf("Valor %s guardado correctamente para la clave %s\n", ev.operation_info.value, ev.operation_info.key);
                        } 
                        else 
                        {
                            printf("Error al escribir en el NVS\n");
                        }
                        nvs_close(nvs_handle);
                    }
                    break;
                default:
                    printf("Operación no válida\n");
                    break;
            }       
        }
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
// TO DO: falta la inicilizacion de valores default
// TO DO: falta funcion de escritura externa al modulo
// TO DO: falta funcion de inicializacion de nvs_flash externa al modulo
void dataflash_manager_init(void)
{
    dataflash_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(dataflash_event_t));
    dataflash_response_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(dataflash_event_t));

    xTaskCreate(dataflash_manager_task, "dataflash_manager_task", 
        configMINIMAL_STACK_SIZE*4, NULL, configMAX_PRIORITIES-2, NULL);
}
//------------------------------------------------------------------------------
void read_variable_from_flash(const char *key) 
{
    dataflash_event_t ev;
    ev.cmd = READ_FLASH;
    strncpy(ev.operation_info.key, key, MAX_VALUE_LENGTH);
    if (xQueueSend(dataflash_manager_queue, &ev, 0) != pdPASS) 
    {
        printf("Error al enviar la operación de lectura a la cola\n");
        return;
    }
}
//------------------------------------------------------------------------------
void wait_for_flash_response(char *value) 
{
    dataflash_event_t ev;
    if (xQueueReceive(dataflash_response_queue, &ev, portMAX_DELAY)) 
    {
        strncpy(value, ev.operation_info.value, MAX_VALUE_LENGTH);
    } 
    else 
    {
        printf("Error al recibir la respuesta de la tarea de flash\n");
    }
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------