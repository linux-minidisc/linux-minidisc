#!/usr/bin/perl

# This program takes in logs saved from DebugView.exe in windows and gets
# rid of the line number and timestamp fields (tab seperated)
sub trim
{
	my @out = @_;
	for (@out) 
	{
		s/^\s+//;
		s/\s+$//;
	}
	return wantarray ? @out : $out[0];
}

foreach $MyFile (@ARGV)
{
	$OutFile = $MyFile . ".clean";
	print "$OutFile\n";
	open(FILE, $MyFile);
	open(FILE2, ">$OutFile");
	while (<FILE>)
	{
		($junk, $junk2, $data) = split(/\t/, $_, 3);
		print  FILE2 "$data";
	}
}
