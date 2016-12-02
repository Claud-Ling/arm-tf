#
# fusedatamap.pl tool
# It can be used to generate fuse register file based on fuse descriptor file (*.fd)
# 
# Author: Tony He
# Date:   2016/11/28
#
#!/usr/bin/perl -w

use strict;
require 5.003;

die "$0 REG.fd bitfields_dir >REG.h" unless (@ARGV==2);

my $F;
my $l0;
my $FN;
my $fn;
my @name;
my @offset;
my @width;
my @rw;
my $i;
my $prevofs;
my $total;
my $regnm;

if ($ARGV[0] =~ /(\w+)\.fd/) {
  $FN=uc($1);
  $fn=lc($1);
}
else {
  die "? $ARGV[0]";
}

$total=0;
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

  # <name> <offset> <width> [comments]
  # offset and width are in hex form
  if ($l0 =~ /(\w+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s*(.*)/) {
    $name[$total]=$1;
    $offset[$total]=hex($2);
    die "? width $1" unless ($3);
    $width[$total]=hex($3);
    $rw[$total]="";
    $rw[$total]="$4" if ($4);
    $total=$total+1;
  }
  else {
    die "? $l0\n";
  }
}

close F;


print <<END;
/*
 * Auto generated from $ARGV[0]
 * DO NOT EDIT!
 *
 * Must include stdint.h.
 * The most important part of the file is the struct describing the fuse registers
 * file content.
 *
 */

#ifndef __${FN}_H__
#define __${FN}_H__

END

for ($i = 0; $i < $total; $i++) {
  if (-e "$ARGV[1]/otp/$name[$i].h") {
    print "#include <otp/$name[$i].h>\n"
  }
}

print <<END;

#ifndef __ASSEMBLY__

struct ${fn} {
END

$prevofs=0;
for ($i = 0; $i < $total; $i++) {
  if (exists $width[$i]) {
    die "? overlap `$name[$i]' in $FN" if ($prevofs!=0 && $prevofs>$offset[$i]);
    my $hole=$offset[$i]-$prevofs;
    die "? wrong offset `$name[$i]' in $FN" if ($hole & 3);
    if ($hole!=0) {
      print "\tvolatile uint32_t hole$i";
      if ($hole>4) {
        printf ("[%d];\n", $hole/4);
      } else {
        print ";\n";
      }
    }
    if (-e "$ARGV[1]/otp/$name[$i].h") {
      $regnm=uc($name[$i]);
      print "\tvolatile union ${regnm}Reg $name[$i]";
    } else {
      print "\tvolatile uint32_t $name[$i]";
    }

    die "? wrong width `$name[$i]' in $FN" if ($width[$i] & 3);
    if ($width[$i]>4) {
      printf ("[%d];", $width[$i]/4);
    } else {
      print ";";
    }
    printf ("\t/* +0x%03x $rw[$i]*/\n", $offset[$i]);

    $prevofs=$offset[$i]+$width[$i];
  }
}

print <<END;
};

#endif /* !__ASSEMBLY__ */
#endif /* __${FN}_H__ */
END
