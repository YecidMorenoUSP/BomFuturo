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


#define BTN GPIO_NUM_33

#define LED0 GPIO_NUM_27
#define LED_ON  gpio_set_level(LED0, 1);
#define LED_OFF gpio_set_level(LED0, 0);

// #define WIFI_SSID   "YecidAP"
// #define WIFI_PASSWD "autocad2013"
// #define HOST_IP_ADDR  "192.168.43.82"

// #define WIFI_SSID   "RoboticLab24"
// #define WIFI_PASSWD "8852966258"
// #define HOST_IP_ADDR  "192.168.0.18"

#define WIFI_SSID   "USP_BomFuturo"
#define WIFI_PASSWD "123456789"
#define HOST_IP_ADDR  "10.42.0.1"

// #define WIFI_SSID   "BF-WIFI"
// #define WIFI_PASSWD "smbfsmbf"
// #define HOST_IP_ADDR  "10.99.35.143"

/*
    1Hz    -> 1000000
    10Hz   -> 100000
    100Hz  -> 10000
    1000Hz -> 1000
*/
#define Fs  500
#define Ts  1000000/Fs     //In microseconds [us] 
#define LOG_BUFFER_SIZE Fs*10
#define SIZE_TO_SEND Fs

#include <CT_custom_timer.h>
#include <tcp_client_v4.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
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


void WifiTask(void* arg);
CT_def ct = {0};

bool _is_sending = true;
IRAM_ATTR int _adc_buffer[LOG_BUFFER_SIZE];
IRAM_ATTR int _buffer_position = 0;
IRAM_ATTR int _last_adc_value = 0;
uint64_t samples = 0;

SemaphoreHandle_t _mtx_useBuffer;
SemaphoreHandle_t _mtx_sendBuffer;


void IRAM_ATTR _cb_readADC(void* arg);

void app_main() {

    
    // esp_log_level_set("*",ESP_LOG_NONE);
	// uart_set_baudrate(UART_NUM_0, 115200);
	
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
	// tcp_client();

	xTaskCreate(&WifiTask, "WifiTask", 2048, NULL, 2, NULL);

    bool btn_1 = false;
    bool btn_0 = false;
	for(;;) {
        // ESP_ERROR_CHECK(esp_timer_dump(stdout));
		// if (xSemaphoreTake(_mtx_useBuffer, portMAX_DELAY)){
		// 	ESP_LOGI("OUT","N:%lld V:%d ",samples,_last_adc_value);	
		// }
        // ESP_LOGI("INFO","Size of int: %d Bytes",sizeof(int));

        // btn_1 = gpio_get_level(BTN);

        // if( !btn_1  && btn_0 ){
        //     CT_start(&ct,Ts);
        // }else{
        //     CT_stop(&ct);
        // }

        // btn_1 = btn_0;

		vTaskDelay(pdMS_TO_TICKS(500));
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
	_connect_socket();

    bool _sock_status = false;
    bool _sock_retry = false;
	for(;;){
        
        // if(!gpio_get_level(BTN)){
        //     vTaskDelay(pdMS_TO_TICKS(100));
        //     continue;
        // }if(!gpio_get_level(BTN)){
        //     vTaskDelay(pdMS_TO_TICKS(100));
        //     continue;
        // }

        if(!_sock_status){
            _close_socket();
            vTaskDelay(pdMS_TO_TICKS(100));
            if(_connect_socket()==ESP_OK) _sock_status = true;
        }
        
        

        if(xSemaphoreTake(_mtx_sendBuffer,pdMS_TO_TICKS(200)) && _sock_status==true){
            CT_stop(&ct);	
            _is_sending = true;
            LED_ON
            if (xSemaphoreTake(_mtx_useBuffer, portMAX_DELAY)){		
                
                if(sendData(_adc_buffer,LOG_BUFFER_SIZE*sizeof(int)) != ESP_OK){ //LOG_BUFFER_SIZE*sizeof(int)
                    _sock_status = false;
                    _sock_retry  = true;
                }else{
                    _sock_status = false;
                    _sock_retry  = false;
                    _is_sending = false;
                    memset(_adc_buffer,0,LOG_BUFFER_SIZE*sizeof(int));
                    _buffer_position = 0;
                    CT_start(&ct,Ts);
                    LED_OFF;
                    // ESP_LOGI("OUT","");	
                    // for(int i = 0 ; i < 20 ; i++) printf("%d ",_adc_buffer[i]);
                    // printf("\n");              
                }

                xSemaphoreGive(_mtx_useBuffer);
            }
        }

        if(_sock_retry)xSemaphoreGive(_mtx_sendBuffer);		
	}
}