#
# fieldhelpers.pl tool
# It can be used to generate bitfield helper.
# It will iterate over all register descriptor files (*.rd) under specified directory
# and create helper for each bitfield respectively.
#
# Note that there are two constraints:
# 1. each RD file under specified directory shall be set with distinct name;
# 2. field name shall be distinct in all RD files scale;
#
#!/usr/bin/perl -w

use strict;
require 5.003;

die "$0 path_to_rd_dir" unless (@ARGV==1);

my $F;
my $l0;
my $reg;
my @flst;
my @rds;
my @name;
my @rdi; #rd file index
my @rw;
my $fi = 0; #file index
my $nb = 0; #number of bitfields
my $i;

$reg = uc("$ARGV[0]_HELPERS");
$reg =~ s/[\/.\s]/_/g;

&find_fileindir("$ARGV[0]");
sub find_fileindir(){
  my $dir = $_[0];
  opendir(DIR,"$dir") || die "can't open dir '$dir'\n";
  my @files = readdir(DIR);
  closedir(DIR);
  for my $file (@files) {
    next if ($file =~ m/\.$/ || $file =~ m/\.\.$/);
    if ($file =~ /\.rd$/i) {
      $flst[$fi] = "$dir/$file";
      $fi++;
    }
    elsif (-d "$dir/$file") {
      &find_fileindir("$dir/$file");
    }
  }
}

$fi = 0; # reset index
for my $file (@flst) {
  open F, "$file";

  $file =~ /(\w+)\.rd$/i;
  for my $rdfile (@rds) {
    die "Identical RD file name '$1'\n" if ($1 eq "$rdfile");
  }
  $rds[$fi] = $1;

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

    # skip configure lines
    if ($l0 =~ /type\s*(.*)/) {
      next;
    }

    # <name> <start> <bits> [comments]
    # start and bits are in decimal form
    if ($l0 =~ /(\w+)\s+(\d+)\s+(\d+)\s*(.*)/) {
      for ($i = 0; $i < $nb; $i++) {
        die "Identical field '$1' in $file and $flst[$rdi[$i]]\n" if ($1 eq $name[$i]);
      }

      $name[$nb] = $1;
      $rdi[$nb] = $fi;
      $rw[$nb]="";
      $rw[$nb]="$4" if ($4);
      $nb++;
    }
    else {
      die "? $l0\n";
    }
  }

  close F;
  $fi++;
}

print <<END;
/*
 * Auto generated from RD files under $ARGV[0],
 * DO NOT EDIT!!
 *
 * The most important part of the file is field helpers list.
 *
 */

#ifndef __${reg}_H__
#define __${reg}_H__

#ifndef __ASSEMBLY__

END

for ($i = 0; $i < $nb; $i++) {
  print "FIELD_HELPER($rds[$rdi[$i]], $name[$i]) /* $rw[$i] */\n";
}

print <<END;

#endif /* !__ASSEMBLY__ */
#endif /* __${reg}_H__ */
END
