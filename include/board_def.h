#ifndef BOARD_DEF_H__
#define BOARD_DEF_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------

//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------
#define ANALOG_POTE

#ifndef ANALOG_POTE
    #define DIGITAL_POTE 
#endif

#define MAX_PERCENTAGE_POWER_VALUE 100
#define MIN_PERCENTAGE_POWER_VALUE 0

// LED DEFINITIONS
#define DEVICE_ON_LED GPIO_NUM_19 
#define DEVICE_MODE_LED GPIO_NUM_4
#define TRIAC_OUTPUT_STATUS_LED GPIO_NUM_5
#define RELE_VEGE_STATUS_LED GPIO_NUM_18
#define PWM_LED_RED GPIO_NUM_2
#define PWM_LED_GREEN GPIO_NUM_15
// BUTTON DEFINITIONS
#define WIFI_MODE_BUTTON GPIO_NUM_34
#define DEVICE_MODE_BUTTON GPIO_NUM_35 // manual o automatico ex PWM_BUTTON
//#define TRIAC_BUTTON GPIO_NUM_26 // rele 
#define VEGE_BUTTON GPIO_NUM_25 // boton de rele
//#define SIMUL_POTE_POS_BUTTON GPIO_NUM_33
#define SIMUL_POTE_NEG_BUTTON GPIO_NUM_32
// DIGITAL OUTPUT FOR PERIPHERALS
#define TRIAC_OUTPUT GPIO_NUM_27
//#define RELE_OUTPUT GPIO_NUM_14 // boton 220 TRIAC_BUTTON
#define ADC_POTE_INPUT ADC_CHANNEL_5 // GPIO_NUM_12  IMPORTANTE: must be LOW during boot
#define PWM_OUTPUT GPIO_NUM_23
// I2C
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

#define DISPLAY_RESET GPIO_NUM_13

#define TRIAC_BUTTON GPIO_NUM_14
#define RELE_OUTPUT GPIO_NUM_26


//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------

//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* BOARD_DEF_H__ */