/**
 * sippak -- SIP command line utility.
 * Copyright (C) 2018, Stas Kobzar <staskobzar@modulis.ca>
 *
 * This file is part of sippak.
 *
 * sippak is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * sippak is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sippak.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file publish.c
 * @brief sippak PUBLISH message send
 *
 * @author Stas Kobzar <stas.kobzar@modulis.ca>
 */
#include <pjlib-util.h> // needed for return value like PJ_CLI_EINVARG
#include <pjsip-simple/publish.h>
#include <pjsip-simple/presence.h>
#include "sippak.h"

#define NAME "mod_publish"

static pj_bool_t on_rx_response (pjsip_rx_data *rdata);
static short unsigned auth_tries = 0;
static int pj_str_toi(pj_str_t val);

static pjsip_module mod_publish =
{
  NULL, NULL,                 /* prev, next.    */
  { "mod-publish", 11 },      /* Name.    */
  -1,                         /* Id      */
  PJSIP_MOD_PRIORITY_TSX_LAYER - 1, /* Priority */
  NULL,                       /* load()    */
  NULL,                       /* start()    */
  NULL,                       /* stop()    */
  NULL,                       /* unload()    */
  NULL,                       /* on_rx_request()  */
  &on_rx_response,            /* on_rx_response()  */
  NULL,                       /* on_tx_request.  */
  NULL,                       /* on_tx_response()  */
  NULL,                       /* on_tsx_state()  */
};

/* On response module callback */
static pj_bool_t on_rx_response (pjsip_rx_data *rdata)
{
  pjsip_msg *msg = rdata->msg_info.msg;

  auth_tries++;

  if (msg->type != PJSIP_RESPONSE_MSG) {
    return PJ_FALSE;
  }

  if (msg->line.status.code == PJSIP_SC_PROXY_AUTHENTICATION_REQUIRED ||
      msg->line.status.code == PJSIP_SC_UNAUTHORIZED) {
    if (auth_tries > 1) {
      sippak_loop_cancel();
      PJ_LOG(1, (NAME, "Authentication failed. Check your username and password"));
      return PJ_TRUE;
    }
  }
  return PJ_FALSE; // continue with othe modules
}

static void publish_cb(struct pjsip_publishc_cbparam *param)
{
  PJ_LOG(3, (NAME, "Response received: %d %.*s",
        param->code,
        param->reason.slen,
        param->reason.ptr));
  sippak_loop_cancel();
}

PJ_DEF(pj_status_t) sippak_cmd_publish (struct sippak_app *app)
{
  pj_status_t status;
  pj_str_t *local_addr;
  int local_port;
  pj_str_t target_uri;
  pj_str_t from_uri;
  pj_str_t pres_id;

  pjsip_publishc  *publish_sess = NULL;
  pjsip_publishc_opt publish_opt;
  pjsip_tx_data *tdata;
  pjsip_route_hdr *route_set;
  pjsip_pres_status pres_status;
  pjsip_cred_info cred[1];
  pj_str_t event;

  if (app->cfg.event.slen == 0) {
    event = pj_str("presence");
  } else{
    event = app->cfg.event;
  }

  if (app->cfg.expires < 1) {
    PJ_LOG(1, (PROJECT_NAME, "Expires header value must be more then 0."));
    exit(PJ_CLI_EINVARG);
  }

  status = sippak_transport_init(app, &local_addr, &local_port);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate transport.");

  target_uri = sippak_create_ruri(app); // also as To header URI
  from_uri = sippak_create_from_hdr(app); // also entity
  pres_id = sippak_create_contact_hdr(app, local_addr, local_port);

  // set presence status
  pj_bzero(&pres_status, sizeof(pres_status));
  pres_status.info_cnt = 1;
  pres_status.info[0].basic_open = app->cfg.pres_status_open;
  pres_status.info[0].id = pres_id;
  pres_status.info[0].rpid.note = app->cfg.pres_note;
  pres_status.info[0].rpid.activity = app->cfg.pres_status_open
    ? PJRPID_ACTIVITY_UNKNOWN
    : PJRPID_ACTIVITY_BUSY;

  pjsip_publishc_opt_default(&publish_opt);

  status = pjsip_publishc_create(app->endpt, &publish_opt, NULL, &publish_cb, &publish_sess);
  SIPPAK_ASSERT_SUCC(status, "Failed to create publish client.");

  status = pjsip_publishc_init(publish_sess, &event, &target_uri, &from_uri, &target_uri, app->cfg.expires);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate publish client.");

  sippak_set_cred(app, cred);

  if (sippak_set_proxies_list(app, &route_set) == PJ_TRUE) {
    pjsip_publishc_set_route_set (publish_sess, route_set);
  }

  status = pjsip_publishc_set_credentials(publish_sess, 1, cred);
  SIPPAK_ASSERT_SUCC(status, "Failed to set auth credentials.");

  status = pjsip_publishc_publish(publish_sess, PJ_TRUE, &tdata);
  SIPPAK_ASSERT_SUCC(status, "Failed to create publish client.");

  if (app->cfg.ctype_e == CTYPE_XPIDF) {
    status = pjsip_pres_create_xpidf(tdata->pool, &pres_status, &pres_id, &tdata->msg->body);
    SIPPAK_ASSERT_SUCC(status, "Failed to create XPIDF publish body.");
  } else if (app->cfg.ctype_e == CTYPE_PIDF) {
    status = pjsip_pres_create_pidf(tdata->pool, &pres_status, &pres_id, &tdata->msg->body);
    SIPPAK_ASSERT_SUCC(status, "Failed to create PIDF publish body.");
  } else {
    PJ_LOG(2, (PROJECT_NAME,
      "Content type \"%.*s\" can not be used. Fall back to pidf content type.",
      app->cfg.ctype_media.subtype.slen, app->cfg.ctype_media.subtype.ptr));
    status = pjsip_pres_create_pidf(tdata->pool, &pres_status, &pres_id, &tdata->msg->body);
  }

  status = pjsip_tsx_layer_init_module(app->endpt);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate transaction layer.");

  status = pjsip_endpt_register_module(app->endpt, &mod_publish);
  SIPPAK_ASSERT_SUCC(status, "Failed to register module mod_publish.");

  return pjsip_publishc_send(publish_sess, tdata);
}
