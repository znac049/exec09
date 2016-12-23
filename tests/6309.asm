	org	$34

fred	rmb	1
	
	org	$e000

Reset
	; Manipulate D
	ldd	#-27
	negd
	
	; Quad byte instructions
	ldq	quad

	leax	quad,pcr
	ldq	4,x
	
	stq	>fred

	ldq	<fred
	stq	>fred+4

	leax	fred,pcr
	stq	8,x
	
	; Test OIM Direct
	lda	#$42
	sta	<fred
	oim	#$82,<fred
	ldb	<fred

	; Test AIM Direct
	lda	#$42
	sta	<fred
	aim	#$82,<fred
	ldb	<fred

	; Test EIM Direct
	lda	#$42
	sta	<fred
	eim	#$82,<fred
	ldb	<fred

	; ADDR
	lda	#27
	lde	#99
	ldf	#$42
	addr	a,e

	ldd	#$1234
	ldw	#$dead
	addr	w,d

	; 8-bit registers
	lde	#$55
	ldf	#$aa

	come
	comf

	dece
	ince

	decf
	incf

	tste
	tstf

done	bra	done

quad	fqb	$deadface
	fqb	$a5a55a5a
	
	
	org	$fffe
	fdb	Reset
