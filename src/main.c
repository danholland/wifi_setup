#include "mgos.h"
#include "mgos_wifi_setup_rpc.h"

enum mgos_app_init_result mgos_app_init(void)
{
  mgos_wifi_setup_rpc_init();
  return MGOS_APP_INIT_SUCCESS;
}
