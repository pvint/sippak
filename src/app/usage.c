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
 * @file usage.c
 * @brief sippak usage print.
 *
 * @author Stas Kobzar <stas.kobzar@modulis.ca>
 */

#include "sippak.h"

PJ_DEF(void) usage ()
{
  version ();
  puts("Usage: " PROJECT_NAME " [COMMAND] [OPTIONS] [DESTINATION]");
  puts("");
  puts("  COMMAND:");
  puts("  Default commans is \"PING\".");
  puts("    PING      Send OPTIONS packet to destination.");
  puts("    PUBLISH   Send PUBLISH events and status. Default document is 'pidf' and default event is 'presence'.");
  puts("    SUBSCRIBE Send SUBSCRIBE request. Default event is 'presence'");
  puts("    NOTIFY    Send NOTIFY request. Default event is 'keep-alive'");
  puts("    REGISTER  AOR contacts list, register or unregister.");
  puts("    REFER     Send REFER method outside dialog. Implements click-to-dial scenario as in RFC5359 #2.18.");
  puts("              This command requires parameter --to for Refer-To header.");
  puts("    MESSAGE   Send MESSAGE method with text. SIP instant messaging.");
  puts("    INVITE    Initiates and handles INVITE session. After session is confirmed (200) sends BYE.");

  puts("");
  puts("  OPTIONS:");
  puts("    -h, --help      Print this usage message and exit.");
  puts("    -V, --version   Print version and exit.");
  puts("    -v, --verbose   Verbosity increase. Short option can be repeated multiple times.");
  puts("                    Long option can have value. Example: --verbose=6");
  puts("    -q, --quiet     Silent or quiet mode. Mute sippak.");
printf("    --ns=LIST       Define DNS nameservers to use. Comma separated list up to %d servers.\n", MAX_NS_COUNT);
  puts("                    Can be defined with ports. If ports are not defined will use default port 53.");
  puts("                    For example: --ns=8.8.8.8 or --ns=4.4.4.4:553,3.3.3.3");
  puts("    --color         Enable colorized output. Disabled by default.");
  puts("    --trail-dot     Output trailing dot '.' at the end of each SIP message line.");
  puts("    --log-time      Print time and microseconds in logs.");
  puts("    --log-level     Print log level: ERROR, INFO etc.");
  puts("    --log-snd       Print log sender file or module name.");

  puts("    -P, --local-port=PORT");
  puts("                    Bind local port. Default is random port.");
  puts("    -l, --local-host=HOST|IP");
  puts("                    Bind local hostname or IP. Default is first available local inface.");
  puts("    -u, --username=USER");
  puts("                    Username part in Authentication as well as in Contact and");
  puts("                    From header URI. Default is from destination URI.");
  puts("    -p, --password=PASS");
  puts("                    Password for authentication.");
  puts("    -c, --contact=CONTACT");
  puts("                    Custom contact header value. Must be valid SIP URI. For example: sip:alice@10.10.10.1:123");
  puts("                    If not set, contact header is generated automatically.");
  puts("    -F, --from-name=DISPLAY_NAME");
  puts("                    Display name in From header. Default is empty.");
  puts("    -t, --proto=PROTO");
  puts("                    Transport protocol to use. Possible values 'tcp' or 'udp'. Default is 'udp'.");
  puts("    -X, --expires=NUMBER");
  puts("                    Expires header value. Must be number more then 0.");
  puts("    --pres-status=STATUS");
  puts("                    Presence status for PUBLISH command. STATUS value can be 'open', 'closed' etc.");
  puts("                    If this parameter is not defined or invalid, will use 'open' status.");
  puts("    --pres-note=MESSAGE");
  puts("                    Presence note message string for PUBLISH command.");
  puts("    -C, --content-type=TYPE");
  puts("                    Publish or notify content type. TYPE values can be pidf, xpidf or mwi.");
  puts("                    Note: XPIDF implementation is not complete in pjproject.");
  puts("    -E, --event=EVENT");
  puts("                    Presence event header for subscribe, publish or notify methods.");
  puts("                    EVENT values can be \"presence\", \"message-summary\", \"keep-alive\" etc.");
  puts("                    For convinence, there is an alias \"mwi\" can be used for \"message-summary\" event.");
  puts("                    For PUBLISH and SUBSCRIBE method default value is 'presence'.");
  puts("                    For NOTIFY method default value is 'keep-alive'.");
  puts("    -M, --mwi=N,N,N,N");
  puts("                    Voice messages list. Comma separated list of numbers.");
  puts("                    List of messages new,old,urgent_new,urgent_old.");
  puts("                    List from 1 to 4 members. Not set members will be initiated with 0.");
  puts("    --mwi-acc=ACCOUNT");
  puts("                    Voicemail account for message-summary body. If not set then destination URI used.");
  puts("    --clist");
  puts("                    Flag for REGISTER method to get list of contacts registered for AOR.");
  puts("    --cancel-all");
  puts("                    Flag for REGISTER method to cancel all registrations for AOR.");
  puts("    --cancel");
  puts("                    Cancel registarations for REGISTER or session for INVITE.");
  puts("                    When used with REGISTER method then cancels contact registration for AOR.");
  puts("                    Contact field can be set with --contact option or will be generated.");
  puts("                    When used with INVITE will cancel session in early state.");
  puts("    --to=SIP_URI");
  puts("                    Parameter for REFER command to setup Refer-To header value.");
  puts("    --body=TEXT");
  puts("                    Parameter for MESSAGE command to setup message body text.");
  puts("    --codec=LIST");
  puts("                    Set codec or codecs to use during INVITE session. Value can be");
  puts("                    a single codec name or comma-separated list of codecs. By default");
  puts("                    sippak will use codec g711. Codec names are case-insensitive.");
  puts("                    Following codec names can be used:");
  puts("                    speex,ilbc,gsm,g711,g722,ipp,l16,amr,silk,opus,bcg729,all");
  puts("                    When set to \"all\" will try to setup all available codecs for media.");
  puts("    --rtp-port=PORT");
  puts("                    Port to use for media streams and negotiate with SDP.");
  puts("    -A, --user-agent=STRING");
  puts("                    Set User-Agent SIP header value.");
  puts("    -H, --header=HEADER");
  puts("                    Add custom header to request. Multiple custom headers (up to 12) can be added.");
  puts("                    Parameter value must contain header name and value separated by colon.");
  puts("                    Examples:");
  puts("                    --header=\"X-Foo: bar\", -H X-Foo:bar, -H \"X-Assert: sip:123@sip.com\"");
  puts("    -R, --proxy=PROXY");
  puts("                    Add proxy to request. Multiple proxies (up to 12) can be added.");
  puts("                    The first proxy will be used as outbound proxy where the request will be sent.");
  puts("                    All additional proxies will be added as Route headers to the request.");
  puts("                    Supported methods are INVITE, REGISTER and PUBLISH.");
  puts("                    Examples:");
  puts("                    --proxy=sip:sip.com:2585, -R sip:10.23.24.100:6060;lr");
  puts("");
}

PJ_DEF(void) version ()
{
  puts(PROJECT_NAME " " PROJECT_VERSION " (" PROJECT_ARCH ") " LICENSE);
  puts("built with pjproject " PJSIP_VERSION ", at " PROJECT_BUILTTIME);
  puts("(c) 2018 " PROJECT_AUTHOR);
  puts("project site and bug report: " PROJECT_URL);
  puts("");
}
