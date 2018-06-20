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
 * @file ping.c
 * @brief sippak INVITE session
 *
 * @author Stas Kobzar <stas.kobzar@modulis.ca>
 */
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjsip_ua.h>
#include "sippak.h"

#define NAME "mod_invite"

static pj_bool_t on_rx_response (pjsip_rx_data *rdata);

static void call_on_state_changed( pjsip_inv_session *inv, pjsip_event *e);
static void call_on_forked(pjsip_inv_session *inv, pjsip_event *e);
static void call_tsx_state_changed(pjsip_inv_session *inv, pjsip_transaction *tsx, pjsip_event *e);

static pj_bool_t early_cancel;
static pjsip_inv_session *inv;

static pjsip_module mod_invite =
{
  NULL, NULL,                 /* prev, next.    */
  { "mod-sippak-inv", 14 },   /* Name. Name mod-invite is reserved by pjsip   */
  -1,                         /* Id      */
  PJSIP_MOD_PRIORITY_TSX_LAYER - 1, /* Priority  */
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
  pjsip_tx_data *tdata;
  pj_status_t status;
  pjsip_msg *msg = rdata->msg_info.msg;
  if (msg->type != PJSIP_RESPONSE_MSG)
    return PJ_FALSE;

  if (msg->line.status.code == 487) {
    /*
     * Could not find other way to ACK 487 response.
     * Handle 487 manually for now.
     * TODO: find a way to handle 487 pjsip way
     * TODO: ACK Cseq must match INVITE CSeq
     */
    status = pjsip_inv_create_ack(inv, -1, &tdata);
    if (status==PJ_SUCCESS && tdata) {
      pjsip_inv_send_msg(inv, tdata);
    }
    sippak_loop_cancel();
  }
  return PJ_FALSE; // continue with othe modules
}

static void call_tsx_state_changed(pjsip_inv_session *inv, pjsip_transaction *tsx, pjsip_event *e)
{
  PJ_UNUSED_ARG(inv);
  PJ_UNUSED_ARG(tsx);
  PJ_UNUSED_ARG(e);
  // printf("===========================> call_tsx_state_changed state: %s\n", pjsip_inv_state_name(inv->state));
}

static void call_on_state_changed( pjsip_inv_session *inv, pjsip_event *e)
{
  PJ_UNUSED_ARG(e);
  pj_status_t status;
  pjsip_tx_data *tdata;

  if (inv->state == PJSIP_INV_STATE_DISCONNECTED) {

    PJ_LOG(3, (NAME, "Call completed."));
    sippak_loop_cancel();

  } else if (inv->state == PJSIP_INV_STATE_EARLY) {

    if(early_cancel == PJ_TRUE) {
      PJ_LOG(3, (NAME, "Cancel session in early state (%d)", inv->cause));
      status = pjsip_inv_end_session(inv, PJSIP_SC_REQUEST_TERMINATED, NULL, &tdata);
      if (status==PJ_SUCCESS && tdata != NULL) { // tdata is null when not provisioning yet received
        pjsip_inv_send_msg(inv, tdata);
      }
    }

  } else if (inv->state == PJSIP_INV_STATE_CONFIRMED) {

    PJ_LOG(3, (NAME, "Call confirmed. Now terminating with BYE."));
    status = pjsip_inv_end_session(inv, PJSIP_SC_OK, NULL, &tdata);
    if (status==PJ_SUCCESS && tdata) {
      pjsip_inv_send_msg(inv, tdata);
    }

  }
}

static void call_on_forked(pjsip_inv_session *inv, pjsip_event *e)
{
  PJ_UNUSED_ARG(e);
  PJ_UNUSED_ARG(inv);
}

/* Ping */
PJ_DEF(pj_status_t) sippak_cmd_invite (struct sippak_app *app)
{
  pj_status_t status;
  pjsip_dialog *dlg = NULL;
  pjsip_tx_data *tdata;
  pj_str_t *local_addr;
  int local_port;
  pj_str_t cnt, from, ruri;
  pjsip_inv_callback inv_cb;
  pjsip_cred_info cred[1];
  pjmedia_sdp_session *sdp_sess;
  pjsip_route_hdr *route_set;

  early_cancel = app->cfg.cancel;

  status = sippak_transport_init(app, &local_addr, &local_port);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate transport.");

  status = pjsip_tsx_layer_init_module(app->endpt);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate transaction layer.");

  status = pjsip_ua_init_module(app->endpt, NULL);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate UA module.");

  /* Initialize 100rel support */
  status = pjsip_100rel_init_module(app->endpt);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate 100rel module.");

  /* invite usage module */
  pj_bzero(&inv_cb, sizeof(inv_cb));
  inv_cb.on_state_changed = &call_on_state_changed;
  inv_cb.on_new_session = &call_on_forked;
  inv_cb.on_tsx_state_changed = &call_tsx_state_changed;
  status = pjsip_inv_usage_init(app->endpt, &inv_cb);
  SIPPAK_ASSERT_SUCC(status, "Failed to set invite callback usage functions.");

  status = pjsip_endpt_register_module(app->endpt, &mod_invite);
  SIPPAK_ASSERT_SUCC(status, "Failed to register module mod_invite.");

  cnt  = sippak_create_contact_hdr(app, local_addr, local_port);
  from = sippak_create_from_hdr(app);
  ruri = sippak_create_ruri(app);

  status = pjsip_dlg_create_uac(pjsip_ua_instance(),
      &from, &cnt, &ruri, &ruri, &dlg);
  SIPPAK_ASSERT_SUCC(status, "Failed to create dialog uac.");

  /* auth credentials */
  sippak_set_cred(app, cred);
  status = pjsip_auth_clt_set_credentials(&dlg->auth_sess, 1, cred);
  SIPPAK_ASSERT_SUCC(status, "Failed to set auth credentials.");

  /* SDP */
  status = sippak_set_media_sdp (app, &sdp_sess);
  SIPPAK_ASSERT_SUCC(status, "Failed to set media SDP.");

  /* invite session */
  status = pjsip_inv_create_uac( dlg, sdp_sess, 0, &inv);
  SIPPAK_ASSERT_SUCC(status, "Failed to create invite UAC.");

  /* outbound proxy */
  if (sippak_set_proxies_list(app, &route_set) == PJ_TRUE) {
    pjsip_dlg_set_route_set(dlg, route_set);
  }

  /* create invite request */
  status = pjsip_inv_invite(inv, &tdata);
  SIPPAK_ASSERT_SUCC(status, "Failed to create invite request.");

  /* send invite */
  return pjsip_inv_send_msg(inv, tdata);
}
