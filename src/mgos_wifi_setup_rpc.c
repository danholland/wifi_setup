/*
 * Copyright (c) 2019 Dan Holland
 * Copyright (c) 2014-2018 Cesanta Software Limited (Scan)
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
#include "mgos.h"
#include "mgos_rpc.h"
#include "mgos_wifi.h"
#include "mgos_webserver_gzip.h"
#include "mgos_timers.h"
#include <tcpip_adapter.h>

#if CS_PLATFORM == CS_P_ESP32
#include "esp_wifi.h"
#endif

char *s_ssid = NULL;
char *s_pass = NULL;
bool b_wifi_setup_rpc_init = false;
static mgos_timer_id s_wifi_timer_id = MGOS_INVALID_TIMER_ID;
int i_counter = 0;

static void check_connected_cb(void *arg)
{
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *)arg;
  enum mgos_wifi_status st = mgos_wifi_get_status();
  if (st == MGOS_WIFI_CONNECTED)
  {
    mg_rpc_send_responsef(ri, "{ status: %d, response: %Q }", 200, "completed");
    i_counter = 0;
    mgos_clear_timer(s_wifi_timer_id);
    s_wifi_timer_id = MGOS_INVALID_TIMER_ID;
    return;
  }
  int timeout = mgos_sys_config_get_wifi_rpc_timeout() ? mgos_sys_config_get_wifi_rpc_timeout() : 30;
  if (i_counter > timeout)
  {
    mgos_clear_timer(s_wifi_timer_id);
    s_wifi_timer_id = MGOS_INVALID_TIMER_ID;
    LOG(LL_ERROR, ("WiFi connection timed out"));
    mg_rpc_send_errorf(ri, -1, "WiFi connection timed out");
    i_counter = 0;
  }
  else
  {
    LOG(LL_INFO, ("%d", i_counter));
    i_counter++;
  }
  return;
}

static void mgos_wifi_setup_connect_rpc_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                                struct mg_rpc_frame_info *fi,
                                                struct mg_str args)
{
  LOG(LL_INFO, ("Wifi.Connect RPC Handler Parsing JSON: %.*s\n", args.len, args.p));
  struct mgos_config_wifi_sta cfg = {0};
  json_scanf(args.p, args.len, ri->args_fmt, &cfg.ssid, &cfg.pass, &cfg.user, &cfg.ip, &cfg.netmask, &cfg.gw, &cfg.nameserver);
  cfg.enable = true;
  char *msg = NULL;
  if (!mgos_wifi_validate_sta_cfg(&cfg, &msg))
  {
    free((char *)cfg.ssid);
    free((char *)cfg.pass);
    free((char *)cfg.user);
    mg_rpc_send_errorf(ri, -1, msg);
    free(msg);
    return false;
  }
  mgos_wifi_setup_sta(&cfg);

  s_wifi_timer_id = mgos_set_timer(1000, MGOS_TIMER_REPEAT, check_connected_cb, ri);
  free((char *)cfg.ssid);
  free((char *)cfg.pass);
  free((char *)cfg.user);
  (void)cb_arg;
  (void)fi;
}

static void mgos_wifi_setup_get_info_rpc_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                                 struct mg_rpc_frame_info *fi,
                                                 struct mg_str args)
{

  tcpip_adapter_ip_info_t ipInfo;
  tcpip_adapter_dns_info_t dnsInfo1;
  tcpip_adapter_dns_info_t dnsInfo2;

  // IP address.
  tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

  // DNS Server
  tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_STA, TCPIP_ADAPTER_DNS_MAIN, &dnsInfo1);
  tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_STA, TCPIP_ADAPTER_DNS_BACKUP, &dnsInfo2);

  char strIp[20];
  char strNm[20];
  char strGw[20];
  char strDns1[20];
  char strDns2[20];

  sprintf(strIp, IPSTR, IP2STR(&ipInfo.ip));
  sprintf(strNm, IPSTR, IP2STR(&ipInfo.netmask));
  sprintf(strGw, IPSTR, IP2STR(&ipInfo.gw));
  sprintf(strDns1, IPSTR, IP2STR(ip_2_ip4(&dnsInfo1.ip)));
  sprintf(strDns2, IPSTR, IP2STR(ip_2_ip4(&dnsInfo2.ip)));
  mg_rpc_send_responsef(ri, "{ status: %d, ip: %Q, netmask: %Q, gw: %Q, dns1: %Q, dns2: %Q }", 200, strIp, strNm, strGw, strDns1, strDns2);
}

esp_err_t mgos_wifi_setup_rpc_start()
{
  LOG(LL_INFO, ("Wifi RPC handlers start"));
  if (!b_wifi_setup_rpc_init)
  {
    if (mgos_sys_config_get_wifi_rpc_apsta())
    {
// Set AP+STA mode if device is ESP32
#if CS_PLATFORM == CS_P_ESP32
      LOG(LL_INFO, ("Forcing AP+STA mode"));
      esp_wifi_set_mode(WIFI_MODE_APSTA);
#endif
    }

    struct mg_rpc *c = mgos_rpc_get_global();
    mg_rpc_add_handler(c, "Wifi.Connect", "{ssid: %Q, pass: %Q, user: %Q}", mgos_wifi_setup_connect_rpc_handler, NULL);
    mg_rpc_add_handler(c, "Wifi.GetInfo", NULL, mgos_wifi_setup_get_info_rpc_handler, NULL);
    b_wifi_setup_rpc_init = true;
    return ESP_OK;
  }

  return ESP_FAIL;
}

bool mgos_wifi_setup_rpc_init(void)
{
  LOG(LL_INFO, ("Wifi RPC handlers init"));
  if (mgos_sys_config_get_wifi_rpc_enable())
  {
    mgos_wifi_setup_rpc_start();
  }
  return true;
}