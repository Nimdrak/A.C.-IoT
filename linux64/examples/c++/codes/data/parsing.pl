#!/usr/bin/perl -w

# t time wifi|lte seq N unackedLtePkts N unackedWifiPkts N compensation N [probe]
# r time wifi|lte seq N unackedLtePkts N unackedWifiPkts N compensation N lteDelay N wifiDelay N
# r time rrc seq N
# b time timeout seq N
# b time hole seq N

use strict;
use warnings;

die "Usage: $0 log_file" unless $#ARGV == 0; # last index

my $filePrefix = $ARGV[0];

my $cmd;
my $exit = 0;
my @mode = qw (t r);
my @link = qw (lte wifi rrc);
my %unackedIndex = ("lte" => 7, "wifi" => 9);
my %delayIndex = ("lte" => 13, "wifi" => 15);

foreach my $l (@link){
	next if ($l eq "rrc");

# UNACKED
	$cmd = "awk '{ if ((\$1 == \"t\" || \$1 == \"r\") && \$3 == \"$l\") print \$2, \$$unackedIndex{$l} }' $filePrefix > ${filePrefix}_$l.unacked";
	print $cmd, "\n";
	$exit = system $cmd;
	die "fail: $cmd" unless $exit == 0;

# DELAY
	$cmd = "awk '{ if (\$1 == \"r\" && \$3 == \"$l\") print \$2, \$$delayIndex{$l} }' $filePrefix > ${filePrefix}_$l.delay";
	print $cmd, "\n";
	$exit = system $cmd;
	die "fail: $cmd" unless $exit == 0;
}

# SEQ - t lte, t wifi, r lte, r wifi, r rrc
foreach my $m (@mode) {
	foreach my $l (@link) {
		next if ($l eq "rrc" && $m eq "t");

		$cmd = "awk '{ if (\$1 == \"$m\" && \$3 == \"$l\") print \$2, \$5 }' $filePrefix > ${filePrefix}_${l}_${m}.seq";
		print $cmd, "\n";
		$exit = system $cmd;
		die "fail: $cmd" unless $exit == 0;
	}
}

# COMPENSATION
$cmd = "awk '{ if ((\$1 == \"t\" || \$1 == \"r\") && \$3 != \"rrc\") print \$2, \$11 }' $filePrefix > ${filePrefix}.comp";
print $cmd, "\n";
$exit = system $cmd;
die "fail: $cmd" unless $exit == 0;

# INSTANTANEOUS THROUGHPUT
my @sinkFiles = ("${filePrefix}_ue");
foreach my $s (@sinkFiles) {
	$cmd = "awk -f instan_thru.awk $s.sink > $s.rate";
	$exit = system $cmd;
	die "fail: $cmd" unless $exit == 0;

	$cmd = "awk -f instan_thru.awk $s.lsink > $s.lrate";
	$exit = system $cmd;
	die "fail: $cmd" unless $exit == 0;
}

# BUFFER timeout 
$cmd = "awk '{ if (\$1 == \"b\" && \$3 == \"timeout\") print \$2, \$5 }' $filePrefix > ${filePrefix}_buffer.timeout";
print $cmd, "\n";
$exit = system $cmd;
die "fail: $cmd" unless $exit == 0;

# BUFFER hole
$cmd = "awk '{ if (\$1 == \"b\" && \$3 == \"hole\") print \$2, \$5 }' $filePrefix > ${filePrefix}_buffer.hole";
print $cmd, "\n";
$exit = system $cmd;
die "fail: $cmd" unless $exit == 0;

# LTE SQdelay
$cmd = "awk '{ if (\$1 == \"d\" && \$3 == \"lte\") print \$2, \$8 }' $filePrefix > ${filePrefix}_lteSQ.delay";
print $cmd, "\n";
$exit = system $cmd;
die "fail: $cmd" unless $exit == 0;

# LTE SPdelay
$cmd = "awk '{ if (\$1 == \"d\" && \$3 == \"lte\") print \$2, \$12 }' $filePrefix > ${filePrefix}_lteSP.delay";
print $cmd, "\n";
$exit = system $cmd;
die "fail: $cmd" unless $exit == 0;

# WIFI SQdelay
$cmd = "awk '{ if (\$1 == \"d\" && \$3 == \"wifi\") print \$2, \$8 }' $filePrefix > ${filePrefix}_wifiSQ.delay";
print $cmd, "\n";
$exit = system $cmd;
die "fail: $cmd" unless $exit == 0;

# WIFI SPdelay
$cmd = "awk '{ if (\$1 == \"d\" && \$3 == \"wifi\") print \$2, \$12 }' $filePrefix > ${filePrefix}_wifiSP.delay";
print $cmd, "\n";
$exit = system $cmd;
die "fail: $cmd" unless $exit == 0;

# WIFI unacked
#$cmd = "awk '{ if (\$1 == \"w\" ) print \$2, \$4, \$6, \$8, \$10 }' $filePrefix > ${filePrefix}_wifitotal.txt";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;


#delay
#$cmd = "awk '{ if (\$1 == \"a\" ) print \$2, \$3 }' $filePrefix > ${filePrefix}_pktgap.txt";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#$cmd = "awk '{ if (\$1 == \"a\" ) print \$2, \$4 }' $filePrefix > ${filePrefix}_pathgap.txt";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#probability using same path
#$cmd = "awk '{ if (\$1 == \"a\" ) print \$2, \$6 }' $filePrefix > ${filePrefix}_switchpathprob.txt";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#gap between estimated Delay (with PD) and Real Delay
#$cmd = "awk '{ if (\$1 == \"wd\" ) print \$2, \$4 }' $filePrefix > ${filePrefix}_wifidelaygapEDRD.delay";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#$cmd = "awk '{ if (\$1 == \"wd\" ) print \$2, \$6 }' $filePrefix > ${filePrefix}_wifidelaygapEDPDRD.delay";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#$cmd = "awk '{ if (\$1 == \"ld\" ) print \$2, \$4 }' $filePrefix > ${filePrefix}_ltedelaygapEDRD.delay";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#$cmd = "awk '{ if (\$1 == \"ld\" ) print \$2, \$6 }' $filePrefix > ${filePrefix}_ltedelaygapEDPDRD.delay";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#$cmd = "awk '{ if (\$1 == \"a\" ) print \$2, \$7 }' $filePrefix > ${filePrefix}_totalsamepathnum.txt";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#$cmd = "awk '{ if (\$1 == \"a\" ) print \$2, \$8 }' $filePrefix > ${filePrefix}_totalpktnum.txt";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#flowletNumber
#$cmd = "awk '{ if (\$1 == \"fl\" ) print \$2, \$3 }' $filePrefix > ${filePrefix}_ltepktnum.flowlet";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;

#$cmd = "awk '{ if (\$1 == \"fw\" ) print \$2, \$3 }' $filePrefix > ${filePrefix}_wifipktnum.flowlet";
#print $cmd, "\n";
#$exit = system $cmd;
#die "fail: $cmd" unless $exit == 0;



exit 0;

$cmd = "awk '{ if (\$1 == \"At\" && \$3 == \"FRC:\") print \$2, \$4 }' $ARGV[0] > $ARGV[0]_frc";
print $cmd, "\n";
system $cmd;
