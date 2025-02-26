/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <golioth/client.h>
#include <golioth/settings.h>
#include "app_settings.h"
#include "golioth/golioth_sys.h"

static int32_t _loop_delay_s = 60;
#define LOOP_DELAY_S_MAX 43200
#define LOOP_DELAY_S_MIN 0

#define TAG "app_settings"

int32_t get_loop_delay_s(void)
{
	return _loop_delay_s;
}

static enum golioth_settings_status on_loop_delay_setting(int32_t new_value, void *arg)
{
	GLTH_LOGI(TAG, "Setting loop delay to %" PRId32 " s", new_value);
	_loop_delay_s = new_value;
	return GOLIOTH_SETTINGS_SUCCESS;
}


int app_settings_register(struct golioth_client *client)
{
	struct golioth_settings *settings = golioth_settings_init(client);

	int err = golioth_settings_register_int_with_range(settings,
							   "LOOP_DELAY_S",
							   LOOP_DELAY_S_MIN,
							   LOOP_DELAY_S_MAX,
							   on_loop_delay_setting,
							   NULL);

	if (err) {
		GLTH_LOGE(TAG, "Failed to register settings callback: %d", err);
	}

	return err;
}
