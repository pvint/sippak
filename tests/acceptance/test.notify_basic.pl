#
# Acceptence test:
# Run sippak NOTIFY command with presence info and check-sync
#

use strict;
use warnings;
use Cwd qw(abs_path);
use File::Basename;
use Test::More;

my $sippak = $ARGV[0];
my $sipp   = $ARGV[1];
my $scenario = $ARGV[2] . "/notify.basic.xml";
my $sippargs = "-timeout 10s -p 5060 -m 1 -bg";
my $output = "";
my $regex  = "";

# run sipp scenario in background mode
system("$sipp $sippargs -sf $scenario");
if ($? == -1) {
  print "Failed execute sipp\n";
  exit(1);
}

# run sippak notify basic presence with pidf
$output = `$sippak notify sip:alice\@127.0.0.1:5060`;

# test request
$regex = '^NOTIFY sip:alice\@127\.0\.0\.1:5060 SIP\/2.0$';
ok ($output =~ m/$regex/m, "Basic NOTIFY packet sent.");

$regex = '^SIP\/2\.0 200 OK Basic NOTIFY Test$';
ok ($output =~ m/$regex/m, "Basic NOTIFY response displayed.");

$regex = '^Event: keep-alive$';
ok ($output =~ m/$regex/m, "Default event is 'keep-alive'");

# special events
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify -E foo sip:alice\@127.0.0.1:5060`;

$regex = '^Event: foo$';
ok ($output =~ m/$regex/m, "Arbitrary event header 'foo'.");

system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify --event=check-sync sip:alice\@127.0.0.1:5060`;

$regex = '^Event: check-sync$';
ok ($output =~ m/$regex/m, "Event header 'check-sync'.");

system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify --event=mwi sip:alice\@127.0.0.1:5060`;

$regex = '^Event: message-summary$';
ok ($output =~ m/$regex/m, "Event header 'message-summary' for alias 'mwi'.");

# custom content type 
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify --event=foo --content-type=bar sip:alice\@127.0.0.1:5060`;

$regex = '^Event: foo$';
ok ($output =~ m/$regex/m, "Custom notify with content type and event 'foo'");

$regex = '^Content-Type: application/bar$';
ok ($output =~ m/$regex/m, "Custom notify with content type 'bar' and event 'foo'");

# custom compound content type
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify --content-type=foo/bar sip:alice\@127.0.0.1:5060`;

$regex = '^Content-Type: foo/bar$';
ok ($output =~ m/$regex/m, "Custom notify with compound content type 'foo/bar'");

# message-summary notify
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify --mwi=0 sip:alice\@127.0.0.1:5060`;

$regex = '^Messages-Waiting: no';
ok ($output =~ m/$regex/m, "MWI basic no message waiting.");

$regex = '^Message-Account: sip:alice@127.0.0.1:5060';
ok ($output =~ m/$regex/m, "MWI basic message account use RURI.");

$regex = '^Voice-Message: 0/0 \(0/0\)';
ok ($output =~ m/$regex/m, "MWI basic voice message is 0/0 (0/0).");

# message-summary notify with account name
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify -M 4,0,1,2 --mwi-acc=sip:VM\@sip.com sip:alice\@127.0.0.1:5060`;

$regex = '^Messages-Waiting: yes';
ok ($output =~ m/$regex/m, "MWI basic with messages waiting.");

$regex = '^Message-Account: sip:VM@sip.com';
ok ($output =~ m/$regex/m, "MWI basic message account set mwi-acc.");

$regex = '^Voice-Message: 4/0 \(1/2\)';
ok ($output =~ m/$regex/m, "MWI basic voice message is 0/0 (0/0).");

# presence pidf basic
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify -C pidf sip:alice\@127.0.0.1:5060`;

$regex = '^Event: presence$';
ok ($output =~ m/$regex/m, "NOTIFY PIDF event header is presence");

$regex = '<basic>open</basic>';
ok ($output =~ m/$regex/m, "NOTIFY default PIDF status is open");

$regex = '^Content-Type: application/pidf\+xml$';
ok ($output =~ m/$regex/m, "NOTIFY PIDF content type is application/pidf+xml");

# presence pidf with status closed
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify -C pidf --pres-status=closed sip:alice\@127.0.0.1:5060`;

$regex = '<basic>closed</basic>';
ok ($output =~ m/$regex/m, "NOTIFY PIDF with status set as closed");

# presence pidf with note
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify -C pidf --pres-note="Out for lunch" sip:alice\@127.0.0.1:5060`;

$regex = '<basic>open</basic>';
ok ($output =~ m/$regex/m, "NOTIFY PIDF with note and default status is open");

$regex = '<note>Out for lunch</note>';
ok ($output =~ m/$regex/m, "NOTIFY PIDF doc note is set");

# presence XPIDF
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify -C xpidf sip:alice\@127.0.0.1:5060`;

$regex = '^Event: presence$';
ok ($output =~ m/$regex/m, "NOTIFY XPIDF event header is presence");

$regex = '^Content-Type: application/xpidf\+xml$';
ok ($output =~ m/$regex/m, "NOTIFY XPIDF document type");

# Header User-Agent add
system("$sipp $sippargs -sf $scenario");
$output = `$sippak notify -A "SIP UA 1.x.x" sip:alice\@127.0.0.1:5060`;

$regex = '^User-Agent: SIP UA 1.x.x$';
ok ($output =~ m/$regex/m, "Add User-Agent header.");

done_testing();
