#ifndef GLOBAL_MANAGER_H__
#define GLOBAL_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <stdint.h>
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------

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
    output_mode_t pwm_mode;
    output_mode_t triac_mode;
    rele_output_status_t rele_vege_status;
}nv_info_t;
//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void global_manager_init(void);

void global_manager_set_pwm_mode_off(void);
void global_manager_set_pwm_mode_manual_on(void);
void global_manager_set_pwm_mode_auto(void);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* GLOBAL_MANAGER_H__ */