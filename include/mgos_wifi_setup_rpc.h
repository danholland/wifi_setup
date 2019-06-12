/*
 * Copyright (c) 2019 Dan Holland
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_rpc.h"

typedef void (*wifi_setup_test_cb_t)(bool result, char *ssid, char *password, void *userdata);
#define MGOS_WIFI_SETUP_EV_BASE MGOS_EVENT_BASE('W', 'F', 'S')

esp_err_t mgos_wifi_setup_rpc_start();

bool mgos_wifi_setup_rpc_init(void);