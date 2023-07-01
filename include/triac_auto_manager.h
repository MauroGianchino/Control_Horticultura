#ifndef TRIAC_AUTO_MANAGER_H__
#define TRIAC_AUTO_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <time.h>
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------
#define MAX_AUTO_TRIAC_CONFIGS_HOURS 4
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef struct{
    time_t turn_on_time;
    time_t turn_off_time;
    uint8_t enable;
}triac_config_info_t;

typedef struct{
    time_t current_time;
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