/*
	SetTimerPeriod( uint64_t period );
		period -> microseconds


	Pinos:
		Leitura GPIO43 -> ADC1_CHANNEL_6

*/

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include "freertos/event_groups.h"
#include <driver/gpio.h>
#include <esp_system.h>
#include <esp_log.h>
#include <driver/uart.h>
#include <driver/adc.h>
#include "esp_wifi.h"
#include "nvs_flash.h"

#include <mqtt_client.h>
            
#define BTN GPIO_NUM_33

#define LED0 GPIO_NUM_27
#define LED_ON  gpio_set_level(LED0, 1);
#define LED_OFF gpio_set_level(LED0, 0);

#define TAG_MQTT "MQTT"

// #define WIFI_SSID   "YecidAP"
// #define WIFI_PASSWD "autocad2013"
// #define HOST_IP_ADDR  "192.168.43.82"

// #define WIFI_SSID   "RoboticLab24"
// #define WIFI_PASSWD "8852966258"
// #define HOST_IP_ADDR  "192.168.0.18"

// #define WIFI_SSID   "USP_BomFuturo"
// #define WIFI_PASSWD "123456789"
// #define HOST_IP_ADDR  "10.42.0.1"

#define WIFI_SSID   "BF-WIFI"
#define WIFI_PASSWD "smbfsmbf"
// #define HOST_IP_ADDR  "10.99.35.143"
// #define HOST_IP_ADDR  "10.25.35.101"
#define HOST_URL  "mqtt://10.99.35.143:1883"
/*
    1Hz    -> 1000000
    10Hz   -> 100000
    100Hz  -> 10000
    1000Hz -> 1000
*/
#define Fs  10
#define Ts  1000000/Fs     //In microseconds [us] 
#define LOG_BUFFER_SIZE 1 //Fs*5
#define SIZE_TO_SEND Fs

#include <CT_custom_timer.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void WifiTask(void* arg);
CT_def ct = {0};

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        gpio_set_level(LED0, 0);
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
        gpio_set_level(LED0, 1);
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

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG_MQTT, "Last error %s: 0x%x", message, error_code);
    }
}

bool connected = false;
esp_mqtt_client_handle_t client;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    int msg_id = 0;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        CT_start(&ct,Ts);
        connected = true;
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/test", "data_3", 0, 1, 0);
        // ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG_MQTT, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        connected = false;
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // connected = true;
        // msg_id = esp_mqtt_client_publish(client, "/test", "data", 0, 0, 0);
        ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        connected = false;
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG_MQTT, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
        break;
    }
}

#define CONFIG_BROKER_URL_FROM_STDIN

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = HOST_URL,
        .username = "esp32",
        .password = "esp32",
        .client_id = "esp32humidity210.25.35.101",
        
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}



bool _is_sending = false;
IRAM_ATTR int _adc_buffer[LOG_BUFFER_SIZE];
IRAM_ATTR int _buffer_position = 0;
IRAM_ATTR int _last_adc_value = 0;
uint64_t samples = 0;

SemaphoreHandle_t _mtx_useBuffer;
SemaphoreHandle_t _mtx_sendBuffer;


void IRAM_ATTR _cb_readADC(void* arg);

void app_main() {

	_mtx_useBuffer = xSemaphoreCreateBinary();
    _mtx_sendBuffer = xSemaphoreCreateBinary();
        
	gpio_set_direction(LED0, GPIO_MODE_OUTPUT);

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = BTN;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    gpio_set_direction(BTN, GPIO_MODE_INPUT);

	
	CT_set_callback(&ct,&_cb_readADC);
	CT_create(&ct);
	CT_start(&ct,Ts);	


	init_wifi();

	xTaskCreate(&WifiTask, "WifiTask", 2048, NULL, 2, NULL);

    bool btn_1 = false;
    bool btn_0 = false;

    mqtt_app_start();

    

	for(;;) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


void  IRAM_ATTR _cb_readADC(void* arg)
{	
	int aux = 0;
	int _cur_adc_value = adc1_get_raw(ADC1_CHANNEL_6);

	xSemaphoreGiveFromISR(_mtx_useBuffer, &aux); 
	
    // _cur_adc_value = 50;

	_last_adc_value = _cur_adc_value;
	_adc_buffer[_buffer_position] = _last_adc_value;

    
	if ( (_buffer_position+1) < (LOG_BUFFER_SIZE)){
		_buffer_position++;
	}else{
        _buffer_position = 0;
        xSemaphoreGive(_mtx_sendBuffer);
	}
    
    // ESP_LOGI("OUT","%d",_buffer_position);	
    
    samples++;

	if (aux) {
		portYIELD_FROM_ISR(); 
	}

    

	// int64_t time_since_boot = esp_timer_get_time();
}

void WifiTask(void* arg){

    char valueNum[10];

	for(;;){       
        

        if(xSemaphoreTake(_mtx_sendBuffer,pdMS_TO_TICKS(200))){
            if(connected){
                LED_ON;
                    sprintf(valueNum,"%d",_last_adc_value);
                    esp_mqtt_client_publish(client, "Humidity", valueNum, 0, 1, 0);
                LED_OFF;
            }else{

            }
        }

        // if(_sock_retry)xSemaphoreGive(_mtx_sendBuffer);		
	}
}