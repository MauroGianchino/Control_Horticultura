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
#define PWM_BUTTON GPIO_NUM_35
#define TRIAC_BUTTON GPIO_NUM_26
#define VEGE_STATUS_BUTTON GPIO_NUM_27
#define SIMUL_POTE_POSITIVE_BUTTON GPIO_NUM_33
#define SIMUL_POTE_NEGATIVE_BUTTON GPIO_NUM_32
// DIGITAL OUTPUT FOR PERIPHERALS
#define TRIAC_OUTPUT GPIO_NUM_25
#define RELE_OUTPUT GPIO_NUM_14
//#define ADC_POTE_INPUT 
//#define PWM_OUTPUT 
// I2C
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22
//#define DISPLAY_RESET 
//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------

//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* BOARD_DEF_H__ */