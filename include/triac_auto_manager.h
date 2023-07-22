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
typedef struct{
    struct tm turn_on_time;
    struct tm turn_off_time;
    uint8_t enable;
}triac_config_info_t;

typedef struct{
    struct tm current_time;
    triac_config_info_t triac_auto[MAX_AUTO_TRIAC_CONFIGS_HOURS];
}triac_auto_info_t;
//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void triac_auto_start(void);
void triac_auto_end(void);
void triac_auto_manager_handler(triac_auto_info_t *info);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* TRIAC_AUTO_MANAGER_H__ */