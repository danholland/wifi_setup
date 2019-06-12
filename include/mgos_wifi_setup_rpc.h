#include "mgos_rpc.h"

typedef void (*wifi_setup_test_cb_t)(bool result, char *ssid, char *password, void *userdata);
#define MGOS_WIFI_SETUP_EV_BASE MGOS_EVENT_BASE('W', 'F', 'S')

esp_err_t mgos_wifi_setup_rpc_start();

bool mgos_wifi_setup_rpc_init(void);