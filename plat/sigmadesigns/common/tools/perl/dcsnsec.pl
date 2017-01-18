#
# dcsnsec.pl tool
# It can be used to generate dcsn_sec.h file based on DCS descriptor file (*.dcsd)
# 
# Author: Tony He
# Date:   2016/11/29
#
#!/usr/bin/perl -w

use strict;
require 5.003;

die "$0 fn.dcsd >fn.h" unless (@ARGV==1);

my $F;
my $l0;
my $FN;
my $fn;
my @iname;
my @initid;
my @irw;
my @tname;
my @tid;
my @tgrp;
my @trw;
my $i;
my $j;
my $inum = 0;
my $tnum = 0;
my @gname;
my @gnt;
my $gnum = 0;

if ($ARGV[0] =~ /(\w+)\.dcsd/) {
  $FN=uc($1);
  $fn=lc($1);
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

  # initor <name> <initid> [comments]
  # initid are in decimal form
  if ($l0 =~ /initor\s+(\w+)\s+(\d+)\s*(.*)/) {
    $iname[$inum]=$1;
    $initid[$inum]=$2;
    $irw[$inum]="";
    $irw[$inum]="$3" if ($3);
    $inum++;
  }
  # target <name> <id> <grp> [comments]
  # id are in decimal form
  elsif ($l0 =~ /target\s+(\w+)\s+(\d+)\s+(\w+)\s*(.*)/) {
    $tname[$tnum]=$1;
    $tid[$tnum]=$2;
    $tgrp[$tnum]=$3;
    $trw[$tnum]="";
    $trw[$tnum]="$4" if ($4);

    if (($tgrp[$tnum] ne "plf") && ($tgrp[$tnum] ne "av") && ($tgrp[$tnum] ne "disp")) {
      die "? group `$tgrp[$tnum]'";
    }

    for ($i = 0; $i < $gnum; $i++) {
      if ($tgrp[$tnum] eq $gname[$i]) {
        last;
      }
    }
    if ($i == $gnum) {
      $gname[$gnum] = $tgrp[$tnum];
      $gnum++;
    }

    $tnum++;
  }
  else {
    die "? $l0";
  }
}

close F;


print <<END;
/*
 * Auto generated from $ARGV[0]
 * DO NOT EDIT!
 *
 * Must include stdint.h.
 * The most important part of the file is the MACROs: DCSN_SEC_XX and DCSN_TARGET_XX
 * and struct describing the dcsn security aperture registers file.
 *
 */

#ifndef __${FN}_H__
#define __${FN}_H__

END

for ($i = 0; $i < $inum; $i++) {
  for ($j = 0; $j < $inum; $j++) {
    if ($i != $j) {
      die "? initiator `$iname[$i]' vs `$iname[$j]' in $FN" if ($initid[$i] == $initid[$j]);
    }
  }
}

print <<END;

/* Initiator */
END

$j = 0;
for ($i = 0; $i < $inum; $i++) {
  $j = $initid[$i] if ($initid[$i] > $j);
  print "#define DCSN_SEC_$iname[$i]\t(1 << $initid[$i])\t/* $irw[$i] */\n";
}
printf ("#define DCSN_SEC_EVERYBODY    0x%x\n", ((1 << ($j + 1)) - 1));

for ($i = 0; $i < $tnum; $i++) {
  for ($j = 0; $j < $tnum; $j++) {
    if ($i != $j) {
      die "? dup target name `$tname[$i] in $FN" if ($tname[$i] eq $tname[$j]);
      if ($tgrp[$i] eq $tgrp[$j]) {
        die "? dup target id `$tname[$i]' vs `$tname[$j]' in $FN" if ($tid[$i] == $tid[$j]);
      }
    }
  }
  for ($j = 0; $j < $gnum; $j++) {
    if ($tgrp[$i] eq $gname[$j]) {
      if (($tid[$i] + 1) > $gnt[$j]) {
        $gnt[$j] = $tid[$i] + 1;
      }
      last;
    }
  }
}

print <<END;

/* Targets */
END

for ($i = 0; $i < $tnum; $i++) {
  my $nm = uc($tname[$i]);
  print "#define DCSN_TARGET_$nm\t$tid[$i]\t/* DCS $tgrp[$i] $trw[$i] */\n";
}

print <<END;

#ifndef __ASSEMBLY__

/* DCSN Control */
END

for ($i = 0; $i < $gnum; $i++) {
  print "struct dcsn_ctrl_$gname[$i] {\n";
  print "\tvolatile uint32_t config_reg_access;   /* +0x0000 */\n";
  print "\tvolatile uint32_t security_reg_access; /* +0x0004 */\n";
  print "\tvolatile uint32_t pad8[126];           /* +0x0008 */\n";
  print "\tvolatile uint32_t access_to_target[$gnt[$i]];/* +0x0200 */\n";
  printf ("\tvolatile uint32_t pad%x[%d];        /* +0x%04x */\n", 512+4*$gnt[$i],(11772-4*$gnt[$i])/4,512+4*$gnt[$i]);
  print "\tvolatile uint32_t module_id;           /* +0x2ffc */\n";
  print "};\n\n";
}

print <<END;

struct dcsn_ctrl {
END

for ($i = 0; $i < $gnum; $i++) {
  print "\tstruct dcsn_ctrl_$gname[$i] *$gname[$i];\n";
}

print <<END;
};

END

if ($gnum != 0) {
  print "#define DEFINE_DCSN_CTRL(";
  for ($i = 0; $i < $gnum; $i++) {
    print "," unless ($i == 0);
    print "a$i";
  }
  print ") struct dcsn_ctrl dcsn_ctl = \\\n";
  print "{";
  for ($i = 0; $i < $gnum; $i++) {
    print "," unless ($i == 0);
    print "(struct dcsn_ctrl_$gname[$i] *) (a$i)";
  }
  print "}\n"
}

print "\n#define DECLARE_DCSN_CTRL extern struct dcsn_ctrl dcsn_ctl\n";

print <<END;

/* DCS control helpers */
END
for ($i = 0; $i < $gnum; $i++) {
  print "\n";
  $j = lc($gname[$i]);
  print "#define dcs_${j}_config    dcsn_ctl.$tgrp[$i] ->config_reg_access\n";
  print "#define dcs_${j}_security  dcsn_ctl.$tgrp[$i] ->security_reg_access\n";
  print "#define dcs_${j}_target(i) dcsn_ctl.$tgrp[$i] ->access_to_target[i]\n";
  print "#define dcs_${j}_module_id dcsn_ctl.$tgrp[$i] ->module_id\n";
  print "/*iterate dcs ${j} targets: void ops(uint32_t *acc, void* param);*/\n";
  print "#define do_for_each_dcs_${j}_target(ops, param) do {\\\n";
  print "\tint k;\\\n";
  print "\tfor (k = 0; k < sizeof(dcsn_ctl.$tgrp[$i] ->access_to_target)/sizeof(dcsn_ctl.$tgrp[$i] ->access_to_target[0]); k++) {\\\n";
  print "\t\tops(&dcs_${j}_target(k), (param));\\\n";
  print "\t}\\\n";
  print "} while(0)\n";
}

print <<END;

/*iterate targets of all DCS: void ops(uint32_t *acc, void* param);*/
#define do_for_each_dcs_target(ops, param) do { \\
END

for ($i = 0; $i < $gnum; $i++) {
  $j = lc($gname[$i]);
  print "\tdo_for_each_dcs_${j}_target(ops, (param));\\\n";
}

print <<END;
} while(0)
END

print <<END;

/*iterate config of all DCS: void ops(uint32_t *acc, void* param);*/
#define do_for_each_dcs_config(ops, param) do { \\
END

for ($i = 0; $i < $gnum; $i++) {
  $j = lc($gname[$i]);
  print "\tops(&dcs_${j}_config, (param));\\\n";
}

print <<END;
} while(0)
END

print <<END;

/*iterate security of all DCS: void ops(uint32_t *acc, void* param);*/
#define do_for_each_dcs_security(ops, param) do { \\
END

for ($i = 0; $i < $gnum; $i++) {
  $j = lc($gname[$i]);
  print "\tops(&dcs_${j}_security, (param));\\\n";
}

print <<END;
} while(0)

/* Target access helpers */
END

for ($i = 0; $i < $tnum; $i++) {
  $j = lc($tname[$i]);
  print "#define dcs_target_access_${j} dcsn_ctl.$tgrp[$i]->access_to_target[$tid[$i]]\n";
}

print <<END;

/*
 * generic dcs target access quotation helper
 * <nm> specifies the target name
 */
#define dcs_target_access(nm) dcs_target_access_##nm

#endif /* !__ASSEMBLY__ */
#endif /* __${FN}_H__ */
END
