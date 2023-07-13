#ifndef GLOBAL_MANAGER_H__
#define GLOBAL_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <stdint.h>
#include <time.h>

#include "../include/pwm_auto_manager.h"
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------
#define MAX_TRIAC_CALENDARS 4
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    UNDEFINED = 0,
    MANUAL_ON = 1,
    MANUAL_OFF = 2,
    AUTOMATIC = 3,
}output_mode_t;

typedef enum{
    RELE_ON = 0,
    RELE_OFF = 1,
}rele_output_status_t;


typedef struct{
    struct tm turn_on_time;
    struct tm turn_off_time;
}calendar_auto_mode_t;

typedef struct{
    output_mode_t pwm_mode;
    pwm_auto_info_t pwm_auto;
    uint8_t pwm_manual_percent_power;
    output_mode_t triac_mode;
    rele_output_status_t rele_vege_status;
    calendar_auto_mode_t triac_auto_calendar[MAX_TRIAC_CALENDARS];
}nv_info_t;
//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void global_manager_init(void);

void global_manager_set_pwm_mode(output_mode_t pwm_mode);
void global_manager_set_pwm_mode_off(void);
void global_manager_set_pwm_mode_manual_on(void);
void global_manager_set_pwm_mode_auto(void);
void global_manager_set_triac_mode_off(void);
void global_manager_set_pwm_triac_manual_on(void);
void global_manager_set_triac_mode_auto(void);
void global_manager_set_rele_vege_status_off(void);
void global_manager_set_rele_vege_status_on(void);
void global_manager_set_pwm_power_value_manual(uint8_t power_percentage_value);
void global_manager_update_current_time(struct tm current_time);
void global_manager_update_simul_day_function_status(simul_day_status_t status);
void global_manager_update_auto_pwm_calendar(calendar_auto_mode_t calendar);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* GLOBAL_MANAGER_H__ */