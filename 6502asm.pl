#!/usr/bin/perl
# 
# 6502asm.pl
# Created: Sun Jul 25 13:24:14 1999 by tek@wiw.org
# Revised: Wed Aug  4 04:34:22 1999 by tek@wiw.org
# Copyright 1999 Julian E. C. Squires (tek@wiw.org)
# This program comes with ABSOLUTELY NO WARRANTY.
# $Id$
#
# Wishlist:
#   proper handling of labels for zeropage fu?
#   handling of non-hex addresses... (such as -42 for relative, etc)
#   fix hex idiocy... also, endian of addresses?
#   checking for bad values
#   Z80 support
#
# Hacknote: relative ops (like BCC) currently want the same op in
#           both relative and {zpage, absolute}
# 

use IO::File;
use strict;

my %opm = ( IMMEDIATE => 0, ABSOLUTE => 1, ZPAGE => 2,
	    IMPLIED => 3, INDX => 4, INDY => 5, ZPAGEX => 6,
	    ABSX => 7, ABSY => 8, RELATIVE => 9, INDIRECT => 10,
	    ZPAGEY => 11 );

my %optable =
  ( ADC => [ 0x69, 0x6D, 0x65, -1, 0x61, 0x71, 0x75, 0x7D, 0x79, -1, -1, -1 ],
    AND => [ 0x29, 0x2D, 0x25, -1, 0x21, 0x31, 0x35, 0x3D, 0x39, -1, -1, -1 ],
    ASL => [ -1, 0x0E, 0x06, 0x0A, -1, -1, 0x16, 0x1E, -1, -1, -1, -1 ],
    BCC => [ -1, 0x90, 0x90, -1, -1, -1, -1, -1, -1, 0x90, -1, -1 ], 
  );

my %reserved = ( A => 1, X => 1, Y => 1, S => 1, P => 1, PC => 1 );

my (%symtable, %symbyline);
my @secpass;
my ( $line, $addr, $org ) = ( 0, 0, 0x8000 );

my $out = new IO::File( "6502.out", "w" );

while( <> ) {
  $line++;

  # Eat comments
  $_ =~ s/;.*$//;

  chomp;

  if( $_ =~ /^(\w+)(\s+.*)?$/ ) {
    print "got $1\n";

    if( defined( $reserved{ $1 } ) or defined( $optable{ $1 } ) ) {
      die "$line:$1: Reserved symbol"
    }

    $symbyline{ $line } = $1;
    $_ =~ s/^(\w+)//
  }

  $secpass[ $#secpass + 1 ] = $_;
}

$line = 0;
$addr = $org;
for $_ ( @secpass ) {
  $line++;

  if( defined( $symbyline{ $line } ) )
    { if( defined( $symtable{ $1 } ) ) { die "$line:$1: Duplicate symbol" }
      print "defined $symbyline{$line}: ".unpack("H*",pack("n",$addr))."\n";
      $symtable{ $symbyline{ $line } } = $addr }

  print "$line:".unpack("H*",pack("n",$addr)).": $_\n";

  # Eat blank lines
  if( $_ =~ m/^\s*$/ ) { next }

  if( $_ !~ m/^\s*(\w+).*$/ ) { die "$line:$_: parse error" }
  if( !defined( $optable{ $1 } ) ) { die "$line:$1: bad opcode" }

  if( /^\s*(\w+)\s+\#\$([0-9A-Fa-f]{2}?)\s*$/ ) { $addr += 2 }
  elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{4}?)\s*$/ ) { $addr += 3 }
  elsif( /^\s*(\w+)\s+(\w+)\s*$/ ) { $addr += 3 }
  elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{2}?)\s*$/ ) { $addr += 2 }
  elsif( /^\s*(\w+)\s*$/ ) { $addr++ }
  elsif( /^\s*(\w+)\s+\(\$([0-9A-Fa-f]{4}?)\)\s*$/ ) { $addr += 3 }
  elsif( /^\s*(\w+)\s+\((\w+)\)\s*$/ ) { $addr += 3 }
  elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{4}?)\s*,?\s*X\s*$/ ) { $addr += 3 }
  elsif( /^\s*(\w+)\s+(\w+)\s*,?\s*X\s*$/ ) { $addr += 3 }
  elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{4}?)\s*,?\s*Y\s*$/ ) { $addr += 3 }
  elsif( /^\s*(\w+)\s+(\w+)\s*,?\s*Y\s*$/ ) { $addr += 3 }
  elsif( /^\s*(\w+)\s+(\$[0-9A-Fa-f]{2}?)\s*,\s*?X\s*$/ ) { $addr += 2 }
  elsif( /^\s*(\w+)\s+(\$[0-9A-Fa-f]{2}?)\s*,\s*?Y\s*$/ ) { $addr += 2 }
  elsif( /^\s*(\w+)\s+\(\$([0-9A-Fa-f]{2}?)\s*,?\s*X\)\s*$/ ) { $addr += 2 }
  elsif( /^\s*(\w+)\s+\(\$([0-9A-Fa-f]{2}?)\s*\)\s*,?\s*Y\s*$/ ) { $addr += 2 }
  else { die "$line: parse error" }
}

$line = 0;
$addr = $org;
for $_ ( @secpass ) {
  $line++;

  print "$line:".unpack("H*",pack("n",$addr)).": $_\n";

  # Eat blank lines
  if( $_ =~ m/^\s*$/ ) { next }

  if( $_ !~ m/^\s*(\w+).*$/ ) { die "$line:$_: parse error" }
  if( !defined( $optable{ $1 } ) ) { die "$line:$1: bad opcode" }

  if( /^\s*(\w+)\s+\#\$([0-9A-Fa-f]{2}?)\s*$/ ) {
    if( $optable{ $1 }[ $opm{ IMMEDIATE } ] == -1 )
    { die "$line:$1:immediate: bad addressing mode" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ IMMEDIATE } ] ) .
		 pack("C", hex( $2 ) ) )

  } elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{4}?)\s*$/ ) {
    if( $optable{ $1 }[ $opm{ RELATIVE } ] != -1 ) {
      $_ = pack("C", $2 - ( $addr + 1 ) );
      $addr += 2;

    } else {
      if( $optable{ $1 }[ $opm{ ABSOLUTE } ] == -1 )
      { die "$line:$1:absolute: bad addressing mode" }

      $_ = pack("v", hex( $2 ) );
      $addr += 3;
    }

    $out->print( chr( $optable{ $1 }[ $opm{ ABSOLUTE } ] ) . $_ )

  } elsif( /^\s*(\w+)\s+(\w+)\s*$/ and defined( $symtable{ $2 } ) ) {
    if( $optable{ $1 }[ $opm{ RELATIVE } ] != -1 ) {
      $_ = pack("C", $symtable{ $2 } - ( $addr + 1 ) );
      $addr += 2;

    } else {
      if( $optable{ $1 }[ $opm{ ABSOLUTE } ] == -1 )
      { die "$line:$1:absolute: bad addressing mode" }

      $_ = pack("v", $symtable{ $2 } );
      $addr += 3;
    }

    $out->print( chr( $optable{ $1 }[ $opm{ ABSOLUTE } ] ) . $_ )

  } elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{2}?)\s*$/ ) {
    if( $optable{ $1 }[ $opm{ RELATIVE } ] != -1 ) {
      $_ = pack("C", $2 - ( $addr + 1 ) );

    } else {
      if( $optable{ $1 }[ $opm{ ZPAGE } ] == -1 )
      { die "$line:$1:zero-page: bad addressing mode" }

      $_ = pack("c", $2 );
    }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ ZPAGE } ] ) . $_ )

  } elsif( /^\s*(\w+)\s*$/ ) {
    if( $optable{ $1 }[ $opm{ IMPLIED } ] == -1 )
    { die "$line:$1:implied: bad addressing mode" }

    $addr++;
    $out->print( chr( $optable{ $1 }[ $opm{ IMPLIED } ] ) )

  } elsif( /^\s*(\w+)\s+\(\$([0-9A-Fa-f]{4}?)\)\s*$/ ) {
    if( $optable{ $1 }[ $opm{ INDIRECT } ] == -1 )
    { die "$line:$1:indirect absolute: bad addressing mode" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ INDIRECT } ] ) .
		 pack("v*", hex( $2 ) ) )

  } elsif( /^\s*(\w+)\s+\((\w+)\)\s*$/ and defined( $symtable{ $2 } ) ) {
    if( $optable{ $1 }[ $opm{ INDIRECT } ] == -1 )
    { die "$line:$1:indirect absolute: bad addressing mode" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ INDIRECT } ] ) .
		 pack("v", $symtable{ $2 } ) )

  } elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{4}?)\s*,?\s*X\s*$/ ) {
    if( $optable{ $1 }[ $opm{ ABSX } ] == -1 )
    { die "$line:$1:absolute indexed by X: bad addressing mode" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ ABSX } ] ) .
		 pack("v", hex( $2 ) ) )

  } elsif( /^\s*(\w+)\s+(\w+)\s*,?\s*X\s*$/ and defined( $symtable{ $2 } ) ) {
    if( $optable{ $1 }[ $opm{ ABSX } ] == -1 )
    { die "$line:$1:absolute indexed by X: bad addressing mode" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ ABSX } ] ) .
		 pack("v", $symtable{ $2 } ) )

  } elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{4}?)\s*,?\s*Y\s*$/ ) {
    if( $optable{ $1 }[ $opm{ ABSY } ] == -1 )
    { die "$line:$1:absolute indexed by Y: bad addressing mode" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ ABSY } ] ) .
		 pack("v", hex( $2 ) ) )

  } elsif( /^\s*(\w+)\s+(\w+)\s*,?\s*Y\s*$/ and defined( $symtable{ $2 } ) ) {
    if( $optable{ $1 }[ $opm{ ABSY } ] == -1 )
    { die "$line:$1:absolute indexed by Y: bad addressing mode" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ ABSY } ] ) .
		 pack("v", $symtable{ $2 } ) )

  } elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{2}?)\s*,\s*?X\s*$/ ) {
    if( $optable{ $1 }[ $opm{ ZPAGEX } ] == -1 )
    { die "$line:$1:zero-page indexed by X: bad addressing mode" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ ZPAGEX } ] ) .
		 pack("c", hex( $2 ) ) )

  } elsif( /^\s*(\w+)\s+\$([0-9A-Fa-f]{2}?)\s*,\s*?Y\s*$/ ) {
    if( $optable{ $1 }[ $opm{ ZPAGEY } ] == -1 )
    { die "$line:$1:zero-page indexed by Y: bad addressing mode" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ ZPAGEY } ] ) .
		 pack("c", hex( $2 ) ) )

  } elsif( /^\s*(\w+)\s+\(\$([0-9A-Fa-f]{2}?)\s*,?\s*X\)\s*$/ ) {
    if( $optable{ $1 }[ $opm{ INDX } ] == -1 )
    { die "$line:$1: indexed indirect by X: bad addressing mode" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ INDX } ] ) .
		 pack("v", hex( $2 ) ) )

  } elsif( /^\s*(\w+)\s+\(\$([0-9A-Fa-f]{2}?)\s*\)\s*,?\s*Y\s*$/ ) {
    if( $optable{ $1 }[ $opm{ INDY } ] == -1 )
    { die "$line:$1: indirect indexed by Y: bad addressing mode" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ INDY } ] ) .
		 pack("v", hex( $2 ) ) )

  } else { die "$line: parse error" }
}

# EOF 6502asm.pl
