
typedef struct{ 
	esp_timer_create_args_t timer_args;
	esp_timer_handle_t timer;
	int val;
} CT_def;

esp_err_t CT_create(CT_def * ct){
	return esp_timer_create(&ct->timer_args, &ct->timer);
}

esp_err_t CT_start(CT_def * ct,uint64_t period_us){
	return esp_timer_start_periodic(ct->timer, period_us);
}

void CT_stop(CT_def * ct){
	esp_timer_stop(ct->timer);
} 

void CT_set_callback(CT_def * ct,esp_timer_cb_t callback){
	ct->timer_args.callback = callback;
}
