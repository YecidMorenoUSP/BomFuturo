
SemaphoreHandle_t smf;
TimerHandle_t tmr;
volatile int state = 0;

void IRAM_ATTR ISR(void *z) {
	int aux = 0;
	xSemaphoreGiveFromISR(smf, &aux); 

	if (aux) {
		portYIELD_FROM_ISR(); 
	}
}

void app_main() {
	
	smf = xSemaphoreCreateBinary();

	tmr = xTimerCreate("tmr_smf", pdMS_TO_TICKS(1000), true, 0, ISR); 
	xTimerStart(tmr, pdMS_TO_TICKS(100));

	gpio_pad_select_gpio(GPIO_NUM_2);
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

	while (1) {
		if (xSemaphoreTake(smf, portMAX_DELAY)) {
			ESP_LOGI("Timer", "Expirou!");
			gpio_set_level(GPIO_NUM_2, state^=1);
		}

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

static void periodic_timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI("T_periodic", "Periodic timer called, time since boot: %lld us", time_since_boot);
}