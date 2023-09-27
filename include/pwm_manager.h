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

/// @brief Inits pwm module. Must be call in main function.
/// @param  
void pwm_manager_init(void);
void pwm_manager_turn_on_pwm(uint8_t pwm_power_percent);
void pwm_manager_turn_off_pwm(void);
void pwm_manager_update_pwm(uint8_t pwm_power_percent);
/// @brief Turn off pwm signal to duty cycle pwm_power_percent in 15 min with fading.
/// @param pwm_power_percent duty cycle to be config.
void pwm_manager_turn_on_pwm_simul_day_on(uint8_t pwm_power_percent);
/// @brief Turn off pwm signal to duty cycle 0 in 15 min with fading.
/// @param  
void pwm_manager_turn_off_pwm_simul_day_on(void);
uint8_t is_fading_in_progress(void);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* LED_MANAGER_H__ */