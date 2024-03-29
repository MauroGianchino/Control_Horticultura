#ifndef GLOBAL_MANAGER_H__
#define GLOBAL_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <stdint.h>
#include <time.h>

#include "../include/pwm_auto_manager.h"
#include "../include/triac_auto_manager.h"
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------
#define MAX_TRIAC_CALENDARS 4
#define DEVICE_SSID_MAX_LENGTH 50
#define DEVICE_PASS_MAX_LENGTH 50
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------
typedef enum{
    UNDEFINED = 0,
    MANUAL_ON = 1,
    MANUAL_OFF = 2,
    AUTOMATIC = 3,
}output_mode_t;

typedef enum{
    RELE_VEGE_ENABLE = 0,
    RELE_VEGE_DISABLE = 1,
}rele_output_status_t;


typedef struct{
    struct tm turn_on_time;
    struct tm turn_off_time;
}calendar_auto_mode_t;

typedef struct{
    char wifi_ssid[DEVICE_SSID_MAX_LENGTH];
    char wifi_password[DEVICE_PASS_MAX_LENGTH];
    output_mode_t pwm_mode;
    pwm_auto_info_t pwm_auto;
    uint8_t pwm_manual_percent_power;
    output_mode_t triac_mode;
    triac_auto_info_t triac_auto;
    rele_output_status_t rele_vege_status;
    uint16_t max_pote_reference;
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
void global_manager_set_triac_mode_off(bool read_from_flash);
void global_manager_set_triac_mode_manual_on(bool read_from_flash);
void global_manager_set_triac_mode_auto(bool read_from_flash);
void global_manager_set_rele_vege_status_off(bool read_from_flash);
void global_manager_set_rele_vege_status_on(bool read_from_flash);
void global_manager_set_pwm_power_value_manual(uint8_t power_percentage_value);
void global_manager_set_pwm_power_value_auto(uint8_t power_percentage_value, bool read_from_flash);
void global_manager_update_current_time(struct tm current_time);
void global_manager_update_simul_day_function_status(simul_day_status_t status, bool read_from_flash);
void global_manager_update_auto_pwm_calendar(calendar_auto_mode_t calendar, bool read_from_flash);
void global_manager_update_auto_triac_calendar(triac_config_info_t triac_info, uint8_t triac_num, bool read_from_flash);
uint8_t global_manager_set_wifi_ssid(char* wifi_ssid, bool read_from_flash);
uint8_t global_manager_set_wifi_password(char* wifi_password, bool read_from_flash);
void global_manager_set_max_pote_reference(uint16_t max_pote_reference, bool read_from_flash);

uint8_t global_manager_get_net_info(char *ssid, char *password);
uint8_t global_manager_get_pwm_info(output_mode_t *pwm_mode, pwm_auto_info_t *pwm_auto);
uint8_t global_manager_get_triac_info(output_mode_t *triac_mode, triac_auto_info_t *triac_auto);
uint8_t global_manager_get_rele_vege_info(rele_output_status_t *rele_vege_status);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* GLOBAL_MANAGER_H__ */