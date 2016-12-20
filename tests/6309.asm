	org	$34

fred	rmb	1
	
	org	$e000

Reset
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

	;  ADDR
	lda	#27
	lde	#99
	ldf	#$42
	addr	a,e

	ldd	#$1234
	ldw	#$dead
	addr	w,d

done	bra	done

	
	org	$fffe
	fdb	Reset
