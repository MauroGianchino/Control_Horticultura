#ifndef ANALOG_INPUT_MANAGER_H__
#define ANALOG_INPUT_MANAGER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include "../include/global_manager.h"
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------

//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void analog_input_manager_init(void);
void analog_input_send_pwm_mode(output_mode_t pwm_mode);
void analog_input_set_max_pote_reference(uint16_t max_pote_reference);
void analog_input_calibrate_pote(void);
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* ANALOG_INPUT_MANAGER_H__ */