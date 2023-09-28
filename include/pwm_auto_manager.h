#ifndef PWM_AUTO_MANAGER_H__
#define PWM_AUTO_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <time.h>
#include <stdbool.h>
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------

//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    SIMUL_DAY_OFF = 0,
    SIMUL_DAY_ON = 1,
}simul_day_status_t;

typedef enum{
    PWM_OUTPUT_OFF = 0,
    PWM_OUTPUT_ON = 1,
}pwm_output_status_t;

typedef struct{
    struct tm current_time;
    pwm_output_status_t output_status;
    bool update_output_percent_power;
    struct tm turn_on_time;
    struct tm turn_off_time;
    simul_day_status_t simul_day_status;
    uint8_t percent_power;
}pwm_auto_info_t;
//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void pwm_auto_manager_handler(pwm_auto_info_t *info, bool pwm_auto_enable);
void turn_off_fading_status(void);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* PWM_AUTO_MANAGER_H__ */