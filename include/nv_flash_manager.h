#ifndef NV_FLASH_MANAGER_H__
#define NV_FLASH_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <stdint.h>

#include "../include/nv_flash_driver.h"
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------
// FLASH KEYs
// Tamaño maximo de key 15 caracteres
#define DEVICE_ALIAS_KEY "alias_key"
#define WIFI_AP_SSID_KEY "ssid_ap_key"
#define PWM_MODE_KEY "pwm_mode_key" 
#define SIMUL_DAY_STATUS_KEY "sim_day_key"
#define PWM_PERCENT_POWER_KEY "pwm_power_key"
#define TRIAC_MODE_KEY "triac_mod_key" 
#define PWM_DATE_ON_KEY "pwm_on_key" 
#define PWM_DATE_OFF_KEY "pwm_off_key"
#define TRIAC1_DATE_ON_KEY "triac1_on_key" 
#define TRIAC1_DATE_OFF_KEY "triac1_off_key"
#define TRIAC2_DATE_ON_KEY "triac2_on_key" 
#define TRIAC2_DATE_OFF_KEY "triac2_off_key"



// DEFAULT VALUES
#define DEVICE_ALIAS_DEFAULT "GreenGrowTech"
#define WIFI_AP_SSID_DEFAULT "GreenGrowTech" // el tamaño maximo del ssid no debe exceder los 32 caracteres
#define PWM_MODE_DEFAULT 2 // MANUAL_OFF
#define PWM_SIMUL_DAY_STATUS_DEFAULT 0 // SIMUL_DAY_OFF
#define PWM_PERCENT_POWER_DEFAULT 50 // 50 percent
#define TRIAC_MODE_DEFAULT 2 // MANUAL_OFF
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void nv_flash_manager_init(void);

void init_date_parameter_in_flash(char *key, struct tm time_info_default);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* NV_FLASH_MANAGER_H__ */