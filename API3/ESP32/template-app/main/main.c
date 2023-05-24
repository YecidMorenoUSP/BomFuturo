#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <math.h>

#include "freertos/event_groups.h"
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


volatile bool WIFI_ENABLED_TO_SEND_BIT =  false;

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

#define SERVER_ADDR "192.168.0.21"  // Dirección IP del servidor
#define SERVER_PORT 5000            // Puerto del servidor

#define WIFI_SSID "2 4G_Escalante Diaz"
#define WIFI_PASSWD "colombia2018"

esp_event_loop_handle_t event_loop_handle;

struct sockaddr_in server_addr;
int sockfd, status;

void createSock(){  

    // Crear el socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP);
    if (sockfd < 0) {
        printf("Error creando el socket\n");
        return;
    }

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr);

    // Conectarse al servidor
    status = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status < 0) {
        printf("Error conectando al servidor\n");
        close(sockfd);
        
        return;
    }

}

void closeSock(){
    close(sockfd);
}

void enviar_entero(float entero)
{
    status = send(sockfd, &entero, sizeof(entero), 0);
    if (status < 0) {
        printf("Error enviando el entero\n");
        close(sockfd);
        vTaskDelay(pdMS_TO_TICKS(1000));
        createSock();
        return;
    }
}

typedef struct {
    int HS;
    float time;
    float val;
} DATA;


void sendData(DATA d){
    status = send(sockfd, &d, sizeof(d), 0);
    if (status < 0) {
        printf("Error enviando el d\n");
        close(sockfd);
        vTaskDelay(pdMS_TO_TICKS(100));
        createSock();
        return;
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        closeSock();
        WIFI_ENABLED_TO_SEND_BIT = false;
        if (s_retry_num < 10) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI("WIFI", "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI("WIFI","connect to the AP fail");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WIFI", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        createSock();
        WIFI_ENABLED_TO_SEND_BIT = true;
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void init_wifi(){

	esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI("WIFI", "wifi_init_sta finished.");

	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("WIFI", "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASSWD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI("WIFI", "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASSWD);
    } else {
        ESP_LOGE("WIFI", "UNEXPECTED EVENT");
    }

}





void app_main()
{ 
    
    init_wifi();

    TickType_t t0 = xTaskGetTickCount();
    float t;
    DATA d;
    d.HS = 1234;
    for(;;) {
		
        vTaskDelay(pdMS_TO_TICKS(10));

        t =  pdTICKS_TO_MS(xTaskGetTickCount()-t0)/1000.0f; 
        if(WIFI_ENABLED_TO_SEND_BIT){
            d.time = t;
            d.val  = sin(2.0*M_PI*t);
            sendData(d);
        }
        
	}


    
}
