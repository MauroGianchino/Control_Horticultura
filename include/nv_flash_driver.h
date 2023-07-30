#ifndef NV_FLASH_DRIVER_H__
#define NV_FLASH_DRIVER_H__
//------------------- INCLUDES -------------------------------------------------
//------------------------------------------------------------------------------
#include <stdint.h>
//------------------- MACROS Y DEFINES -----------------------------------------
//------------------------------------------------------------------------------

//------------------- TYPEDEF --------------------------------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE DATOS EXTERNOS ----------------------------
//------------------------------------------------------------------------------

//------------------- DECLARACION DE FUNCIONES EXTERNAS ------------------------
//------------------------------------------------------------------------------
void nv_flash_driver_init(void);

void nv_flash_driver_install_flash(void);
void nv_flash_driver_erase_flash(void);

void init_parameter_in_flash_str(const char *key, char *default_value);
void read_parameter_from_flash_str(const char *key);
uint8_t wait_for_flash_response_str(char *value); 
void write_parameter_on_flash_str(const char *key, char *value);  

void init_parameter_in_flash_uint32(const char *key, uint32_t default_value); 
void read_parameter_from_flash_uint32(const char *key);
uint8_t wait_for_flash_response_uint32(uint32_t *value); 
void write_parameter_on_flash_uint32(const char *key, uint32_t value);

/*
Ejemplo de lectura de una variable de dataflash:

char value[MAX_VALUE_LENGTH];
read_variable_from_flash("mi_variable", "valor_default");
wait_for_flash_response(value);
printf("Valor de mi_variable: %s\n", value);
*/
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* NV_FLASH_DRIVER_H__ */