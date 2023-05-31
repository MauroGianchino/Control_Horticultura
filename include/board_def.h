#ifndef BOARD_DEF_H__
#define BOARD_DEF_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------

//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------
// LED DEFINITIONS
#define DEVICE_ON_LED GPIO_NUM_2 
#define PWM_OUTPUT_STATUS_LED GPIO_NUM_4
#define TRIAC_OUTPUT_STATUS_LED GPIO_NUM_5
#define RELE_VEGE_STATUS_LED GPIO_NUM_18
#define WIFI_STATUS_1_LED GPIO_NUM_19
#define WIFI_STATUS_2_LED GPIO_NUM_17
// BUTTON DEFINITIONS
#define WIFI_MODE_BUTTON GPIO_NUM_34
#define PWM_STATUS_BUTTON GPIO_NUM_35
#define TRIAC_STATUS_BUTTON GPIO_NUM_26
#define LED_VEGE_STATUS_BUTTON GPIO_NUM_27
#define SIMUL_POTE_POSITIVE_STATUS_BUTTON GPIO_NUM_33
#define SIMUL_POTE_NEGATIVE_STATUS_BUTTON GPIO_NUM_32
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------

//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* BOARD_DEF_H__ */