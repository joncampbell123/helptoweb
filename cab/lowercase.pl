#!/usr/bin/perl
#
# rename all files in the current directory to lowercase.
# Microsoft CHM files are apparently case-insensitive.

sub shellesc($) {
    my $x = shift @_;

    $x =~ s/\'/\\\'/;
    $x =~ s/\"/\\\"/;
    $x =~ s/\$/\\\$/;

    return $x;
}

open(LST,"find |") || die;
while (my $spath = <LST>) {
    chomp $spath;
    next unless -e $spath;
    $dpath = lc($spath);
    next if $spath eq $dpath;
    rename($spath,$dpath);
}
close(LST);

