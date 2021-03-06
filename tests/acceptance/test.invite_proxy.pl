#
# Acceptence test:
# Run sippak handle proxy set
#

use strict;
use warnings;
use Cwd qw(abs_path);
use File::Basename;
use Test::More;

my $sippak = $ARGV[0];
my $sipp   = $ARGV[1];
my $scenario = $ARGV[2] . "/invite.proxy.xml";
my $sippargs = "-timeout 10s -p 5060 -m 1 -bg";
my $output = "";
my $regex  = "";

# run sipp scenario in background mode
system("$sipp $sippargs -sf $scenario");
if ($? == -1) {
  print "Failed execute sipp\n";
  exit(1);
}

# run sippak publish basic
$output = `$sippak INVITE --proxy=sip:127.0.0.1:5060 sip:alice\@pbx.corporate.big`;

# test request
$regex = '^INVITE sip:alice\@pbx.corporate.big SIP\/2\.0$';
ok ($output =~ m/$regex/m, "Basic INVITE packet sent to RURI with domain.");

$regex = '^SIP\/2.0 180 Ringing$';
ok ($output =~ m/$regex/m, "Ringing 180 early response.");

$regex = '^SIP\/2.0 200 OK Invite$';
ok ($output =~ m/$regex/m, "Invite session is confirmed.");

$regex = '^ACK sip:alice\@127\.0\.0\.1:5060 SIP\/2\.0$';
ok ($output =~ m/$regex/m, "Invite 200 in acknowlaged.");

$regex = '^BYE sip:alice\@127\.0\.0\.1:5060 SIP\/2\.0$';
ok ($output =~ m/$regex/m, "Session is terminated with BYE.");

$regex = '^SIP\/2.0 200 OK bye$';
ok ($output =~ m/$regex/m, "BYE method is confirmed.");

# test multiple proxies
system("$sipp $sippargs -sf $scenario");
$output = `$sippak INVITE --proxy=sip:127.0.0.1:5060 -R sip:foo.com:2525 -R sips:125.1.1.100:8080 sip:alice\@pbx.corporate.big`;

$regex = '^Route: <sip:127.0.0.1:5060;lr>$';
ok ($output =~ m/$regex/m, "First proxy with loose route parameter");

$regex = '^Route: <sip:foo.com:2525>$';
ok ($output =~ m/$regex/m, "Second proxy as Router header.");

$regex = '^Route: <sips:125.1.1.100:8080>$';
ok ($output =~ m/$regex/m, "Third proxy as Router header");

done_testing();
