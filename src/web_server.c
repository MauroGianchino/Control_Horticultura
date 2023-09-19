#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include <esp_wifi.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "web_server.h"
#include "cJSON.h"
#include "../include/version.h"
#include "../include/current_time_manager.h"
#include "../include/global_manager.h"
#include <time.h>

static const char *TAG = "WEBSERVER";
static const char *PWM = "PWM";
static const char *TRIAC = "TRIAC";
static const char *VEGEFLOR = "VEGEFLOR";
static const char *VERSIONN = "VERSION";
static const char *HORA = "HORA";

//---------------RED-------------------

red_t red; // variable para leer el ssid y pass de la red

//---------------Hora-------------------

struct tm aux_hora; // variable para setear y leer la hora actual

//---------------PWM------------------

calendar_auto_mode_t pwm_hora; // variable para setear la hora del pwm

simul_day_status_t dia; // variable para setear la simulacion dia del PWM

output_mode_t modo_pwm; // variable setear y leer el modo del pwm

pwm_auto_info_t pwm_info; // variable con la info para leer el pwm

//---------------TRIAC------------------

output_mode_t modo_triac; // variable para setear y leer el modo del triac

calendar_auto_mode_t triac_h; // variable para setear el modo del triac

triac_config_info_t triac_h1; // variable para setear los horarios del triac

triac_config_info_t triac_h2; // variable para setear los horarios del triac

triac_config_info_t triac_h3; // variable para setear los horarios del triac

triac_config_info_t triac_h4; // variable para setear los horarios del triac

triac_auto_info_t triac_auto_info; // variable para leer toda la data del triac (y los 4 horarios)

//---------------VEGEFLOR------------------

rele_output_status_t rele_status; // variable para leer el estado del relé

//----------FUNCIONES------------//

void reset_triac_h(triac_config_info_t *triac_h)
{
    triac_h->enable = 0;
    triac_h->turn_off_time.tm_hour = 0;
    triac_h->turn_off_time.tm_min = 0;
    triac_h->turn_on_time.tm_hour = 0;
    triac_h->turn_on_time.tm_min = 0;
}

void init_red(red_t *red)
{
    memset(red->ID, '\0', sizeof(red->ID));
    strcpy(red->ID, "-");
    memset(red->PASS, '\0', sizeof(red->PASS));
    strcpy(red->PASS, "-");
}

void print_red(red_t *red)
{

    ESP_LOGW(TAG, "ID:%s", red->ID);

    ESP_LOGW(TAG, "PASS:%s", red->PASS);
}

void analyze_token_pwm(char *token)
{
    int dh, dm; // unidades y decenas de horas y minutos
    uint8_t inten;
    switch (token[0])
    {
    case 'r': // Parseo intensidad
        ESP_LOGI(PWM, "%d", strlen(token));
        if (strlen(token) == 7) // caso de que sea un numero de un solo digito
        {
            inten = (uint8_t)atoi(&token[6]);
            global_manager_set_pwm_power_value_auto(inten, pdFALSE);
        }
        else if (strlen(token) == 8) // caso de un numero de dos digitos
        {
            dh = atoi(&token[6]);
            inten = (uint8_t)atoi(&token[6]);
            ESP_LOGI(TAG, "%d", dh);
            global_manager_set_pwm_power_value_auto(inten, pdFALSE);
        }
        else if (strlen(token) == 9) // caso 100
        {
            global_manager_set_pwm_power_value_auto(100, pdFALSE);
        }
        else
        {
            ESP_LOGE(PWM, "Error en parseo del RANGO");
        }

        break;

    case 'm':
        if (token[10] == 'A')
        {
            global_manager_set_pwm_mode_auto();
            global_manager_set_triac_mode_auto(false);
        }
        else if (token[10] == 'M')
        {
            global_manager_set_pwm_mode_manual_on();
            global_manager_set_triac_mode_off(false);
        }
        else if (token[10] == 'O')
        {
            //global_manager_set_pwm_mode_off();
        }
        else
        {
            ESP_LOGE(PWM, "Erro en parseo del MODO");
        }
        break;
    case 'i':
        dh = atoi(&token[7]);
        dm = atoi(&token[12]);

        pwm_hora.turn_on_time.tm_hour = dh;
        pwm_hora.turn_on_time.tm_min = dm;

        break;
    case 'f':
        dh = atoi(&token[7]);
        dm = atoi(&token[12]);

        pwm_hora.turn_off_time.tm_hour = dh;
        pwm_hora.turn_off_time.tm_min = dm;

        break;
    case 'O':
        if (token[9] == 'S')
        {
            global_manager_update_simul_day_function_status(SIMUL_DAY_ON, pdFALSE);
        }
        else
        {
            global_manager_update_simul_day_function_status(SIMUL_DAY_OFF, pdFALSE);
        }

        break;
    default:
        break;
    }
}

void analyze_token_triac(char *token)
{
    int dh, dm; // unidades y decenas de horas y minutos
    ESP_LOGI(TRIAC, "%d", strlen(token));
    switch (token[0])
    {
    case 'm': // Parseo modo
        if (token[12] == 'E')
        {
            global_manager_set_triac_mode_manual_on(false);
            global_manager_set_pwm_mode_manual_on();
        }
        else if (token[12] == 'A' && token[13] == 'p')
        {
            global_manager_set_triac_mode_off(false);
            global_manager_set_pwm_mode_manual_on();
        }
        else if (token[12] == 'A')
        {
            global_manager_set_triac_mode_auto(false);
            global_manager_set_pwm_mode_auto();
        }
        else
        {
            ESP_LOGE(TRIAC, "Error en parseo del MODO");
        }

        break;
    case 'c':
        if (token[9] == '1')
        {
            ESP_LOGE(TRIAC, "ENTRE EN ENABLE DE H1");
            triac_h1.enable = 1;
        }
        if (token[9] == '2')
        {
            triac_h2.enable = 1;
        }
        if (token[9] == '3')
        {
            triac_h3.enable = 1;
        }
        if (token[9] == '4')
        {
            triac_h4.enable = 1;
        }
        // else
        //{
        //  ESP_LOGE(TRIAC, "Error en parseo del del CHECKBOX");
        //}
        break;

    case 'i':
        dh = atoi(&token[4]);
        dm = atoi(&token[9]);
        if (token[2] == '1')
        {

            triac_h1.turn_on_time.tm_hour = dh;
            triac_h1.turn_on_time.tm_min = dm;
        }
        else if (token[2] == '2')
        {

            triac_h2.turn_on_time.tm_hour = dh;
            triac_h2.turn_on_time.tm_min = dm;
        }
        else if (token[2] == '3')
        {

            triac_h3.turn_on_time.tm_hour = dh;
            triac_h3.turn_on_time.tm_min = dm;
        }
        else if (token[2] == '4')
        {

            triac_h4.turn_on_time.tm_hour = dh;
            triac_h4.turn_on_time.tm_min = dm;
        }
        else
        {
            ESP_LOGE(TRIAC, "Error en parseo del HORARIO INICIAL");
        }
        break;
    case 'f':
        dh = atoi(&token[4]);
        dm = atoi(&token[9]);
        if (token[2] == '1')
        {

            triac_h1.turn_off_time.tm_hour = dh;
            triac_h1.turn_off_time.tm_min = dm;
        }
        else if (token[2] == '2')
        {

            triac_h2.turn_off_time.tm_hour = dh;
            triac_h2.turn_off_time.tm_min = dm;
        }
        else if (token[2] == '3')
        {

            triac_h3.turn_off_time.tm_hour = dh;
            triac_h3.turn_off_time.tm_min = dm;
        }
        else if (token[2] == '4')
        {

            triac_h4.turn_off_time.tm_hour = dh;
            triac_h4.turn_off_time.tm_min = dm;
        }
        else
        {
            ESP_LOGE(TRIAC, "Error en parseo del HORARIO FINAL");
        }

        break;
    default:
        break;
    }
}

void parse_pwm(char *buff)
{
    // el & es el separador de los campos
    ESP_LOGI(PWM, "Testeo del parseo de PWM");
    char delim[2] = "&";
    char *token;
    token = strtok(buff, delim);
    while (token != NULL)
    {
        analyze_token_pwm(token);
        ESP_LOGI(PWM, "%s", token);
        token = strtok(NULL, delim);
    }
    global_manager_update_auto_pwm_calendar(pwm_hora, pdFALSE);
    ESP_LOGI(PWM, "Salgo del parseo");
};

void parse_triac(char *buff)
{
    // el & es el separador de los campos
    ESP_LOGI(TRIAC, "Testeo del parseo de TRIAC");
    int status = global_manager_get_triac_info(&modo_triac, &triac_auto_info);

    triac_auto_info.triac_auto[0].enable = 0;
    triac_auto_info.triac_auto[1].enable = 0;
    triac_auto_info.triac_auto[2].enable = 0;
    triac_auto_info.triac_auto[3].enable = 0;

    triac_h1 = triac_auto_info.triac_auto[0];
    triac_h2 = triac_auto_info.triac_auto[1];
    triac_h3 = triac_auto_info.triac_auto[2];
    triac_h4 = triac_auto_info.triac_auto[3];

    char delim[2] = "&";
    char *token;
    token = strtok(buff, delim);
    while (token != NULL)
    {
        analyze_token_triac(token);
        ESP_LOGI(TRIAC, "%s", token);
        token = strtok(NULL, delim);
    }
    if (triac_h1.enable != 1)
        triac_h1.enable = 0;
    if (triac_h2.enable != 1)
        triac_h2.enable = 0;
    if (triac_h3.enable != 1)
        triac_h3.enable = 0;
    if (triac_h4.enable != 1)
        triac_h4.enable = 0;
    ESP_LOGE(TRIAC, " EL ENABLE DEL 1 ES %d", triac_h1.enable);
    global_manager_update_auto_triac_calendar(triac_h1, 1, false);
    global_manager_update_auto_triac_calendar(triac_h2, 2, false);
    global_manager_update_auto_triac_calendar(triac_h3, 3, false);
    global_manager_update_auto_triac_calendar(triac_h4, 4, false);

    ESP_LOGI(TRIAC, "Salgo del parseo");
}

void parse_vegeflor(char *buff)
{
    // el & es el separador de los campos
    ESP_LOGI(VEGEFLOR, "Testeo del parseo de VEGEFLOR");
    if (buff[14] == 'V')
    {
        global_manager_set_rele_vege_status_on(pdFALSE);
    }
    else if (buff[14] == 'F')
    {
        global_manager_set_rele_vege_status_off(pdFALSE);
    }
    else
    {
        ESP_LOGE(VEGEFLOR, "Error en parseo del MODO");
    }
    ESP_LOGI(VEGEFLOR, "Salgo del parseo");
};

void parse_red(char *buff, red_t *red)
{
    // el & es el separador de los campos
    ESP_LOGI(TAG, "Testeo del parseo de RED");
    uint8_t status = 0;
    char *e;
    int j = 0;
    int len = strlen(buff);
    int index_amp;
    int equalIndex = 6;
    e = strchr(buff, '&');
    index_amp = (int)(e - buff);
    int secondEqualIndex = index_amp + 12;
    ESP_LOGW(TAG, "%d", index_amp);
    for (int i = equalIndex + 1; i < index_amp; i++)
    {
        red->ID[j] = buff[i];
        j++;
    }
    red->ID[j] = '\0';
    j = 0;
    status = global_manager_set_wifi_ssid(red->ID, pdFALSE);
    for (int i = secondEqualIndex + 1; i <= len; i++)
    {
        red->PASS[j] = buff[i];
        j++;
    }
    status = global_manager_set_wifi_password(red->PASS, pdFALSE);
    // strncpy(red->ID, buff + equalIndex + 1, index - equalIndex - 1);
    // strncpy(red->PASS, buff + secondEqualIndex + 1, len - secondEqualIndex - 1);

    ESP_LOGI(TAG, "Salgo del parseo de RED");
};

void parse_hora(char *buff, struct tm *aux)
{
    // el & es el separador de los campos
    ESP_LOGI(HORA, "Entro al parseo de HORA");

    aux->tm_hour = atoi(&buff[5]);
    aux->tm_min = atoi(&buff[10]);
    current_time_manager_set_current_time(*aux);

    ESP_LOGI(HORA, "Salgo del parseo HORA");
};

//----------URL´S------------//
httpd_uri_t index_uri = {
    .uri = "/index",
    .method = HTTP_GET,
    .handler = index_get_handler,
    .user_ctx = NULL};

httpd_uri_t config_uri = {
    .uri = "/config",
    .method = HTTP_GET,
    .handler = config_get_handler,
    .user_ctx = NULL};

httpd_uri_t red_post = {
    .uri = "/red",
    .method = HTTP_POST,
    .handler = red_post_handler,
    .user_ctx = NULL};

httpd_uri_t pwm_post = {
    .uri = "/pwm",
    .method = HTTP_POST,
    .handler = pwm_post_handler,
    .user_ctx = NULL};

httpd_uri_t triac_post = {
    .uri = "/triac",
    .method = HTTP_POST,
    .handler = triac_post_handler,
    .user_ctx = NULL};

httpd_uri_t vegeflor_post = {
    .uri = "/vegeflor",
    .method = HTTP_POST,
    .handler = vegeflor_post_handler,
    .user_ctx = NULL};

httpd_uri_t hora_post = {
    .uri = "/hora",
    .method = HTTP_POST,
    .handler = hora_post_handler,
    .user_ctx = NULL};

httpd_uri_t data_red_uri = {
    .uri = "/data_red",
    .method = HTTP_GET,
    .handler = red_data_handler,
    .user_ctx = NULL};

httpd_uri_t data_pwm_uri = {
    .uri = "/data_pwm",
    .method = HTTP_GET,
    .handler = pwm_data_handler,
    .user_ctx = NULL};

httpd_uri_t data_triac_uri = {
    .uri = "/data_triac",
    .method = HTTP_GET,
    .handler = triac_data_handler,
    .user_ctx = NULL};

httpd_uri_t data_vegeflor_uri = {
    .uri = "/data_vegeflor",
    .method = HTTP_GET,
    .handler = vegeflor_data_handler,
    .user_ctx = NULL};

httpd_uri_t version_data_uri = {
    .uri = "/version",
    .method = HTTP_GET,
    .handler = version_data_handler,
    .user_ctx = NULL};

httpd_uri_t hora_data_uri = {
    .uri = "/data_hora",
    .method = HTTP_GET,
    .handler = hora_data_handler,
    .user_ctx = NULL};

httpd_uri_t image = {
    .uri = "/logo",
    .method = HTTP_GET,
    .handler = logo_handler,
    .user_ctx = NULL};

//----------HANDLERS PARA LOS HTML------------//
esp_err_t index_get_handler(httpd_req_t *req)
{
    extern unsigned char index_start[] asm("_binary_index_html_start");
    extern unsigned char index_end[] asm("_binary_index_html_end");
    size_t index_len = index_end - index_start;
    char indexHtml[index_len];
    memcpy(indexHtml, index_start, index_len);
    httpd_resp_send(req, indexHtml, index_len);
    return ESP_OK;
}

esp_err_t config_get_handler(httpd_req_t *req)
{
    extern unsigned char config_start[] asm("_binary_config_html_start");
    extern unsigned char config_end[] asm("_binary_config_html_end");
    size_t config_len = config_end - config_start;
    char configHtml[config_len];
    memcpy(configHtml, config_start, config_len);
    httpd_resp_send(req, configHtml, config_len);
    return ESP_OK;
}

esp_err_t logo_handler(httpd_req_t *req)
{
    extern unsigned char logo_start[] asm("_binary_logo_png_start");
    extern unsigned char logo_end[] asm("_binary_logo_png_end");
    size_t logo_len = logo_end - logo_start;
    char logo[logo_len];
    memcpy(logo, logo_start, logo_len);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, logo, logo_len);
    return ESP_OK;
}

//----------HANDLERS PARA LOS POST DE LAS SECCIONES------------//
esp_err_t pwm_post_handler(httpd_req_t *req)
{
    // Enviar una respuesta HTTP predeterminada
    ESP_LOGI(TAG, "ENTRE AL HANDLER DEL PWM");
    char buff[150];
    int ret, remaining = 0;
    remaining = req->content_len;
    ret = req->content_len;
    ESP_LOGI(TAG, "%d", ret);
    ESP_LOGI(TAG, "%d", remaining);
    if (remaining >= sizeof(buff))
    {
        /* Buffer de datos insuficiente */
        ESP_LOGI(TAG, "PAYLOAD MUY GRANDE");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }
    else
    {
        while (remaining > 0)
        {
            /* Leer los datos del formulario */
            ret = httpd_req_recv(req, buff, sizeof(buff)); // en buff se pone lo que estaba en el req y devuelve el numero de bytes que se pasaron al buffer
            if (ret <= 0)                                  // si es 0 o menor es que hubo error
            {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT)
                {
                    /* El tiempo de espera para recibir los datos ha expirado */
                    httpd_resp_send_408(req);
                }
                return ESP_FAIL;
            }
            remaining -= ret; // resto la cantidad que se pasaron para pasar en el siguiente ciclo el resto. aca cuidado porque los esstaria sobrescribiendo
            // Procesar los datos recibidos, por ejemplo, almacenarlos en una variable
        }
        ESP_LOGI(TAG, "%s", buff);
        parse_pwm(buff);
        ESP_LOGI(TAG, "Salgo del PWM HANDLER");

        //  aca irian las funciones de Gaston
        return ESP_OK;
    }
}

esp_err_t red_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "ENTRE AL HANDLER DE LA RED");
    char buff[50];
    memset(buff, '\0', sizeof(buff));
    int ret, remaining = 0;
    remaining = req->content_len;
    ret = req->content_len;
    ESP_LOGI(TAG, "%d", ret);
    ESP_LOGI(TAG, "%d", remaining);
    if (remaining >= sizeof(buff))
    {
        /* Buffer de datos insuficiente */
        ESP_LOGI(TAG, "PAYLOAD MUY GRANDE");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }
    else
    {
        while (remaining > 0)
        {
            /* Leer los datos del formulario */
            ret = httpd_req_recv(req, buff, sizeof(buff)); // en buff se pone lo que estaba en el req y devuelve el numero de bytes que se pasaron al buffer
            if (ret <= 0)                                  // si es 0 o menor es que hubo error
            {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT)
                {
                    /* El tiempo de espera para recibir los datos ha expirado */
                    httpd_resp_send_408(req);
                }
                return ESP_FAIL;
            }
            remaining -= ret; // resto la cantidad que se pasaron para pasar en el siguiente ciclo el resto. aca cuidado porque los esstaria sobrescribiendo
            // Procesar los datos recibidos, por ejemplo, almacenarlos en una variable
        }
        ESP_LOGI(TAG, "%s", buff);
        parse_red(buff, &red);
        print_red(&red);
        ESP_LOGI(TAG, "Salgo del RED HANDLER");
    }

    return ESP_OK;
}

esp_err_t triac_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "ENTRE AL HANDLER DEL TRIAC");
    char buff[200];
    int ret, remaining = 0;
    remaining = req->content_len;
    ret = req->content_len;
    ESP_LOGI(TAG, "%d", ret);
    ESP_LOGI(TAG, "%d", remaining);
    if (remaining >= sizeof(buff))
    {
        /* Buffer de datos insuficiente */
        ESP_LOGI(TRIAC, "PAYLOAD MUY GRANDE");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }
    else
    {
        while (remaining > 0)
        {
            /* Leer los datos del formulario */
            ret = httpd_req_recv(req, buff, sizeof(buff)); // en buff se pone lo que estaba en el req y devuelve el numero de bytes que se pasaron al buffer
            if (ret <= 0)                                  // si es 0 o menor es que hubo error
            {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT)
                {
                    /* El tiempo de espera para recibir los datos ha expirado */
                    httpd_resp_send_408(req);
                }
                return ESP_FAIL;
            }
            remaining -= ret; // resto la cantidad que se pasaron para pasar en el siguiente ciclo el resto. aca cuidado porque los esstaria sobrescribiendo
            // Procesar los datos recibidos, por ejemplo, almacenarlos en una variable
        }
        ESP_LOGI(TAG, "%s", buff);
        parse_triac(buff);
        ESP_LOGI(TAG, "Salgo del TRIAC HANDLER");

        //  aca irian las funciones de Gaston
        return ESP_OK;
    }
}

esp_err_t vegeflor_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "ENTRE AL HANDLER DEL VEGEFLOR");
    char buff[30];
    int ret, remaining = 0;
    remaining = req->content_len;
    ret = req->content_len;
    ESP_LOGI(TAG, "%d", ret);
    ESP_LOGI(TAG, "%d", remaining);
    if (remaining >= sizeof(buff))
    {
        /* Buffer de datos insuficiente */
        ESP_LOGI(VEGEFLOR, "PAYLOAD MUY GRANDE");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }
    else
    {
        while (remaining > 0)
        {
            /* Leer los datos del formulario */
            ret = httpd_req_recv(req, buff, sizeof(buff)); // en buff se pone lo que estaba en el req y devuelve el numero de bytes que se pasaron al buffer
            if (ret <= 0)                                  // si es 0 o menor es que hubo error
            {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT)
                {
                    /* El tiempo de espera para recibir los datos ha expirado */
                    httpd_resp_send_408(req);
                }
                return ESP_FAIL;
            }
            remaining -= ret; // resto la cantidad que se pasaron para pasar en el siguiente ciclo el resto. aca cuidado porque los esstaria sobrescribiendo
            // Procesar los datos recibidos, por ejemplo, almacenarlos en una variable
        }
        ESP_LOGI(TAG, "%s", buff);
        parse_vegeflor(buff);
        ESP_LOGI(TAG, "Salgo del VEGEFLOR HANDLER");

        //  aca irian las funciones de Gaston
        return ESP_OK;
    }
}

esp_err_t hora_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "ENTRE AL HANDLER DE LA HORA");
    char buff[30];
    int ret, remaining = 0;
    remaining = req->content_len;
    ret = req->content_len;
    ESP_LOGI(TAG, "%d", ret);
    ESP_LOGI(TAG, "%d", remaining);
    if (remaining >= sizeof(buff))
    {
        /* Buffer de datos insuficiente */
        ESP_LOGI(VEGEFLOR, "PAYLOAD MUY GRANDE");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }
    else
    {
        while (remaining > 0)
        {
            /* Leer los datos del formulario */
            ret = httpd_req_recv(req, buff, sizeof(buff)); // en buff se pone lo que estaba en el req y devuelve el numero de bytes que se pasaron al buffer
            if (ret <= 0)                                  // si es 0 o menor es que hubo error
            {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT)
                {
                    /* El tiempo de espera para recibir los datos ha expirado */
                    httpd_resp_send_408(req);
                }
                return ESP_FAIL;
            }
            remaining -= ret; // resto la cantidad que se pasaron para pasar en el siguiente ciclo el resto. aca cuidado porque los esstaria sobrescribiendo
            // Procesar los datos recibidos, por ejemplo, almacenarlos en una variable
        }
        ESP_LOGI(TAG, "%s", buff);
        parse_hora(buff, &aux_hora);
        ESP_LOGI(TAG, "Salgo del HORA HANDLER");

        //  aca irian las funciones de Gaston
        return ESP_OK;
    }
}

//----------HANDLERS PARA LEER LOS DATOS------------//
esp_err_t red_data_handler(httpd_req_t *req)
{
    uint8_t status = 0;
    status = global_manager_get_net_info(red.ID, red.PASS);
    if (status == 1)
    {
        cJSON *json_object = cJSON_CreateObject();
        cJSON_AddStringToObject(json_object, "ID", red.ID);
        cJSON_AddStringToObject(json_object, "PASS", red.PASS);
        char *json_str = cJSON_Print(json_object);
        ESP_LOGI("RED", "JSON ES: %s", json_str);
        cJSON_Delete(json_object);

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_str, strlen(json_str));

        free(json_str);

        return ESP_OK;
    }
    else
    {
        ESP_LOGI(TAG, "ERROR EN OBTENER SSID y PASSWORD");
        return ESP_FAIL;
    }
}

esp_err_t pwm_data_handler(httpd_req_t *req)
{
    char *modo;
    uint8_t status = 0;
    status = global_manager_get_pwm_info(&modo_pwm, &pwm_info);
    if (status == 1)
    {
        cJSON *json_object = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_object, "intensidad", pwm_info.percent_power);

        if (modo_pwm == MANUAL_ON)
        {
            modo = "Manual";
            cJSON_AddNumberToObject(json_object, "ih1h", pwm_info.turn_on_time.tm_hour);
            cJSON_AddNumberToObject(json_object, "ih1m", pwm_info.turn_on_time.tm_min);
            cJSON_AddNumberToObject(json_object, "fh1h", pwm_info.turn_off_time.tm_hour);
            cJSON_AddNumberToObject(json_object, "fh1m", pwm_info.turn_off_time.tm_min);
        }
        else if (modo_pwm == MANUAL_OFF)
        {
            modo = "OFF";
            cJSON_AddNumberToObject(json_object, "ih1h", pwm_info.turn_on_time.tm_hour);
            cJSON_AddNumberToObject(json_object, "ih1m", pwm_info.turn_on_time.tm_min);
            cJSON_AddNumberToObject(json_object, "fh1h", pwm_info.turn_off_time.tm_hour);
            cJSON_AddNumberToObject(json_object, "fh1m", pwm_info.turn_off_time.tm_min);
        }
        else if (modo_pwm == AUTOMATIC)
        {
            modo = "Automatico";
            cJSON_AddNumberToObject(json_object, "ih1h", pwm_info.turn_on_time.tm_hour);
            cJSON_AddNumberToObject(json_object, "ih1m", pwm_info.turn_on_time.tm_min);
            cJSON_AddNumberToObject(json_object, "fh1h", pwm_info.turn_off_time.tm_hour);
            cJSON_AddNumberToObject(json_object, "fh1m", pwm_info.turn_off_time.tm_min);
        }
        else
        {
            modo = "Indefinido";
            cJSON_AddStringToObject(json_object, "ih1h", "-");
            cJSON_AddStringToObject(json_object, "ih1m", "-");
            cJSON_AddStringToObject(json_object, "fh1h", "-");
            cJSON_AddStringToObject(json_object, "fh1m", "-");
        }
        cJSON_AddStringToObject(json_object, "Modo", modo);
        if (pwm_info.simul_day_status == SIMUL_DAY_ON)
        {
            modo = "Si";
        }
        else
        {
            modo = "No";
        }
        cJSON_AddStringToObject(json_object, "DIA", modo);

        char *json_str = cJSON_Print(json_object);
        ESP_LOGI(PWM, "JSON ES: %s", json_str);
        cJSON_Delete(json_object);

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_str, strlen(json_str));

        free(json_str);
        return ESP_OK;
    }
    else
    {
        ESP_LOGI(TAG, "ERROR EN OBTENER PWM");
        return ESP_FAIL;
    }
}

esp_err_t triac_data_handler(httpd_req_t *req)
{
    char *modo;
    uint8_t status = 0;
    status = global_manager_get_triac_info(&modo_triac, &triac_auto_info);
    if (status == 1)
    {
        cJSON *json_object = cJSON_CreateObject();
        if (modo_triac == MANUAL_ON)
        {
            modo = "Encendido";
        }
        if (modo_triac == MANUAL_OFF)
        {
            modo = "Apagado";
        }
        if (modo_triac == AUTOMATIC)
        {
            modo = "Automatico";
        }
        ESP_LOGE(TRIAC, " EL ENABLE DEL 1 ES %d", triac_auto_info.triac_auto[0].enable);
        cJSON_AddStringToObject(json_object, "Modo", modo);
        cJSON_AddBoolToObject(json_object, "cb1", triac_auto_info.triac_auto[0].enable);
        cJSON_AddNumberToObject(json_object, "ih1h", triac_auto_info.triac_auto[0].turn_on_time.tm_hour);
        cJSON_AddNumberToObject(json_object, "ih1m", triac_auto_info.triac_auto[0].turn_on_time.tm_min);
        cJSON_AddNumberToObject(json_object, "fh1h", triac_auto_info.triac_auto[0].turn_off_time.tm_hour);
        cJSON_AddNumberToObject(json_object, "fh1m", triac_auto_info.triac_auto[0].turn_off_time.tm_min);
        cJSON_AddBoolToObject(json_object, "cb2", triac_auto_info.triac_auto[1].enable);
        cJSON_AddNumberToObject(json_object, "ih2h", triac_auto_info.triac_auto[1].turn_on_time.tm_hour);
        cJSON_AddNumberToObject(json_object, "ih2m", triac_auto_info.triac_auto[1].turn_on_time.tm_min);
        cJSON_AddNumberToObject(json_object, "fh2h", triac_auto_info.triac_auto[1].turn_off_time.tm_hour);
        cJSON_AddNumberToObject(json_object, "fh2m", triac_auto_info.triac_auto[1].turn_off_time.tm_min);
        cJSON_AddBoolToObject(json_object, "cb3", triac_auto_info.triac_auto[2].enable);
        cJSON_AddNumberToObject(json_object, "ih3h", triac_auto_info.triac_auto[2].turn_on_time.tm_hour);
        cJSON_AddNumberToObject(json_object, "ih3m", triac_auto_info.triac_auto[2].turn_on_time.tm_min);
        cJSON_AddNumberToObject(json_object, "fh3h", triac_auto_info.triac_auto[2].turn_off_time.tm_hour);
        cJSON_AddNumberToObject(json_object, "fh3m", triac_auto_info.triac_auto[2].turn_off_time.tm_min);
        cJSON_AddBoolToObject(json_object, "cb4", triac_auto_info.triac_auto[3].enable);
        cJSON_AddNumberToObject(json_object, "ih4h", triac_auto_info.triac_auto[3].turn_on_time.tm_hour);
        cJSON_AddNumberToObject(json_object, "ih4m", triac_auto_info.triac_auto[3].turn_on_time.tm_min);
        cJSON_AddNumberToObject(json_object, "fh4h", triac_auto_info.triac_auto[3].turn_off_time.tm_hour);
        cJSON_AddNumberToObject(json_object, "fh4m", triac_auto_info.triac_auto[3].turn_off_time.tm_min);

        char *json_str = cJSON_Print(json_object);
        ESP_LOGI(TRIAC, "JSON ES: %s", json_str);
        cJSON_Delete(json_object);

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_str, strlen(json_str));

        free(json_str);

        return ESP_OK;
    }
    else
    {
        ESP_LOGI(TAG, "ERROR EN OBTENER DATOS DE TRIAC");
        return ESP_FAIL;
    }
}

esp_err_t vegeflor_data_handler(httpd_req_t *req)
{
    char *modo;
    uint8_t status = 0;
    status = global_manager_get_rele_vege_info(&rele_status);
    if (status == 1)
    {
        cJSON *json_object = cJSON_CreateObject();
        if (rele_status == RELE_VEGE_ENABLE)
        {
            modo = "Vegetativo";
        }
        else
        {
            modo = "Floracion";
        }
        cJSON_AddStringToObject(json_object, "Modo", modo);

        char *json_str = cJSON_Print(json_object);
        ESP_LOGI(VEGEFLOR, "JSON ES: %s", json_str);
        cJSON_Delete(json_object);

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_str, strlen(json_str));

        free(json_str);

        return ESP_OK;
    }
    else
    {
        ESP_LOGI(TAG, "ERROR EN OBTENER VEGEFLOR");
        return ESP_FAIL;
    }
}

esp_err_t version_data_handler(httpd_req_t *req)
{
    cJSON *json_object = cJSON_CreateObject();
    char version[10];
    uint8_t len_ver;
    get_version(version, &len_ver);
    cJSON_AddStringToObject(json_object, "Version", version);

    char *json_str = cJSON_Print(json_object);
    ESP_LOGI(VERSIONN, "JSON ES: %s", json_str);
    cJSON_Delete(json_object);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    free(json_str);

    return ESP_OK;
}

esp_err_t hora_data_handler(httpd_req_t *req)
{

    uint8_t status = 0;
    status = global_manager_get_current_time_info(&aux_hora);
    if (status == 1)
    {
        cJSON *json_object = cJSON_CreateObject();
        cJSON_AddNumberToObject(json_object, "Hora_h", aux_hora.tm_hour);
        cJSON_AddNumberToObject(json_object, "Hora_m", aux_hora.tm_min);
        char *json_str = cJSON_Print(json_object);
        ESP_LOGI(HORA, "JSON ES: %s", json_str);
        cJSON_Delete(json_object);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_str, strlen(json_str));
        free(json_str);
        return ESP_OK;
    }
    else
    {
        ESP_LOGI(TAG, "ERROR EN OBTENER HORA");
        return ESP_FAIL;
    }
}

//---------FUNCIONES DEL WEBSERVER-------------//

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG(); // Configuracion por default del server
    config.stack_size = 250000;
    config.max_uri_handlers = 15;
    config.max_resp_headers = 15;

    esp_err_t ret = httpd_start(&server, &config);
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (ret == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        // ESP_LOGI(TAG, "Registering HTML");
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &config_uri);
        httpd_register_uri_handler(server, &pwm_post);
        httpd_register_uri_handler(server, &red_post);
        httpd_register_uri_handler(server, &triac_post);
        httpd_register_uri_handler(server, &vegeflor_post);
        httpd_register_uri_handler(server, &data_red_uri);
        httpd_register_uri_handler(server, &data_pwm_uri);
        httpd_register_uri_handler(server, &data_triac_uri);
        httpd_register_uri_handler(server, &data_vegeflor_uri);
        httpd_register_uri_handler(server, &version_data_uri);
        httpd_register_uri_handler(server, &hora_data_uri);
        httpd_register_uri_handler(server, &hora_post);
        httpd_register_uri_handler(server, &image);
        init_red(&red);
        return server;
    }
    else if (ret == ESP_ERR_HTTPD_ALLOC_MEM)
    {
        ESP_LOGI(TAG, "ESP_ERR_HTTPD_ALLOC_MEM \n");
    }
    else if (ret == ESP_ERR_HTTPD_TASK)
    {
        ESP_LOGI(TAG, "ESP_ERR_HTTPD_TASK \n");
    }
    else
    {
        ESP_LOGI(TAG, "ret = %i \n", (int)ret);
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

void disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK)
        {
            *server = NULL;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}
