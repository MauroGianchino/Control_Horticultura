#ifndef PWM_AUTO_MANAGER_H__
#define PWM_AUTO_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <time.h>
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------

//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef struct{
    time_t current_time;
    time_t pwm_auto_turn_on_time;
    time_t pwm_auto_turn_off_time;
    uint8_t simul_day_status;
    uint8_t pwm_percent_power;
}pwm_auto_info_t;
//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------

void pwm_auto_start(void);
void pwm_auto_end(void);
void pwm_auto_manager_handler(pwm_auto_info_t *info);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* PWM_AUTO_MANAGER_H__ */