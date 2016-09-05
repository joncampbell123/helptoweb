#!/usr/bin/perl
#
# Read all html files in the current directory, read the meta Microsoft.Help.Id, and build a catalog from it

use utf8;
use encoding 'UTF-8';
use Data::Dumper;
use XML::Simple;

binmode STDOUT, ':utf8';

sub shellesc($) {
    my $x = shift @_;

    $x =~ s/\'/\\\'/;
    $x =~ s/\"/\\\"/;
    $x =~ s/\$/\\\$/;

    return $x;
}

my @catalog;

open(CAT,">catalog.txt") || die;
binmode(CAT,':utf8');

open(LS,"find |") || die;
while (my $path = <LS>) {
    chomp $path;
    $path =~ s/^\.\///;
    next unless -f $path;
    next unless $path =~ m/\.(html|htm)$/i;

    print "Parsing $path...\n";
    $parser = new XML::Simple;

    my $html;
    open(X,"<$path") || die;
    binmode(X,':utf8');
    read(X,$html,1024*1024*128);
    close(X);

    utf8::encode($html);
    $data = XMLin($html);

    # we're looking for root -> head -> <meta name="Microsoft.Help.Id" content="...">
    $x = $data->{"head"};
    next unless defined($x);
    $x = $x->{"meta"};
    next unless defined($x);
    next unless ref($x) eq 'ARRAY';

    my $id = undef;
    my @a = @{$x};

    for ($i=0;$i < @a;$i++) {
        my $yy = $a[$i];
        my %x = %{$yy};

        my $name = $x{"name"};
        my $content = $x{'content'};

        next unless defined($name);
        next unless defined($content);

        if (lc($name) eq lc("Microsoft.Help.Id")) {
            print CAT "$path\n";
            print CAT "-id:$content\n";
            print CAT "\n";
        }
    }
}
close(LS);
close(CAT);

