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
 * @file dns.c
 * @brief sippak DNS helper
 *
 * @author Stas Kobzar <stas.kobzar@modulis.ca>
 */

#include <pjsip.h>
#include <pjlib.h>
#include <resolv.h>

#include "sippak.h"

#define NAME "dns_helper"

static int get_port (pj_str_t server)
{
  pj_ssize_t found_idx = 0;
  pj_str_t token;
  int port = 53; // default DNS port is 53
  pj_str_t delim = pj_str(":");

  found_idx = pj_strtok (&server, &delim, &token, 0);
  found_idx = pj_strtok (&server, &delim, &token, found_idx + token.slen);

  if (found_idx == server.slen) {
    // port in server string is not found
    return port; // return default port
  }

  port = pj_strtol (&token);

  return port;
}

static char *get_server_ip (pj_str_t *server)
{
  pj_ssize_t found_idx = 0;
  pj_str_t token;
  pj_str_t delim = pj_str(":");

  found_idx = pj_strtok (server, &delim, &token, 0);
  return token.ptr;
}

int sippak_get_ns_list (struct sippak_app *app, pj_str_t *ns, pj_uint16_t *ports)
{
  pj_ssize_t found_idx = 0;
  pj_str_t token;
  int ns_idx = 0;
  pj_str_t ns_str= pj_str(app->cfg.nameservers);
  pj_str_t delim = pj_str(",");

  for (
      found_idx = pj_strtok (&ns_str, &delim, &token, 0);
      found_idx != ns_str.slen;
      found_idx = pj_strtok (&ns_str, &delim, &token, found_idx + token.slen),
      ns_idx++
      )
  {
    char *srv_ip = get_server_ip(&token);
    printf("================= srv_ip %s\n",  srv_ip);
    ns[ns_idx] = pj_strdup3(app->pool, srv_ip);
    ports[ns_idx] = get_port(token);
  }

  return ns_idx;
}

pj_status_t sippak_set_resolver_ns(struct sippak_app *app)
{
  pj_status_t status;
  pj_dns_resolver *resv;
  char addr_str[PJ_INET_ADDRSTRLEN];
  pj_str_t nameservers[MAX_NS_COUNT];
  pj_uint16_t ports[MAX_NS_COUNT];
  unsigned serv_num = 0;
  pj_str_t *dst;

  printf("=============> _PATH_RESCONF " _PATH_RESCONF " \n");

  if (res_init() == -1) {
    PJ_LOG(1, (NAME, "failed to init resolv lib"));
    return PJ_ERESOLVE;
  }

  status = pjsip_endpt_create_resolver(app->endpt, &resv);
  if (status != PJ_SUCCESS) return status;

  PJ_LOG(3, (NAME, "Found name servers %d, max allowed server %d",
        _res.nscount, MAX_NS_COUNT));

  for (unsigned i = 0; i < (_res.nscount > MAX_NS_COUNT ? MAX_NS_COUNT : _res.nscount); i++) {
    if (_res.nsaddr_list[i].sin_family != pj_AF_INET()) {
      PJ_LOG(3, (NAME, "skip non IPv4 name server"));
      continue;
    }

    pj_inet_ntop (pj_AF_INET(), &_res.nsaddr_list[i].sin_addr, addr_str, sizeof(addr_str));

    nameservers[serv_num] = pj_strdup3(app->pool, addr_str);
    ports[serv_num] = pj_ntohs(_res.nsaddr_list[i].sin_port);

    serv_num++;
  }

  // debug name servers
  for (unsigned i = 0; i < serv_num; i++) {
    PJ_LOG(3, (NAME, "Name server #%d: %.*s:%d", i + 1, nameservers[i].slen, nameservers[i].ptr, ports[i]));
  }
  status = pj_dns_resolver_set_ns(resv, 1, nameservers, ports);
  if (status != PJ_SUCCESS) return status;

  return pjsip_endpt_set_resolver(app->endpt, resv);
}
