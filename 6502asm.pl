#!/usr/bin/perl
# 
# 6502asm.pl
# Created: Sun Jul 25 13:24:14 1999 by tek@wiw.org
# Revised: Sat Aug  7 19:18:38 1999 by tek@wiw.org
# Copyright 1999 Julian E. C. Squires (tek@wiw.org)
# This program comes with ABSOLUTELY NO WARRANTY.
# $Id: 6502asm.pl,v 1.6 2000/07/10 07:07:54 tek Exp $
#
# Wishlist:
#   extensive test suite
#   proper handling of labels for zeropage fu?
#   handling of non-hex addresses... (such as -42 for relative, etc)
#   fix hex idiocy... also, endian of addresses?
#   checking for bad values (eg ranges from parseliteral)
#   DASM compatability mode (including support of label: form)
#   Z80 support
#   local labels ala nasm (.foo)
#   better argument handling, more flags
#

use IO::File;
use strict;

my %opm = ( IMMEDIATE => 0, ABSOLUTE => 1, ZPAGE => 2,
	    IMPLIED => 3, INDX => 4, INDY => 5, ZPAGEX => 6,
	    ABSX => 7, ABSY => 8, RELATIVE => 9, INDIRECT => 10,
	    ZPAGEY => 11 );

# Hacknote: relative ops (like BCC) currently want the same op in
#           both relative and {zpage, absolute}
# 
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
    # There are _lots_ of other NOPs
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
    # SBC is also 0xEB
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

    SLO => [ -1, 0x0F, 0x07, -1, 0x03, 0x13, 0x17, 0x1F, 0x1B, -1, -1, -1 ],
    RLA => [ -1, 0x2F, 0x27, -1, 0x23, 0x33, 0x37, 0x3F, 0x3B, -1, -1, -1 ],
    SRE => [ -1, 0x4F, 0x47, -1, 0x43, 0x53, 0x57, 0x5F, 0x5B, -1, -1, -1 ],
    RRA => [ -1, 0x6F, 0x67, -1, 0x63, 0x73, 0x77, 0x7F, 0x7B, -1, -1, -1 ],
    SAX => [ -1, 0x8F, 0x87, -1, 0x83, -1, -1, 0x9F, -1, -1, -1, 0x97 ],

    LAX => [ -1, 0xAF, 0xA7, -1, 0xA3, 0xB3, -1, 0xBF, 0xBB, -1, -1, 0xB7 ],
    DCP => [ -1, 0xCF, 0xC7, -1, 0xC3, 0xD3, 0xD7, 0xDF, 0xDB, -1, -1, -1 ],
    ISB => [ -1, 0xEF, 0xE7, -1, 0xE3, 0xF3, 0xF7, 0xFF, 0xFB, -1, -1, -1 ],

    ANC => [ 0x0B, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 ], # Also 0x2B
    ASR => [ 0x4B, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 ],
    ARR => [ 0x6B, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 ],
    ANE => [ 0x8B, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 ],
    LXA => [ 0xAB, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 ],
    SBX => [ 0xCB, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 ],

    SHA => [ -1, -1, -1, -1, 0x93, -1, -1, -1, 0x9F, -1, -1, -1 ],
    SHS => [ -1, -1, -1, -1, -1, -1, -1, -1, 0x9B, -1, -1, -1 ],
    SHX => [ -1, -1, -1, -1, -1, -1, -1, -1, 0x9E, -1, -1, -1 ],
    SHY => [ -1, -1, -1, -1, -1, -1, -1, 0x9C, -1, -1, -1, -1 ],

    # JAM, HNG, whatever you will... it wedges the machine. There are
    # a lot more of these.
    JAM => [ -1, -1, -1, 0x02, -1, -1, -1, -1, -1, -1, -1, -1 ],
  );

my %ascommands = ( 'ORG' => 1,
		   'DB' => 1,
		   'DW' => 1
		 );

my %reserved = ( A => 1, X => 1, Y => 1, S => 1, P => 1, PC => 1 );

my ( %symtable, %symbyline );
my @secpass;
my ( $line, $addr, $org, $outfile, $infile ) = ( 0, 0, 0x8000, undef, undef );
my ( $val, $lastsym );
my %flags = ( VERBOSE => 0, CASESENSE => 0 );

# $<hex>, 0x<hex>, 0<octal>, 0b<binary>, <decimal>
my $literal = '(?:([+-]?0[xX][0-9A-Fa-f]+)|(?:\$([+-]?[0-9A-Fa-f]+))|([+-]?0[0-7]+)|([+-]?0[bB][01]+)|([+-]?\d+))';

sub parseliteral {
  if(defined($_[0])) { return hex($_[0]) }
  elsif(defined($_[1])) { return hex($_[1]) }
  elsif(defined($_[2])) { return oct($_[2]) }
  elsif(defined($_[3])) { return oct($_[3]) }
  elsif(defined($_[4])) { return $_[4] }
  else { return undef }
}

while(scalar(@ARGV)) {
  $_ = shift @ARGV;
  if($_ eq '-v') {
    $flags{VERBOSE} = 1;
  } elsif($_ eq '-C') {
    $flags{CASESENSE} = 1;
  } else {
    if( defined( $infile ) ) { die "Too many input files. (sorry)\n" }
    $infile = $_;
  }
}

if( !defined( $infile ) ) {
  die "Insufficient arguments. (you need to specify at least an input file)\n"
}

my $in = new IO::File( $infile, "r" ) or die "$infile: $!\n";
if( $infile =~ m/(\.s)|(\.asm)^/ ) {
  $outfile = $infile;
  $outfile =~ s/(\.s)|(\.asm)$/.bin/;
} else {
  $outfile = $infile . '.bin';
}
my $out = new IO::File( $outfile, "w" ) or die "$outfile: $!\n";

my $tmp;

$line = 0;
$addr = $org;
PASS1: while( $_ = $in->getline() ) {
  $line++;

  # Eat comments
  $_ =~ s/;.*$//;

  chomp; $_ = uc unless $flags{CASESENSE};

  if( $_ =~ /^(\.)?(\w+)(\s+.*)?$/ ) {
    $val = $2;
    if(defined($1)) { 
      if(!defined($lastsym)) {
        die "$line:.$2: No previous symbol to use for local!\n";
      }
      $val = $lastsym . '.' . $val;
    } else {
      $lastsym = $val;
    }

    if( defined( $reserved{ $val } ) or defined( $optable{ $val } ) or
        defined( $ascommands{ $val } ) ) {
      die "$line:$val: Reserved symbol\n"
    }

    $symbyline{ $line } = $val;

    if( defined( $symtable{ $symbyline{ $line } } ) ) {
      die "$line:$val: Duplicate symbol\n"
    }

    if( $flags{VERBOSE} == 1 ) {
      print "defined $symbyline{$line}: ".unpack("H*",pack("n",$addr))."\n"
    }

    $symtable{ $symbyline{ $line } } = $addr;

    $_ =~ s/^\.?\w+//
  }

  $tmp = "\$".unpack("H*",pack("n",$addr));
  s/\*/$tmp/;

  # Eat blank lines
  if( $_ =~ m/^\s*$/ ) { next PASS1 }

  $secpass[ $#secpass + 1 ] = $_;

  if( $_ !~ m/^\s*(\w+).*$/ ) { die "$line:$_: parse error\n" }
  if( defined( $ascommands{ $1 } ) ) {
    if( $1 eq 'ORG' ) {
      if( /^\s*(\w+)\s+$literal\s*$/ ) {
	$addr = parseliteral($2, $3, $4, $5, $6);
	if(!defined($addr) || $addr > 65535 || $addr < 0) {
          die "$line: bad argument to ORG\n"
	}
      } else {
        die "$line: bad argument to ORG\n"
      }
    }

    next PASS1;
  }
  if( !defined( $optable{ $1 } ) ) { die "$line:$1: bad opcode\n" }

  if( /^\s*(\w+)\s+\#?$literal(?:\s*,?\s*[XY])?\s*$/ or
      /^\s*(\w+)\s+\($literal(?:\)\s*,?\s*X)|(?:\s*,?\s*Y\))\s*$/ ) {
    $val = parseliteral($2, $3, $4, $5, $6);
    if( !defined($val) ) {
      die "$line: bad argument\n";
    } elsif( $val >= -128 and $val <= 255 ) {
      $addr += 2;
    } elsif( $val >= -32768 and $val <= 65535) {
      $addr += 3;
    } else {
      die "$line:$2: out of bounds\n"
    }
  }
  elsif( /^\s*(\w+)\s+(\.)?(\w+)(?:\s*,?\s*[XY])?\s*$/ or
         /^\s*(\w+)\s+\(\.)?(\w+)(?:\s*,?\s*Y\))|(?:\)\s*,?\s*X)|\)\s*$/ ) {
    if(defined($2)) {
      s/\.(\w+)/$lastsym.$3/;
    }

    $addr += 3 
  }
  elsif( /^\s*(\w+)\s*$/ ) { $addr++ }
  elsif( /^\s*(\w+)\s+A\s*$/ ) { $addr++ }
  else { die "$line:$_: parse error\n" }
}

$line = 0;
$addr = $org;
PASS2: for $_ ( @secpass ) {
  $line++;

  if( $_ !~ m/^\s*(\w+).*$/ ) { die "$line:$_: parse error\n" }
  if( defined( $ascommands{ $1 } ) ) {
    if( $1 eq 'ORG' ) {
      if( /^\s*(\w+)\s+$literal\s*$/ ) {
	$addr = parseliteral( $2, $3, $4, $5, $6 );
      } else {
	die "$line:$2: bad argument to ORG\n"
      }
    } elsif( $1 eq 'DB' ) {
      if( /^\s*(\w+)\s+$literal\s*$/ ) {
	$out->print( pack("C*", parseliteral( $2, $3, $4, $5, $6 ) ) );
      } else {
	die "$line:$2: bad argument to DB\n"
      }
    } elsif( $1 eq 'DW' ) {
      if( /^\s*(\w+)\s+$literal\s*$/ ) {
	$out->print( pack("C*", parseliteral( $2, $3, $4, $5, $6 ) ) );
      } else {
	die "$line:$2: bad argument to DW\n"
      }
    }

    next PASS2;
  }
  if( !defined( $optable{ $1 } ) ) { die "$line:$1: bad opcode\n" }

  if( /^\s*(\w+)\s+\#$literal\s*$/ ) {
    if( $optable{ $1 }[ $opm{ IMMEDIATE } ] == -1 )
    { die "$line:$1:immediate: bad addressing mode\n" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ IMMEDIATE } ] ) .
		 pack("C", parseliteral( $2, $3, $4, $5, $6 ) ) )

  } elsif( /^\s*(\w+)\s+$literal\s*$/ ) {
    if( $optable{ $1 }[ $opm{ RELATIVE } ] != -1 ) {
      $_ = pack("C", parseliteral( $2, $3, $4, $5, $6 ) - ( $addr + 1 ) );
      $addr += 2;

    } else {
      if( $optable{ $1 }[ $opm{ ABSOLUTE } ] == -1 )
      { die "$line:$1:absolute: bad addressing mode\n" }

      $_ = pack("v", parseliteral( $2, $3, $4, $5, $6 ) );
      $addr += 3;
    }

    $out->print( chr( $optable{ $1 }[ $opm{ ABSOLUTE } ] ) . $_ )

  } elsif( /^\s*(\w+)\s+(\w+)\s*$/ and defined( $symtable{ $2 } ) ) {
    if( $optable{ $1 }[ $opm{ RELATIVE } ] != -1 ) {
      $_ = pack("C", $symtable{ $2 } - ( $addr + 1 ) );
      $addr += 2;

    } else {
      if( $optable{ $1 }[ $opm{ ABSOLUTE } ] == -1 )
      { die "$line:$1:absolute: bad addressing mode\n" }

      $_ = pack("v", $symtable{ $2 } );
      $addr += 3;
    }

    $out->print( chr( $optable{ $1 }[ $opm{ ABSOLUTE } ] ) . $_ )

  } elsif( /^\s*(\w+)\s+$literal\s*$/ ) {
    if( $optable{ $1 }[ $opm{ RELATIVE } ] != -1 ) {
      $_ = pack("C", parseliteral( $2, $3, $4, $5, $6 ) - ( $addr + 1 ) );

    } else {
      if( $optable{ $1 }[ $opm{ ZPAGE } ] == -1 )
      { die "$line:$1:zero-page: bad addressing mode\n" }

      $_ = pack("c", parseliteral( $2, $3, $4, $5, $6 ) );
    }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ ZPAGE } ] ) . $_ )

  } elsif( /^\s*(\w+)\s*$/ ) {
    if( $optable{ $1 }[ $opm{ IMPLIED } ] == -1 )
    { die "$line:$1:implied: bad addressing mode\n" }

    $addr++;
    $out->print( chr( $optable{ $1 }[ $opm{ IMPLIED } ] ) )

  } elsif( /^\s*(\w+)\s+A\s*$/ ) {
    if( $optable{ $1 }[ $opm{ IMPLIED } ] == -1 )
    { die "$line:$1:accumulator: bad addressing mode\n" }

    $addr++;
    $out->print( chr( $optable{ $1 }[ $opm{ IMPLIED } ] ) )

  } elsif( /^\s*(\w+)\s+\($literal\)\s*$/ ) {
    if( $optable{ $1 }[ $opm{ INDIRECT } ] == -1 )
    { die "$line:$1:indirect absolute: bad addressing mode\n" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ INDIRECT } ] ) .
		 pack("v*", parseliteral( $2, $3, $4, $5, $6 ) ) )

  } elsif( /^\s*(\w+)\s+\((\w+)\)\s*$/ and defined( $symtable{ $2 } ) ) {
    if( $optable{ $1 }[ $opm{ INDIRECT } ] == -1 )
    { die "$line:$1:indirect absolute: bad addressing mode\n" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ INDIRECT } ] ) .
		 pack("v", $symtable{ $2 } ) )

  } elsif( /^\s*(\w+)\s+$literal\s*,?\s*X\s*$/ ) {
    if( $optable{ $1 }[ $opm{ ABSX } ] == -1 )
    { die "$line:$1:absolute indexed by X: bad addressing mode\n" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ ABSX } ] ) .
		 pack("v", parseliteral( $2, $3, $4, $5, $6 ) ) )

  } elsif( /^\s*(\w+)\s+(\w+)\s*,?\s*X\s*$/ and defined( $symtable{ $2 } ) ) {
    if( $optable{ $1 }[ $opm{ ABSX } ] == -1 )
    { die "$line:$1:absolute indexed by X: bad addressing mode\n" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ ABSX } ] ) .
		 pack("v", $symtable{ $2 } ) )

  } elsif( /^\s*(\w+)\s+$literal\s*,?\s*Y\s*$/ ) {
    if( $optable{ $1 }[ $opm{ ABSY } ] == -1 )
    { die "$line:$1:absolute indexed by Y: bad addressing mode\n" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ ABSY } ] ) .
		 pack("v", parseliteral( $2, $3, $4, $5, $6 ) ) )

  } elsif( /^\s*(\w+)\s+(\w+)\s*,?\s*Y\s*$/ and defined( $symtable{ $2 } ) ) {
    if( $optable{ $1 }[ $opm{ ABSY } ] == -1 )
    { die "$line:$1:absolute indexed by Y: bad addressing mode\n" }

    $addr += 3;
    $out->print( chr( $optable{ $1 }[ $opm{ ABSY } ] ) .
		 pack("v", $symtable{ $2 } ) )

  } elsif( /^\s*(\w+)\s+$literal\s*,\s*?X\s*$/ ) {
    if( $optable{ $1 }[ $opm{ ZPAGEX } ] == -1 )
    { die "$line:$1:zero-page indexed by X: bad addressing mode\n" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ ZPAGEX } ] ) .
		 pack("c", parseliteral( $2, $3, $4, $5, $6 ) ) )

  } elsif( /^\s*(\w+)\s+$literal\s*,\s*?Y\s*$/ ) {
    if( $optable{ $1 }[ $opm{ ZPAGEY } ] == -1 )
    { die "$line:$1:zero-page indexed by Y: bad addressing mode\n" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ ZPAGEY } ] ) .
		 pack("c", parseliteral( $2, $3, $4, $5, $6 ) ) )

  } elsif( /^\s*(\w+)\s+\($literal\s*,?\s*X\)\s*$/ ) {
    if( $optable{ $1 }[ $opm{ INDX } ] == -1 )
    { die "$line:$1: indexed indirect by X: bad addressing mode\n" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ INDX } ] ) .
		 pack("v", parseliteral( $2, $3, $4, $5, $6 ) ) )

  } elsif( /^\s*(\w+)\s+\($literal\s*\)\s*,?\s*Y\s*$/ ) {
    if( $optable{ $1 }[ $opm{ INDY } ] == -1 )
    { die "$line:$1: indirect indexed by Y: bad addressing mode\n" }

    $addr += 2;
    $out->print( chr( $optable{ $1 }[ $opm{ INDY } ] ) .
		 pack("v", parseliteral( $2, $3, $4, $5, $6 ) ) )

  } else { die "$line:$_: parse error\n" }
}

# EOF 6502asm.pl
