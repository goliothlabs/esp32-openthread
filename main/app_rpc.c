#include "esp_system.h"
#include "esp_log.h"
#include <golioth/client.h>
#include <golioth/rpc.h>
#include "app_rpc.h"
#include "golioth/golioth_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "zcbor_common.h"

#define TAG "app_rpc"


static void rpc_log_if_register_failure(int err)
{
	if (err) {
		GLTH_LOGE(TAG, "Failed to register RPC: %d", err);
	}
}

static void rpc_reboot_task(void *aContext)
{
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	esp_restart();
}

static enum golioth_rpc_status on_reboot(zcbor_state_t *request_params_array,
					 zcbor_state_t *response_detail_map,
					 void *callback_arg)
{
	GLTH_LOGI(TAG, "Prepare to restart system!");

	/* Create a task so this RPC can return confirmation to Golioth */
	xTaskCreate(rpc_reboot_task, "rpc_reboot_task", 1024, xTaskGetCurrentTaskHandle(), 5, NULL);

	return GOLIOTH_RPC_OK;
}

static enum golioth_rpc_status on_set_log_level(zcbor_state_t *request_params_array,
						zcbor_state_t *response_detail_map,
						void *callback_arg)
{
	double param_0;
	uint8_t log_level;
	bool ok;

	GLTH_LOGW(TAG, "on_set_log_level");

	ok = zcbor_float_decode(request_params_array, &param_0);
	if (!ok) {
		GLTH_LOGE(TAG, "Failed to decode array item");
		return GOLIOTH_RPC_INVALID_ARGUMENT;
	}

	log_level = (uint8_t)param_0;

	if ((log_level <= 0) || (log_level > ESP_LOG_VERBOSE)) {

		GLTH_LOGE(TAG, "Requested log level is out of bounds: %d", log_level);
		return GOLIOTH_RPC_INVALID_ARGUMENT;
	}

	esp_log_level_set("*", log_level);

	GLTH_LOGW(TAG, "Log levels for all modules set to: %d", log_level);

	ok = zcbor_tstr_put_lit(response_detail_map, "log_modules") &&
	     zcbor_tstr_put_term(response_detail_map, "all", 8);

	return GOLIOTH_RPC_OK;
}

void app_rpc_register(struct golioth_client *client)
{
	struct golioth_rpc *rpc = golioth_rpc_init(client);

	int err;

	err = golioth_rpc_register(rpc, "reboot", on_reboot, NULL);
	rpc_log_if_register_failure(err);

	err = golioth_rpc_register(rpc, "set_log_level", on_set_log_level, NULL);
	rpc_log_if_register_failure(err);
}

