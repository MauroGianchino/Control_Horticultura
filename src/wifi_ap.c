#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "nvs_flash.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/ip4_addr.h"
#include "esp_mac.h"
#include "wifi_ap.h"
#include "../include/global_manager.h"
#include "../include/web_server.h"
#include "../include/nv_flash_manager.h"
//------------------------------------------------------------------------------
#define DEBUG_MODULE

#define EXAMPLE_ESP_WIFI_SSID "Red_Hor"
#define EXAMPLE_ESP_WIFI_PASS "Horti2505"
#define EXAMPLE_ESP_WIFI_CHANNEL 1
#define EXAMPLE_MAX_STA_CONN 10
#define TAG "WIFI AP"
#define DEFAULT_AP_IP "192.168.1.1"
#define DEFAULT_AP_GATEWAY "192.168.1.1"
#define DEFAULT_AP_NETMASK "255.255.255.0"
//------------------------------------------------------------------------------
esp_netif_t *esp_netif_ap = NULL;

// red_t red_things;
//------------------------------------------------------------------------------
static uint8_t nv_get_ssid_ap_wifi(char *ssid);
static uint8_t nv_get_password_ap_wifi(char *password);
//------------------------------------------------------------------------------
static uint8_t nv_get_ssid_ap_wifi(char *ssid)
{
    char ssid_aux[DEVICE_SSID_MAX_LENGTH];
    bool read_ok = false;
    uint8_t retries_max = 5;
    uint8_t retries = 0;

    memset(ssid_aux, '\0', sizeof(ssid_aux));

    do{
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if (read_str_from_flash(WIFI_AP_SSID_KEY, ssid_aux))
        {
            #ifdef DEBUG_MODULE
                printf("ssid read: %s \n", ssid_aux);
            #endif
            strcpy(ssid, ssid_aux);
            read_ok = true;
        }
        else
        {
            retries++;
            read_ok = false;
            #ifdef DEBUG_MODULE
                printf("ssid not_read \n");
            #endif
        }
    }while((read_ok == false) && (retries < retries_max));

    #ifdef DEBUG_MODULE
        if(retries == retries_max)
        {
            printf("ssid not_read, max retries reached \n");
        }
    #endif
    
    if(read_ok == true)
    {
        return 1;
    }
    
    strcpy(ssid, EXAMPLE_ESP_WIFI_SSID);
    return 0;
}
//------------------------------------------------------------------------------
static uint8_t nv_get_password_ap_wifi(char *password)
{
    char password_aux[DEVICE_PASS_MAX_LENGTH];
    bool read_ok = false;
    uint8_t retries_max = 5;
    uint8_t retries = 0;

    memset(password_aux, '\0', sizeof(password_aux));

    do{
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if (read_str_from_flash(WIFI_AP_PASSWORD_KEY, password_aux))
        {
            #ifdef DEBUG_MODULE
                printf("password read: %s \n", password_aux);
            #endif
            if(strlen(password_aux) < 8)
            {
                strcpy(password, EXAMPLE_ESP_WIFI_PASS);
                return 0;
            }
            strcpy(password, password_aux);
            read_ok = true;
        }
        else
        {
            retries++;
            read_ok = false;
            #ifdef DEBUG_MODULE
                printf("password not_read \n");
            #endif
        }
    }while((read_ok == false) && (retries < retries_max));

    #ifdef DEBUG_MODULE
        if(retries == retries_max)
        {
            printf("password not_read, max retries reached \n");
        }
    #endif
    
    if(read_ok == true)
    {
        return 1;
    }
    
    strcpy(password, EXAMPLE_ESP_WIFI_PASS);
    return 0;
}
//------------------------------------------------------------------------------
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;

        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;

        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}
//------------------------------------------------------------------------------
void wifi_init_softap(void)
{
    // 1-Fase de inicio de WiFi

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_ap = esp_netif_create_default_wifi_ap(); // WiFi AP

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // estructura de configuracion. se inicia en default.

    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // Inicio el WiFi

    // 2-Fase de configuración de WiFi

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL)); // aca entra la funcion del handler

    // global_manager_get_net_info(red_things.ID, red_things.PASS);

    char ssid[50];
    char password[50];

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                .required = true,
            },
        },
    };

    nv_get_ssid_ap_wifi((char *)ssid);

    memset((char *)wifi_config.ap.ssid, '\0', sizeof(wifi_config.ap.ssid));
    memcpy((char *)wifi_config.ap.ssid, ssid, strlen(ssid));
    wifi_config.ap.ssid_len = strlen(ssid);

    nv_get_password_ap_wifi((char *)password);
    memset((char *)wifi_config.ap.password, '\0', sizeof(wifi_config.ap.password));
    strncpy((char *)wifi_config.ap.password, password, strlen(password));
    //printf("strlen(password) %d \n",strlen(password));

    //memset((char *)wifi_config.ap.password, '\0', sizeof(wifi_config.ap.password));
    //strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
    
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    /* DHCP AP configuration */
    // esp_netif_dhcps_stop(esp_netif_ap); /* DHCP client/server must be stopped before setting new IP information. */
    /*esp_netif_ip_info_t ap_ip_info;
    memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));
    inet_pton(AF_INET, DEFAULT_AP_IP, &ap_ip_info.ip);
    inet_pton(AF_INET, DEFAULT_AP_GATEWAY, &ap_ip_info.gw);
    inet_pton(AF_INET, DEFAULT_AP_NETMASK, &ap_ip_info.netmask);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));*/

    // 3- Fase de start de WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}
//------------------------------------------------------------------------------