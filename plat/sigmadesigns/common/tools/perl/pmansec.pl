#
# pmansec.pl tool
# It can be used to generate PMAN security file based on PMAN security descriptor file (*.pmsd)
# 
# Author: Tony He
# Date:   2016/11/28
#
#!/usr/bin/perl -w

use strict;
require 5.003;

die "$0 REG.pmsd >REG.h" unless (@ARGV==1);

my $F;
my $l0;
my $FN;
my @name;
my @sid;
my @rw;
my $i;
my $total;
my $tmp;

if ($ARGV[0] =~ /(\w+)\.pmsd/) {
  $FN=uc($1);
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

  # <name> <id> [comments]
  # id in decimal form
  if ($l0 =~ /(\w+)\s+(\d+)\s*(.*)/) {
    $name[$total]=$1;
    $sid[$total]=$2;
    $rw[$total]="";
    $rw[$total]="$3" if ($3);
    for ($i = 0; $i < $total; $i++) {
      die "? duplicated id $name[$i] and $name[$total]" if ($sid[$i] == $sid[$total]);
    }

    $total++;
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
 * The most important part of the file is the MACROs describing PMAN
 * security number.
 *
 */

#ifndef __${FN}_H__
#define __${FN}_H__

END

for ($i = 0; $i < $total; $i++) {
  $tmp = uc($name[$i]);
  print "#define PMAN_RGN_SEC_${tmp}  (1 << $sid[$i])\n";
}

print <<END;

#ifndef __ASSEMBLY__

#endif /* !__ASSEMBLY__ */
#endif /* __${FN}_H__ */
END
