#ifndef PWM_MANAGER_H__
#define PWM_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------

//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------

//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void pwm_manager_init(void);

void pwm_manager_turn_on_pwm(uint8_t pwm_power_percent);
void pwm_manager_turn_off_pwm(void);
void pwm_manager_update_pwm(uint8_t pwm_power_percent);
void pwm_manager_turn_on_pwm_simul_day_on(uint8_t pwm_power_percent);
void pwm_manager_turn_off_pwm_simul_day_on(void);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* LED_MANAGER_H__ */