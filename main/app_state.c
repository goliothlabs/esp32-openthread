#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <golioth/client.h>
#include <golioth/lightdb_state.h>
#include <golioth/payload_utils.h>
#include <golioth/golioth_sys.h>
#include <golioth/zcbor_utils.h>
#include <stdint.h>
#include <string.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>
#include <cJSON.h>
#include <inttypes.h>

#include "app_state.h"
#include "golioth/golioth_debug.h"

#define TAG "app_state"

static struct golioth_client *client;

int32_t _example_int0 = 0;
int32_t _example_int1 = 1;
struct app_state parsed_state;

static void async_handler(struct golioth_client *client,
			 const struct golioth_response *response,
			 const char *path,
			 void *arg)
{
	if (response->status != GOLIOTH_OK) {
		GLTH_LOGW(TAG, "Failed to set state: %d", response->status);
		return;
	}

	GLTH_LOGD(TAG, "State successfully set");
}

int app_state_reset_desired(void)
{
	GLTH_LOGI(TAG, "Resetting \"%s\" LightDB State endpoint to defaults.", APP_STATE_DESIRED_ENDP);

	int err = 0;
	uint8_t buf[128];
	
	ZCBOR_STATE_E(zse, 1, buf, sizeof(buf), 1);

	bool ok = zcbor_map_start_encode(zse, 2) &&
		  zcbor_tstr_put_lit(zse, "example_int0") &&
		  zcbor_int32_put(zse, -1) &&
		  zcbor_tstr_put_lit(zse, "example_int1") &&
		  zcbor_int32_put(zse, -1) &&
		  zcbor_map_end_encode(zse, 2);
		  
	
	if (!ok) {
		GLTH_LOGE(TAG, "Failed to encode CBOR object");
		return err;
	}

	size_t payload_size = (intptr_t) zse->payload - (intptr_t) buf;

	GLTH_LOG_BUFFER_HEXDUMP(TAG, buf, payload_size, GOLIOTH_DEBUG_LOG_LEVEL_DEBUG);

	err = golioth_lightdb_set_async(client,
					APP_STATE_DESIRED_ENDP,
					GOLIOTH_CONTENT_TYPE_CBOR,
					buf,
					payload_size,
					async_handler,
					NULL);
	if (err) {
		GLTH_LOGE("Reset Desired", "Unable to write to LightDB State: %d", err);
	}
	return err;
}

int app_state_update_actual(void)
{
	int err = 0;
	uint8_t buf[128];
	
	ZCBOR_STATE_E(zse, 1, buf, sizeof(buf), 1);
	
	bool ok = zcbor_map_start_encode(zse, 2) &&
		  zcbor_tstr_put_lit(zse, "example_int0") &&
		  zcbor_int32_put(zse, _example_int0) &&
		  zcbor_tstr_put_lit(zse, "example_int1") &&
		  zcbor_int32_put(zse, _example_int1) &&
		  zcbor_map_end_encode(zse, 2);
	
	if (!ok) {
		GLTH_LOGE(TAG, "Failed to encode CBOR object");
		return err;
	}

	size_t payload_size = (intptr_t) zse->payload - (intptr_t) buf;

	err = golioth_lightdb_set_async(client,
					APP_STATE_ACTUAL_ENDP,
					GOLIOTH_CONTENT_TYPE_CBOR,
					buf,
					payload_size,
					async_handler,
					NULL);

	if (err) {
		GLTH_LOGE(TAG, "Unable to write to LightDB State: %d", err);
	}

	return err;
}

static int zcbor_map_int32_decode(zcbor_state_t *zsd, void *value)
{
	bool ok;

	ok = zcbor_int32_decode(zsd, value);
	if (!ok) {
		GLTH_LOGW(TAG, "Desired value decode error = %d", ok);
		return -1;
	}

	return 0;
}

static void app_state_desired_handler(struct golioth_client *client,
				      const struct golioth_response *response,
				      const char *path,
				      const uint8_t *payload,
				      size_t payload_size,
				      void *arg)
{
	int err = 0;
	int ret = 0;
	
	if (response->status != GOLIOTH_OK) {
		GLTH_LOGE(TAG, "Failed to receive '%s' endpoint: %d",
			  APP_STATE_DESIRED_ENDP,
		 	  response->status);
		return;
	}

	GLTH_LOG_BUFFER_HEXDUMP(TAG, payload, payload_size, GOLIOTH_DEBUG_LOG_LEVEL_DEBUG);

	/* Decode received CBOR buffer */
	ZCBOR_STATE_D(zsd, 2, payload, payload_size, 1, 0);

	struct zcbor_map_entry map_entries[] = {
		ZCBOR_TSTR_LIT_MAP_ENTRY("example_int0", zcbor_map_int32_decode, &parsed_state.example_int0),
		ZCBOR_TSTR_LIT_MAP_ENTRY("example_int1", zcbor_map_int32_decode, &parsed_state.example_int1)
	};
	
	GLTH_LOG_BUFFER_HEXDUMP(TAG, payload, payload_size, GOLIOTH_DEBUG_LOG_LEVEL_DEBUG);

	ret = zcbor_map_decode(zsd, map_entries, 2);

	if (ret < 0) {
		GLTH_LOGE(TAG, "Error parsing desired values: %d", ret);
		app_state_reset_desired();
		return;
	}

	uint8_t desired_processed_count = 0;
	uint8_t state_change_count = 0;

	if (_example_int0 != parsed_state.example_int0) {
		/* Process example_int0 */
		if ((parsed_state.example_int0 >= 0) && (parsed_state.example_int0 < 65536)) {
			GLTH_LOGD(TAG, "Validated desired example_int0 value: %"PRId32,
				  parsed_state.example_int0);

			_example_int0 = parsed_state.example_int0;
			++state_change_count;
			++desired_processed_count;
		} else if (parsed_state.example_int0 == -1) {
			GLTH_LOGD(TAG, "No change requested for example_int0");
		} else {
			GLTH_LOGE(TAG, "Invalid desired example_int0 value: %"PRId32,
				  parsed_state.example_int0);
			++desired_processed_count;
		}
	}
	if (_example_int1 != parsed_state.example_int1) {
		/* Process example_int1 */
		if ((parsed_state.example_int1 >= 0) && (parsed_state.example_int1 < 65536)) {
			GLTH_LOGD(TAG, "Validated desired example_int1 value: %"PRId32,
				  parsed_state.example_int1);
			
			_example_int1 = parsed_state.example_int1;
			++state_change_count;
			++desired_processed_count;
		} else if (parsed_state.example_int1 == -1) {
			GLTH_LOGD(TAG, "No change requested for example_int1");
		} else {
			GLTH_LOGE(TAG, "Invalid desired example_int1 value: %"PRId32,
				  parsed_state.example_int1);
			++desired_processed_count;
		}
	}

	if (state_change_count) {
		/* The state was changed, so update the state on the Golioth servers */
		err = app_state_update_actual();
	}
	if (desired_processed_count) {
		/* We processed some desired changes to return these to -1 on the server
		 * to indicate the desired values were received.
		 */
		err = app_state_reset_desired();
	}

	if (err) {
		GLTH_LOGE(TAG, "Failed to update cloud state: %d", err);
	}
}

int app_state_observe(struct golioth_client *state_client)
{
	int err;

	client = state_client;

	err = golioth_lightdb_observe_async(client,
					    APP_STATE_DESIRED_ENDP,
					    GOLIOTH_CONTENT_TYPE_CBOR,
					    app_state_desired_handler,
					    NULL);
	if (err) {
		GLTH_LOGW(TAG, "failed to observe lightdb path: %d", err);
		return err;
	}

	/* This will only run once. It updates the actual state of the device
	 * with the Golioth servers. Future updates will be sent whenever
	 * changes occur.
	 */
	err = app_state_update_actual();
	if (err) {
		GLTH_LOGW(TAG, "Failed to Update Desired %d", err);
		return err;
	}

	return err;
}

