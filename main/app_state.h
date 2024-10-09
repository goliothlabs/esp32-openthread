#ifndef __APP_STATE_H__
#define __APP_STATE_H__

#include <golioth/client.h>
#include <stdint.h>

#define APP_STATE_DESIRED_ENDP "desired"
#define APP_STATE_ACTUAL_ENDP  "state"

struct app_state {
	int32_t example_int0;
	int32_t example_int1;
};

int app_state_observe(struct golioth_client *state_client);
int app_state_update_actual(void);

#endif /* __APP_STATE_H__ */

