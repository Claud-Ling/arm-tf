#
# bitfield.pl tool
# It can be used to generate register bitfield file based on a register descriptor file (*.rd)
#
#
#!/usr/bin/perl -w

use strict;
require 5.003;

die "$0 REG.rd >REG.c" unless (@ARGV==1);

my $nope = "";
my $F;
my $l0;
my $reg;
my @name;
my @width;
my @rw;
my $nbits = 32; # default 32-bits
my $i;
my $previ;

if ($ARGV[0] =~ /(\w+)\.rd/) {
  $reg=uc($1);
}
else {
  die "? $ARGV[0]";
}

open F, "<$ARGV[0]";

while ($l0=<F>) {
  chomp($l0);

  # support comments
  if ($l0 =~ /^#.*/) {
    next;
  }

  # support empty lines that do not contain white spaces
  if ($l0 =~ /^$/) {
    next;
  }

  # support configure line: type b|B|h|H|w|W|q|Q
  if ($l0 =~ /^type\s*(.*)/) {
    $nbits = 0;
    $nbits = 8  if ($1 =~ /\b[bB]\b\s*(.*)/);
    $nbits = 16 if ($1 =~ /\b[hH]\b\s*(.*)/);
    $nbits = 32 if ($1 =~ /\b[wW]\b\s*(.*)/);
    $nbits = 64 if ($1 =~ /\b[qQ]\b\s*(.*)/);
    die "? $l0 " unless ($nbits != 0);
    next;
  }

  # <name> <start> <bits> [comments]
  # start and bits are in decimal form
  if ($l0 =~ /(\w+)\s+(\d+)\s+(\d+)\s*(.*)/) {
    $name[$2]=$1;
    die "? width $1" unless ($3);
    $width[$2]=$3;
    $rw[$2]="";
    $rw[$2]="$4" if ($4);
  }
  else {
    die "? $l0\n";
  }
}

close F;


print <<END;
/*
 * Auto generated from $ARGV[0],
 * DO NOT EDIT!!
 *
 * Must include stdint.h.
 * The most important part of the file is the union describing the register
 * content.
 *
 */

#ifndef __${reg}_H__
#define __${reg}_H__

END

for ($i = ${nbits}-1; $i >= 0; $i--) {
  if (exists $width[$i]) {
    print <<END;
#define ${reg}_$name[$i]_SHIFT $i /* $rw[$i] */
#define ${reg}_$name[$i]_WIDTH $width[$i]
END
  }
}

print <<END;

#ifndef __ASSEMBLY__

union ${reg}Reg {
        struct { __extension__ uint${nbits}_t // lsbs...
END

$previ=0;
for ($i = 0; $i < $nbits; $i++) {
  if (exists $width[$i]) {
    print ",\n" if ($previ);

    die "? overlap `$name[$i]' in $reg" if ($previ > $i);
    my $hole=$i-$previ;
    print "                                                hole$i: $hole,\n" unless ($hole==0);
    print "                $name[$i]: $width[$i]  /* $rw[$i] */";

    $previ=$i+$width[$i];
  }

}
$previ=$nbits-$previ;
if ($previ==0) {
  print "; // ... to msbs\n";
}
else {
  print ",\n                                                hole0: $previ; // ... to msbs\n";
}

print <<END;
        } bits;

        uint${nbits}_t val;
};

#endif /* !__ASSEMBLY__ */
#endif /* __${reg}_H__ */
END
