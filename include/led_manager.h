#ifndef LED_MANAGER_H__
#define LED_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include "../include/pwm_auto_manager.h"
#include "../include/global_manager.h"
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------

//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void led_manager_init(void);

void led_manager_power_up(void);
void led_manager_triac_on(void);
void led_manager_triac_off(void);
void led_manager_rele_vege_on(void);
void led_manager_rele_vege_off(void);

void led_manager_set_device_mode_auto(void);
void led_manager_set_device_mode_manual(void);

void led_manager_send_pwm_info(uint8_t duty_cycle, uint8_t is_simul_day_working, simul_day_status_t simul_day_status);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* LED_MANAGER_H__ */