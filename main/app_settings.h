#ifndef __APP_SETTINGS_H__
#define __APP_SETTINGS_H__

#include <stdint.h>
#include <golioth/client.h>

int32_t get_loop_delay_s(void);
int app_settings_register(struct golioth_client *client);

#endif /* __APP_SETTINGS_H__ */
