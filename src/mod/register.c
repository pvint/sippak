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
 * @brief sippak send REGISTER request
 *
 * @author Stas Kobzar <stas.kobzar@modulis.ca>
 */
#include <pjsip_ua.h>
#include "sippak.h"

#define NAME "mod_register"

static void reg_callback(struct pjsip_regc_cbparam *param);
static void print_reg_success(struct pjsip_regc_cbparam *regp);

static pjsip_module mod_register =
{
  NULL, NULL,                 /* prev, next.    */
  { "mod-register", 12 },     /* Name.    */
  -1,                         /* Id      */
  PJSIP_MOD_PRIORITY_APPLICATION - 1, /* Priority          */
  NULL,                       /* load()    */
  NULL,                       /* start()    */
  NULL,                       /* stop()    */
  NULL,                       /* unload()    */
  NULL,                       /* on_rx_request()  */
  NULL,            /* on_rx_response()  */
  NULL,                       /* on_tx_request.  */
  NULL,                       /* on_tx_response()  */
  NULL,                       /* on_tsx_state()  */
};

static void print_reg_success(struct pjsip_regc_cbparam *regp)
{
  struct sippak_app *app = regp->token;
  if (app->cfg.cancel_all_reg == PJ_TRUE) {
    PJ_LOG(3, (NAME, "Cancel all registrations. Response received: %d %.*s",
          regp->code, regp->reason.slen, regp->reason.ptr));
  } else if (app->cfg.cancel == PJ_TRUE) {
    PJ_LOG(3, (NAME, "Unregister contact. Response received: %d %.*s",
          regp->code, regp->reason.slen, regp->reason.ptr));
  } else if (app->cfg.is_clist == PJ_TRUE) {
    PJ_LOG(3, (NAME, "Request for current contacts. Response received: %d %.*s",
          regp->code, regp->reason.slen, regp->reason.ptr));
  } else {
    PJ_LOG(3, (NAME, "Successfully registered. Response received: %d %.*s",
          regp->code, regp->reason.slen, regp->reason.ptr));
  }
  PJ_LOG(3, (NAME, "Contacts list (%d):", regp->contact_cnt));
  for(int i = 0; i < regp->contact_cnt; i++) {
    char buf[PJSIP_MAX_URL_SIZE] = {0};
    pjsip_contact_hdr *hdr = regp->contact[i];
    hdr->vptr->print_on(hdr, buf, PJSIP_MAX_URL_SIZE);
    PJ_LOG(3, (NAME, "\t%d - %s", i + 1, buf));
  }
}

static void reg_callback(struct pjsip_regc_cbparam *regp)
{
  if (regp->code == PJSIP_SC_OK) {
    print_reg_success(regp);

  } else if (regp->code == PJSIP_SC_UNAUTHORIZED) {
    PJ_LOG(1, (NAME, "Authentication failed. Check your username and password"));
  } else {
    PJ_LOG(1, (NAME, "Registration response: %d %.*s", regp->code,
          regp->reason.slen, regp->reason.ptr));
  }
  sippak_loop_cancel();
}

/* Register */
PJ_DEF(pj_status_t) sippak_cmd_register (struct sippak_app *app)
{
  pj_status_t status;
  pj_str_t *local_addr;
  int local_port;
  pjsip_tx_data *tdata;
  pjsip_regc *regc;
  pjsip_cred_info cred[1];
  pjsip_route_hdr *route_set;

  pj_str_t srv_url, from_uri, to_uri;
  pj_str_t contacts[1];

  status = sippak_transport_init(app, &local_addr, &local_port);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate transport.");

  status = pjsip_tsx_layer_init_module(app->endpt);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate transaction layer.");

  status = pjsip_endpt_register_module(app->endpt, &mod_register);
  SIPPAK_ASSERT_SUCC(status, "Failed to register module mod_register.");

  status = pjsip_regc_create(app->endpt,
      app, // void *token: A data to be associated with the client registration struct.
      &reg_callback, // Pointer to callback function to receive registration status
      &regc);
  SIPPAK_ASSERT_SUCC(status, "Failed to create register client.");

  srv_url = sippak_create_reg_ruri(app);
  to_uri = sippak_create_ruri(app);
  from_uri = sippak_create_from_hdr(app);
  contacts[0] = sippak_create_contact_hdr(app, local_addr, local_port);

  status = pjsip_regc_init(regc, &srv_url, &from_uri, &to_uri,
      app->cfg.is_clist == PJ_TRUE ? 0: 1, // no contact header, means return contacts list
      contacts, app->cfg.expires);
  SIPPAK_ASSERT_SUCC(status, "Failed to initiate register client.");

  sippak_set_cred(app, cred);
  status = pjsip_regc_set_credentials(regc, 1, cred);
  SIPPAK_ASSERT_SUCC(status, "Failed to set auth credentials.");

  if (sippak_set_proxies_list(app, &route_set) == PJ_TRUE) {
    pjsip_regc_set_route_set(regc, route_set);
  }

  if (app->cfg.cancel_all_reg == PJ_TRUE) {
    status = pjsip_regc_unregister_all(regc, &tdata);
    SIPPAK_ASSERT_SUCC(status, "Failed to cancel all registarations for %.*s.",
        from_uri.slen, from_uri.ptr);
  } else if (app->cfg.cancel == PJ_TRUE) {
    status = pjsip_regc_unregister(regc, &tdata);
    SIPPAK_ASSERT_SUCC(status, "Failed to cancel registaration for %.*s.",
        from_uri.slen, from_uri.ptr);
  } else {
    status = pjsip_regc_register(regc, PJ_FALSE, &tdata);
    SIPPAK_ASSERT_SUCC(status, "Failed to register %.*s.",
        from_uri.slen, from_uri.ptr);
  }

  return pjsip_regc_send(regc, tdata);
}
