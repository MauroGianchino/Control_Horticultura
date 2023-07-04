#ifndef DATAFLASH_MANAGER_H__
#define DATAFLASH_MANAGER_H__
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
void dataflash_manager_init(void);
void read_variable_from_flash(const char *key);
void wait_for_flash_response(char *value);

/*
Ejemplo de lectura de una variable de dataflash:

char value[MAX_VALUE_LENGTH];
read_variable_from_flash("mi_variable");
wait_for_flash_response(value);
printf("Valor de mi_variable: %s\n", value);
*/
//------------------- FIN DEL ARCHIVO ------------------------------------------
#endif /* DATAFLASH_MANAGER_H__ */