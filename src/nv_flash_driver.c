//--------------------INCLUDES--------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "freertos/queue.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "../include/nv_flash_driver.h"
#include "../include/board_def.h"
#include "../include/global_manager.h"
//--------------------MACROS Y DEFINES------------------------------------------
//------------------------------------------------------------------------------
#define QUEUE_ELEMENT_QUANTITY 30

#define NVS_NAMESPACE "device"
#define MAX_VALUE_LENGTH 80
#define MAX_KEY_LENGTH 15


#define TIMEOUT_MS 500

#define DEBUG_MODULE
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    CMD_UNDEFINED = 0,
    INIT_FLASH = 1,
    ERASE_FLASH = 2,
    INIT_PARAMETER_IN_FLASH_STR = 3,
    READ_FLASH_STR = 4,
    WRITE_FLASH_STR = 5,
    INIT_PARAMETER_IN_FLASH_UINT32 = 6,
    READ_FLASH_UINT32 = 7,
    WRITE_FLASH_UINT32 = 8
}dataflash_event_cmds_t;

typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    uint32_t value_uint32;
    uint32_t default_value_uint32;
    char default_value[MAX_VALUE_LENGTH];
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
    uint32_t value_aux= 0;
    char value[MAX_VALUE_LENGTH];
    size_t len = sizeof(value);


    while(1)
    {
        if(xQueueReceive(dataflash_manager_queue, &ev, portMAX_DELAY) == pdTRUE)
        {
            switch(ev.cmd)
            {
                case CMD_UNDEFINED:
                    #ifdef DEBUG_MODULE
                        printf("Operación no válida\n");
                    #endif
                    break;
                case ERASE_FLASH:
                    nvs_flash_erase();
                    nvs_flash_init();
                    break;
                case INIT_FLASH:
                    ret = nvs_flash_init();
                    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
                    {
                        ESP_ERROR_CHECK(nvs_flash_erase());
                        ret = nvs_flash_init();
                    }
                    if(ret != ESP_OK) 
                    {
                        #ifdef DEBUG_MODULE
                            printf("Error al inicializar el subsistema NVS\n");
                        #endif
                    } 
                    else 
                    {
                        #ifdef DEBUG_MODULE
                            printf("Subsistema NVS inicializado correctamente\n");
                        #endif
                    }
                    break;
                case INIT_PARAMETER_IN_FLASH_STR:
                    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
                    if(ret != ESP_OK) 
                    {
                        #ifdef DEBUG_MODULE
                            printf("Error al abrir el espacio de almacenamiento NVS\n");
                        #endif
                    } 
                    else 
                    {
                        memset(value, '\0', sizeof(value));
                        ret = nvs_get_str(nvs_handle, ev.operation_info.key, value, &len);

                        if(ret == ESP_OK) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Valor de %s: %s\n", ev.operation_info.key, value);
                            #endif
                        } 
                        else if(ret == ESP_ERR_NVS_NOT_FOUND) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Clave %s no encontrada\n", ev.operation_info.key);
                            #endif
                            
                            ret = nvs_set_str(nvs_handle, ev.operation_info.key, (const char*)ev.operation_info.default_value);
                            #ifdef DEBUG_MODULE
                                printf((ret != ESP_OK) ? "FAILED!\n" : "DONE\n");
                                printf("SAVING UPDATES IN NVS ... ");
                            #endif
                            ret = nvs_commit(nvs_handle);
                            #ifdef DEBUG_MODULE    
                                printf((ret != ESP_OK) ? "FAILED!\n" : "DONE\n");
                            #endif
                        } 
                        else 
                        {
                            printf("%d",(int)ret);
                            #ifdef DEBUG_MODULE
                                printf("Error al leer del NVS\n");
                            #endif
                        }
                        nvs_close(nvs_handle);
                    }
                    break;
                case READ_FLASH_STR:
                    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
                    if(ret != ESP_OK) 
                    {
                        #ifdef DEBUG_MODULE
                            printf("Error al abrir el espacio de almacenamiento NVS\n");
                        #endif
                    } 
                    else 
                    {
                        memset(value, '\0', sizeof(value));
                        ret = nvs_get_str(nvs_handle, ev.operation_info.key, value, &len);
                        if(ret == ESP_OK) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Valor de %s: %s\n", ev.operation_info.key, value);
                            #endif
                            strncpy(ev.operation_info.value, value, MAX_VALUE_LENGTH);
                            xQueueSend(dataflash_response_queue, &ev, 10);  // Envía la respuesta a la cola de respuesta
                        } 
                        else if(ret == ESP_ERR_NVS_NOT_FOUND) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Clave %s no encontrada\n", ev.operation_info.key);
                            #endif
                        }                          
                        else 
                        {
                            #ifdef DEBUG_MODULE
                                printf("%d",(int)ret);
                                printf("Error al leer del NVS\n");
                            #endif
                        }
                        nvs_close(nvs_handle);
                    }
                    break;
                case WRITE_FLASH_STR:
                    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
                    if(ret != ESP_OK) 
                    {
                        #ifdef DEBUG_MODULE
                            printf("Error al abrir el espacio de almacenamiento NVS\n");
                        #endif
                    } 
                    else 
                    {
                        ret = nvs_set_str(nvs_handle, ev.operation_info.key, ev.operation_info.value);
                        if(ret == ESP_OK) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Valor %s guardado correctamente para la clave %s\n", ev.operation_info.value, ev.operation_info.key);
                            #endif
                            ret = nvs_commit(nvs_handle);
                            #ifdef DEBUG_MODULE    
                                printf((ret != ESP_OK) ? "FAILED!\n" : "DONE\n");
                            #endif
                        } 
                        else 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Error al escribir en el NVS\n");
                            #endif
                        }
                        nvs_close(nvs_handle);
                    }
                    break;
                case INIT_PARAMETER_IN_FLASH_UINT32:
                    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
                    if(ret != ESP_OK) 
                    {
                        #ifdef DEBUG_MODULE
                            printf("Error al abrir el espacio de almacenamiento NVS\n");
                        #endif
                    } 
                    else 
                    {
                        ret = nvs_get_u32(nvs_handle, ev.operation_info.key, &value_aux);
                        if(ret == ESP_OK) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Valor de %s: %d\n", ev.operation_info.key, (int)value_aux);
                            #endif
                        } 
                        else if(ret == ESP_ERR_NVS_NOT_FOUND) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Clave %s no encontrada\n", ev.operation_info.key);
                            #endif
                            ret = nvs_set_u32(nvs_handle, ev.operation_info.key, ev.operation_info.default_value_uint32);
                            #ifdef DEBUG_MODULE
                                printf((ret != ESP_OK) ? "FAILED!\n" : "DONE\n");
                                printf("SAVING UPDATES IN NVS ... ");
                            #endif
                            ret = nvs_commit(nvs_handle);
                            #ifdef DEBUG_MODULE    
                                printf((ret != ESP_OK) ? "FAILED!\n" : "DONE\n");
                            #endif
                        } 
                        else 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Error al leer del NVS\n");
                            #endif
                        }
                        nvs_close(nvs_handle);
                    }
                    break;
                case READ_FLASH_UINT32:
                    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
                    if(ret != ESP_OK) 
                    {
                        #ifdef DEBUG_MODULE
                            printf("Error al abrir el espacio de almacenamiento NVS\n");
                        #endif
                    } 
                    else 
                    {
                        ret = nvs_get_u32(nvs_handle, ev.operation_info.key, &value_aux);
                        if(ret == ESP_OK) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Valor de %s: %d\n", ev.operation_info.key, (int)value_aux);
                            #endif
                            ev.operation_info.value_uint32 = value_aux;
                            xQueueSend(dataflash_response_queue, &ev, 0);  // Envía la respuesta a la cola de respuesta
                        } 
                        else if(ret == ESP_ERR_NVS_NOT_FOUND) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Clave %s no encontrada\n", ev.operation_info.key);
                            #endif
                        } 
                        else 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Error al leer del NVS\n");
                            #endif
                        }
                        nvs_close(nvs_handle);
                    }
                    break;
                case WRITE_FLASH_UINT32:
                    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
                    if(ret != ESP_OK) 
                    {
                        #ifdef DEBUG_MODULE
                            printf("Error al abrir el espacio de almacenamiento NVS\n");
                        #endif
                    } 
                    else 
                    {
                        ret = nvs_set_u32(nvs_handle, ev.operation_info.key, ev.operation_info.value_uint32);
                        if(ret == ESP_OK) 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Valor %d guardado correctamente para la clave %s\n", (int)ev.operation_info.value_uint32, ev.operation_info.key);
                            #endif
                            ret = nvs_commit(nvs_handle);
                            #ifdef DEBUG_MODULE    
                                printf((ret != ESP_OK) ? "FAILED!\n" : "DONE\n");
                            #endif
                        } 
                        else 
                        {
                            #ifdef DEBUG_MODULE
                                printf("Error al escribir en el NVS\n");
                            #endif
                        }
                        nvs_close(nvs_handle);
                    }
                    break;
                default:
                    #ifdef DEBUG_MODULE
                        printf("Operación no válida\n");
                    #endif
                    break;
            }       
        }
    }
}
//------------------- DEFINICION DE FUNCIONES EXTERNAS -------------------------
//------------------------------------------------------------------------------
void nv_flash_driver_init(void)
{
    dataflash_manager_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(dataflash_event_t));
    dataflash_response_queue = xQueueCreate(QUEUE_ELEMENT_QUANTITY, sizeof(dataflash_event_t));

    xTaskCreate(dataflash_manager_task, "dataflash_manager_task", 
        configMINIMAL_STACK_SIZE*8, NULL, configMAX_PRIORITIES-2, NULL);
}
//------------------------------------------------------------------------------
void nv_flash_driver_install_flash(void)
{
    dataflash_event_t ev;
    ev.cmd = INIT_FLASH;

    xQueueSend(dataflash_manager_queue, &ev, 0);
}
//------------------------------------------------------------------------------
void nv_flash_driver_erase_flash(void)
{
    dataflash_event_t ev;
    ev.cmd = ERASE_FLASH;

    xQueueSend(dataflash_manager_queue, &ev, 0);
}
//------------------------------------------------------------------------------
void init_parameter_in_flash_str(const char *key, char *default_value) 
{
    dataflash_event_t ev;
    ev.cmd = INIT_PARAMETER_IN_FLASH_STR;

    memset(ev.operation_info.default_value, '\0', sizeof(ev.operation_info.default_value));
    memset(ev.operation_info.key, '\0', sizeof(ev.operation_info.key));

    strcpy(ev.operation_info.key, key);
    strcpy(ev.operation_info.default_value, default_value);

    if(xQueueSend(dataflash_manager_queue, &ev, 10) != pdPASS) 
    {
        #ifdef DEBUG_MODULE
            printf("Error al enviar la operación de lectura a la cola\n");
        #endif
        return;
    }
}
//------------------------------------------------------------------------------
void write_parameter_on_flash_str(const char *key, char *value) 
{
    dataflash_event_t ev;
    ev.cmd = WRITE_FLASH_STR;

    memset(ev.operation_info.default_value, '\0', sizeof(ev.operation_info.default_value));
    memset(ev.operation_info.key, '\0', sizeof(ev.operation_info.key));

    strcpy(ev.operation_info.key, key);
    strcpy(ev.operation_info.value, value);

    if(xQueueSend(dataflash_manager_queue, &ev, 0) != pdPASS) 
    {
        #ifdef DEBUG_MODULE
            printf("Error al enviar la operación de lectura a la cola\n");
        #endif
        return;
    }
}
//------------------------------------------------------------------------------
void read_parameter_from_flash_str(const char *key) 
{
    dataflash_event_t ev;
    ev.cmd = READ_FLASH_STR;
    memset(ev.operation_info.key, '\0', sizeof(ev.operation_info.key));
    strcpy(ev.operation_info.key, key);
    if(xQueueSend(dataflash_manager_queue, &ev, 10) != pdPASS) 
    {
        #ifdef DEBUG_MODULE
            printf("Error al enviar la operación de lectura a la cola\n");
        #endif
        return;
    }
}
//------------------------------------------------------------------------------
uint8_t wait_for_flash_response_str(char *value) 
{
    dataflash_event_t ev;
    memset(ev.operation_info.value, '\0', sizeof(ev.operation_info.value));
    if(xQueueReceive(dataflash_response_queue, &ev, TIMEOUT_MS / portTICK_PERIOD_MS)) 
    {
        strncpy(value, ev.operation_info.value, strlen(ev.operation_info.value));
        return 1;
    } 
    else 
    {
        #ifdef DEBUG_MODULE
            printf("Error al recibir la respuesta de la tarea de flash\n");
        #endif
        return 0;
    }
}
//------------------------------------------------------------------------------
void init_parameter_in_flash_uint32(const char *key, uint32_t default_value) 
{
    dataflash_event_t ev;
    ev.cmd = INIT_PARAMETER_IN_FLASH_UINT32;
    strcpy(ev.operation_info.key, key);
    ev.operation_info.default_value_uint32 =  default_value;
    if(xQueueSend(dataflash_manager_queue, &ev, 0) != pdPASS) 
    {
        #ifdef DEBUG_MODULE
            printf("Error al enviar la operación de lectura a la cola\n");
        #endif
        return;
    }
}
//------------------------------------------------------------------------------
void read_parameter_from_flash_uint32(const char *key) 
{
    dataflash_event_t ev;
    ev.cmd = READ_FLASH_UINT32;
    strcpy(ev.operation_info.key, key);
    if(xQueueSend(dataflash_manager_queue, &ev, 0) != pdPASS) 
    {
        #ifdef DEBUG_MODULE
            printf("Error al enviar la operación de lectura a la cola\n");
        #endif
        return;
    }
}
//------------------------------------------------------------------------------
uint8_t wait_for_flash_response_uint32(uint32_t *value) 
{
    dataflash_event_t ev;
    if(xQueueReceive(dataflash_response_queue, &ev, TIMEOUT_MS / portTICK_PERIOD_MS)) 
    {
        *value = ev.operation_info.value_uint32;
        return 1;
    } 
    else 
    {
        #ifdef DEBUG_MODULE
            printf("Error al recibir la respuesta de la tarea de flash\n");
        #endif
        return 0;
    }
}
//------------------------------------------------------------------------------
void write_parameter_on_flash_uint32(const char *key, uint32_t value) 
{
    dataflash_event_t ev;
    ev.cmd = WRITE_FLASH_UINT32;
    strcpy(ev.operation_info.key, key);
    ev.operation_info.value_uint32 = value;

    if(xQueueSend(dataflash_manager_queue, &ev, 10) != pdPASS) 
    {
        #ifdef DEBUG_MODULE
            printf("Error al enviar la operación de lectura a la cola\n");
        #endif
        return;
    }
}
//---------------------------- END OF FILE -------------------------------------
//------------------------------------------------------------------------------