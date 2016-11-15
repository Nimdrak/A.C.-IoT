#!/usr/bin/perl -w

use strict;
use warnings;

die "Usage: $0 log_file" unless $#ARGV == 0; # last index

my $filePrefix = $ARGV[0];

my $cmd;
my $exit = 0;

# SUCCESS COUNT (CONTROL GROUP)
	$cmd = "awk '{ if (\$1 == \"@\" && \$2 == \"1\") print \$2 }' $filePrefix > control.count";
	print $cmd, "\n";
	$exit = system $cmd;
	die "fail: $cmd" unless $exit == 0;

exit 0;

system $cmd;
