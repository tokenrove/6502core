;
; A little test for 6502asm
;
moo	ADC #$42
	ADC $8002, Y
	ADC bar
bar     ASL
        BCC moo
	AND quux, X
quux
	ASL $05, X
        ASL $06

