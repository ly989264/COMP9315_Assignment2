#!/usr/bin/perl -w

open IN, "<", "./res.res" or die "Cannot open file";
%hash = ();
while (<IN>) {
	chomp;
	if (!defined $hash{$_}) {
		$hash{$_} = 1;
	} else {
		$hash{$_}++;
	}
}
for (sort keys %hash) {
	print $_, " ", $hash{$_}, "\n";
}
