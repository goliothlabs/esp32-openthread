/*
 * Copyright (c) 2024 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_openthread.h"
#include "esp_openthread_lock.h"
#include "esp_openthread_netif_glue.h"
#include "esp_openthread_types.h"
#include "esp_ot_config.h"
#include "esp_vfs_eventfd.h"
#include "freertos/task.h"
#include "hal/uart_types.h"
#include "openthread/instance.h"
#include "openthread/logging.h"
#include "openthread/tasklet.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "nvs.h"
#include "shell.h"
#include "sample_credentials.h"
#include <golioth/client.h>
#include "golioth/golioth_sys.h"
#include <golioth/stream.h>
#include <golioth/settings.h>
#include <golioth/fw_update.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include "app_rpc.h"
#include "app_settings.h"
#include "app_state.h"

#include "esp_openthread_cli.h"

#define TAG "ot_main"

struct golioth_client *glth_client;

static SemaphoreHandle_t _connected_sem = NULL;
static const char *_current_version = "1.0.0";

static void on_client_event(struct golioth_client *client,
			    enum golioth_client_event event,
			    void *arg)
{
	bool is_connected = (event == GOLIOTH_CLIENT_EVENT_CONNECTED);
	if (is_connected) {
		xSemaphoreGive(_connected_sem);
	}
	GLTH_LOGI(TAG, "Golioth client %s", is_connected ? "connected" : "disconnected");
}


static esp_netif_t *init_openthread_netif(const esp_openthread_platform_config_t *config)
{
	esp_netif_config_t cfg = ESP_NETIF_DEFAULT_OPENTHREAD();
	esp_netif_t *netif = esp_netif_new(&cfg);
	assert(netif != NULL);

	ESP_ERROR_CHECK(esp_netif_attach(netif, esp_openthread_netif_glue_init(config)));

	return netif;
}

static void async_stream_handler(struct golioth_client *client,
                                 const struct golioth_response *response,
                                 const char *path,
				 void *arg)
{
	if (response->status != GOLIOTH_OK) {
		GLTH_LOGW(TAG, "Failed to push async stream payload: %d", response->status);
		return;
	}

	GLTH_LOGI(TAG, "Successfully pushed async stream payload");

	return;
}

static void send_stream_payload(int* counter)
{

	uint8_t cbor_buf[15];

	/* Encode CBOR payload */
	ZCBOR_STATE_E(zse, 1, cbor_buf, sizeof(cbor_buf), 1);
	
	bool ok = zcbor_map_start_encode(zse, 1) && \
		  zcbor_tstr_put_lit(zse, "counter") && \
		  zcbor_float32_put(zse, *counter) && \
		  zcbor_map_end_encode(zse, 1);

	if (!ok) {
		GLTH_LOGE(TAG, "Failed to close CBOR map");
		return;
	}
   
	size_t payload_size = (intptr_t) zse->payload - (intptr_t) cbor_buf;
   
	/* Stream CBOR payload */
	golioth_stream_set_async(glth_client,
				 "cbor",
				 GOLIOTH_CONTENT_TYPE_CBOR,
				 cbor_buf,
                                 payload_size,
				 async_stream_handler,
				 NULL);
   
	/* Encode JSON payload */
	char json_buf[sizeof("{\"counter\":4294967295}")];
	snprintf(json_buf, sizeof(json_buf), "{\"counter\":%d}", *counter);

	/* Stream JSON payload */
	golioth_stream_set_async(glth_client,
				 "json",
				 GOLIOTH_CONTENT_TYPE_JSON,
				 (const uint8_t *) json_buf,
                                 payload_size,
				 async_stream_handler,
				 NULL);

}

static void golioth_task(void *aContext)
{
	int counter = 0;

	const struct golioth_client_config *glth_config = golioth_sample_credentials_get();
	glth_client = golioth_client_create(glth_config);
	assert(glth_client);

	_connected_sem = xSemaphoreCreateBinary();

	golioth_client_register_event_callback(glth_client, on_client_event, NULL);

	GLTH_LOGW(TAG, "Waiting for connection to Golioth...");
	xSemaphoreTake(_connected_sem, portMAX_DELAY);

	/* Initialize DFU components */
	golioth_fw_update_init(glth_client, _current_version);
	
	/* Register Settings service */
	app_settings_register(glth_client);
	
	/* Register RPC service */
	app_rpc_register(glth_client);

	/* Observe State service data */
	app_state_observe(glth_client);

	while (true) {

		send_stream_payload(&counter);
		++counter;


		/* fix: If LOOP_DELAY_S is changed, it won't take effect unitl the 
		   previous value has expired.
		*/
	   	vTaskDelay((get_loop_delay_s() * 1000) / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	GLTH_LOGI(TAG, "Starting OpenThread Demo v%s", _current_version);

	esp_vfs_eventfd_config_t eventfd_config = {
		.max_fds = 3,
	};
	
	nvs_init();
	shell_start();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_vfs_eventfd_register(&eventfd_config));

	esp_openthread_platform_config_t ot_config = {
		.radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
		.port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
	};

	xTaskCreate(golioth_task, "golioth_task", 10240, xTaskGetCurrentTaskHandle(), 5, NULL);

	// Initialize the OpenThread stack
	ESP_ERROR_CHECK(esp_openthread_init(&ot_config));

	// Initialize the OpenThread cli
	#if CONFIG_OPENTHREAD_CLI
		esp_openthread_cli_init();
	#endif

	#if CONFIG_OPENTHREAD_CLI_ESP_EXTENSION
		esp_cli_custom_command_init();
	#endif

	#if CONFIG_OPENTHREAD_CLI
		esp_openthread_cli_create_task();
	#endif

	esp_netif_t *openthread_netif;
	openthread_netif = init_openthread_netif(&ot_config);
	esp_netif_set_default_netif(openthread_netif);
	
	otOperationalDatasetTlvs dataset;
	otError error = otDatasetGetActiveTlvs(esp_openthread_get_instance(), &dataset);
	ESP_ERROR_CHECK(esp_openthread_auto_start((error == OT_ERROR_NONE) ? &dataset : NULL));
	esp_openthread_launch_mainloop();

}
