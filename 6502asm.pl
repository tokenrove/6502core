#!/usr/bin/perl
# 
# 6502asm.pl
# Created: Sun Jul 25 13:24:14 1999 by tek@wiw.org
# Revised: Fri Aug  6 21:05:25 1999 by tek@wiw.org
# Copyright 1999 Julian E. C. Squires (tek@wiw.org)
# This program comes with ABSOLUTELY NO WARRANTY.
# $Id: 6502asm.pl,v 1.1.1.1 1999/08/04 07:06:56 tek Exp $
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
    BCS => [ -1, 0x80, 0x80, -1, -1, -1, -1, -1, -1, 0x80, -1, -1 ],

    BEQ => [ -1, 0xF0, 0xF0, -1, -1, -1, -1, -1, -1, 0xF0, -1, -1 ],
    BIT => [ -1, 0x2C, 0x24, -1, -1, -1, -1, -1, -1, -1, -1, -1 ],
    BMI => [ -1, 0x30, 0x30, -1, -1, -1, -1, -1, -1, 0x30, -1, -1 ],
    BNE => [ -1, 0xD0, 0xD0, -1, -1, -1, -1, -1, -1, 0xD0, -1, -1 ],
    BPL => [ -1, 0x10, 0x10, -1, -1, -1, -1, -1, -1, 0x10, -1, -1 ],

    BRK => [ -1, -1, -1, 0x00, -1, -1, -1, -1, -1, -1, -1, -1 ],
    BVC => [ -1, 0x50, 0x50, -1, -1, -1, -1, -1, -1, 0x50, -1, -1 ],
    BVS => [ -1, 0x70, 0x70, -1, -1, -1, -1, -1, -1, 0x70, -1, -1 ],
    CLC => [ -1, -1, -1, 0x18, -1, -1, -1, -1, -1, -1, -1, -1 ],
    CLD => [ -1, -1, -1, 0xD8, -1, -1, -1, -1, -1, -1, -1, -1 ],

    CLI => [ -1, -1, -1, 0x58, -1, -1, -1, -1, -1, -1, -1, -1 ],
    CLV => [ -1, -1, -1, 0xB8, -1, -1, -1, -1, -1, -1, -1, -1 ],
    CMP => [ 0xC9, 0xCD, 0xC5, -1, 0xC1, 0xD1, 0xD5, 0xDD, 0xD9, -1, -1, -1 ],
    CPX => [ 0xE0, 0xEC, 0xE4, -1, -1, -1, -1, -1, -1, -1, -1, -1 ],
    CPY => [ 0xC0, 0xCC, 0xC4, -1, -1, -1, -1, -1, -1, -1, -1, -1 ],

    DEC => [ -1, 0xCE, 0xC6, -1, -1, -1, 0xD6, 0xDE, -1, -1, -1, -1 ],
    DEX => [ -1, -1, -1, 0xCA, -1, -1, -1, -1, -1, -1, -1, -1 ],
    DEY => [ -1, -1, -1, 0x88, -1, -1, -1, -1, -1, -1, -1, -1 ],
    EOR => [ 0x49, 0x4D, 0x45, -1, 0x41, 0x51, 0x55, 0x5D, 0x59, -1, -1, -1 ],
    INC => [ -1, 0xEE, 0xE6, -1, -1, -1, 0xF6, 0xFE, -1, -1, -1, -1 ],

    INX => [ -1, -1, -1, 0xE8, -1, -1, -1, -1, -1, -1, -1, -1 ],
    INY => [ -1, -1, -1, 0xC8, -1, -1, -1, -1, -1, -1, -1, -1 ],
    JMP => [ -1, 0x4C, -1, -1, -1, -1, -1, -1, -1, -1, 0x6C, -1 ],
    JSR => [ -1, 0x20, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 ],
    LDA => [ 0xA9, 0xAD, 0xA5, -1, 0xA1, 0xB1, 0xB5, 0xBD, 0xB9, -1, -1, -1 ],

    LDX => [ 0xA2, 0xAE, 0xA6, -1, -1, -1, -1, -1, 0xBE, -1, -1, 0xB6 ],
    LDY => [ 0xA0, 0xAC, 0xA4, -1, -1, -1, 0xB4, 0xBC, -1, -1, -1, -1 ],
    LSR => [ -1, 0x4E, 0x46, 0x4A, -1, -1, 0x56, 0x5E, -1, -1, -1, -1 ],
    NOP => [ -1, -1, -1, 0xEA, -1, -1, -1, -1, -1, -1, -1, -1 ],
    ORA => [ 0x09, 0x0D, 0x05, -1, 0x01, 0x11, 0x15, 0x1D, 0x19, -1, -1, -1 ],

    PHA => [ -1, -1, -1, 0x48, -1, -1, -1, -1, -1, -1, -1, -1 ],
    PHP => [ -1, -1, -1, 0x08, -1, -1, -1, -1, -1, -1, -1, -1 ],
    PLA => [ -1, -1, -1, 0x68, -1, -1, -1, -1, -1, -1, -1, -1 ],
    PLP => [ -1, -1, -1, 0x28, -1, -1, -1, -1, -1, -1, -1, -1 ],
    ROL => [ -1, 0x2E, 0x26, 0x2A, -1, -1, 0x36, 0x3E, -1, -1, -1, -1 ],

    ROR => [ -1, 0x6E, 0x66, 0x6A, -1, -1, 0x76, 0x7E, -1, -1, -1, -1 ],
    RTI => [ -1, -1, -1, 0x40, -1, -1, -1, -1, -1, -1, -1, -1 ],
    RTS => [ -1, -1, -1, 0x60, -1, -1, -1, -1, -1, -1, -1, -1 ],
    SBC => [ 0xE9, 0xED, 0xE5, -1, 0xE1, 0xF1, 0xF5, 0xFD, 0xF9, -1, -1, -1 ],
    SEC => [ -1, -1, -1, 0x38, -1, -1, -1, -1, -1, -1, -1, -1 ],

    SED => [ -1, -1, -1, 0xF8, -1, -1, -1, -1, -1, -1, -1, -1 ],
    SEI => [ -1, -1, -1, 0x78, -1, -1, -1, -1, -1, -1, -1, -1 ],
    STA => [ -1, 0x8D, 0x85, -1, 0x81, 0x91, 0x95, 0x9D, 0x99, -1, -1, -1 ],
    STX => [ -1, 0x8E, 0x86, -1, -1, -1, -1, -1, -1, -1, -1, 0x96 ],
    STY => [ -1, 0x8C, 0x84, -1, -1, -1, 0x94, -1, -1, -1, -1, -1 ],

    TAX => [ -1, -1, -1, 0xAA, -1, -1, -1, -1, -1, -1, -1, -1 ],
    TAY => [ -1, -1, -1, 0xA8, -1, -1, -1, -1, -1, -1, -1, -1 ],
    TSX => [ -1, -1, -1, 0xBA, -1, -1, -1, -1, -1, -1, -1, -1 ],
    TXA => [ -1, -1, -1, 0x8A, -1, -1, -1, -1, -1, -1, -1, -1 ],
    TXS => [ -1, -1, -1, 0x9A, -1, -1, -1, -1, -1, -1, -1, -1 ],

    TYA => [ -1, -1, -1, 0x98, -1, -1, -1, -1, -1, -1, -1, -1 ],

# Now, the undocumented ones...

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
  elsif( /^\s*(\w+)\s+A\s*$/ ) { $addr++ }
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

  } elsif( /^\s*(\w+)\s+A\s*$/ ) {
    if( $optable{ $1 }[ $opm{ IMPLIED } ] == -1 )
    { die "$line:$1:accumulator: bad addressing mode" }

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
