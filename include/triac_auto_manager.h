#ifndef TRIAC_AUTO_MANAGER_H__
#define TRIAC_AUTO_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <time.h>
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------
#define MAX_AUTO_TRIAC_CONFIGS_HOURS 4
#define TRIAC_CALENDAR_1 0
#define TRIAC_CALENDAR_2 1
#define TRIAC_CALENDAR_3 2
#define TRIAC_CALENDAR_4 3
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    TRIAC_OUTPUT_OFF = 0,
    TRIAC_OUTPUT_ON = 1,
}triac_output_status_t;

typedef struct{
    struct tm turn_on_time;
    struct tm turn_off_time;    
    uint8_t enable;
}triac_config_info_t;

typedef struct{
    struct tm current_time;
    triac_output_status_t output_status;
    triac_config_info_t triac_auto[MAX_AUTO_TRIAC_CONFIGS_HOURS];
}triac_auto_info_t;
//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void triac_auto_manager_handler(triac_auto_info_t *info, bool triac_auto_enable);
uint8_t check_turn_off_triac(triac_auto_info_t *info);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* TRIAC_AUTO_MANAGER_H__ */