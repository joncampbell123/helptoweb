#!/usr/bin/perl
#
# Read an MSHC ContentStore, extract all files into the current directory.

my $path = shift @ARGV;
die "need path" unless defined($path) && $path ne "";
die "$path does not exist" unless -d $path;
my $basepath = $path;

system("rm -Rf FINAL && mkdir -p FINAL") == 0 || die;

sub shellesc($) {
    my $x = shift @_;

    $x =~ s/\'/\\\'/;
    $x =~ s/\"/\\\"/;
    $x =~ s/\$/\\\$/;

    return $x;
}

my @mshc;

open(MSHC,"cd \"$basepath\" && find -name \\*.mshc |") || die;
while (my $rpath = <MSHC>) {
    chomp $rpath;
    $path = "$basepath/$rpath";
    next unless -f $path;
    push(@mshc,$path);
}
close(MSHC);

for ($i=0;$i < @mshc;$i++) {
    $path = $mshc[$i];
    $name = substr($path,rindex($path,'/')+1);
    $name =~ s/\.mshc$//;
    die if $name eq "";

    print "Extracting $path... ($name)\n";

    system("rm -Rf .tmp && mkdir -p .tmp") == 0 || die;
    system("cd .tmp && unzip -- \"".shellesc($path)."\"") == 0 || die;

    open(LS,"cd .tmp && find |") || die;
    while (my $elem = <LS>) {
        chomp $elem;
        $elem =~ s/^\.\///;
        $felem = ".tmp/$elem";
        die unless -e $felem;
        next unless -f $felem;

        $nelem = "FINAL/".lc($name);
        if (!( -d $nelem )) {
            system("mkdir -p \"".shellesc($nelem)."\"") == 0 || die;
        }

        $nelem = "FINAL/".lc($name)."/".lc($elem);
        die if -e $nelem;
        rename($felem,$nelem) || die;
    }
    close(LS);
}

