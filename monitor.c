/*
 * Copyright 2001 by Arto Salmi and Joze Fabcic
 * Copyright 2006-2008 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of GCC6809.
 *
 * GCC6809 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GCC6809 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GCC6809; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "6809.h"
#include "monitor.h"
#include <ctype.h>
#include <signal.h>

/* The function call stack */
struct function_call fctab[MAX_FUNCTION_CALLS];

/* The top of the function call stack */
struct function_call *current_function_call;

/* Automatically break after executing this many instructions */
int auto_break_insn_count = 0;

int monitor_on = 0;

int dump_every_insn = 0;

enum addr_mode
{
  _illegal, _implied, _imm_byte, _imm_word, _direct, _extended,
  _indexed, _rel_byte, _rel_word, _reg_post, _sys_post, _usr_post,
#ifdef H6309
  _imm_direct, _imm_quad, _reg_reg, _single_bit, _blk_move
#endif
};

enum opcode
{
  _undoc, _abx, _adca, _adcb, _adda, _addb, _addd, _anda, _andb,
  _andcc, _asla, _aslb, _asl, _asra, _asrb, _asr, _bcc, _lbcc,
  _bcs, _lbcs, _beq, _lbeq, _bge, _lbge, _bgt, _lbgt, _bhi,
  _lbhi, _bita, _bitb, _ble, _lble, _bls, _lbls, _blt, _lblt,
  _bmi, _lbmi, _bne, _lbne, _bpl, _lbpl, _bra, _lbra, _brn,
  _lbrn, _bsr, _lbsr, _bvc, _lbvc, _bvs, _lbvs, _clra, _clrb,
  _clr, _cmpa, _cmpb, _cmpd, _cmps, _cmpu, _cmpx, _cmpy, _coma,
  _comb, _com, _cwai, _daa, _deca, _decb, _dec, _eora, _eorb,
  _exg, _inca, _incb, _inc, _jmp, _jsr, _lda, _ldb, _ldd,
  _lds, _ldu, _ldx, _ldy, _leas, _leau, _leax, _leay, _lsra,
  _lsrb, _lsr, _mul, _nega, _negb, _neg, _nop, _ora, _orb,
  _orcc, _pshs, _pshu, _puls, _pulu, _rola, _rolb, _rol, _rora,
  _rorb, _ror, _rti, _rts, _sbca, _sbcb, _sex, _sta, _stb,
  _std, _sts, _stu, _stx, _sty, _suba, _subb, _subd, _swi,
  _swi2, _swi3, _sync, _tfr, _tsta, _tstb, _tst, _reset,
#ifdef H6309
  _negd, _comd, _lsrd, _rord, _asrd, _rold, _decd, _incd, _tstd,
  _clrd, _oim, _aim, _eim, _addr, _lde, _ldf, _ldw, _dece, _ince, 
  _tste, _clre, _decf, _incf, _tstf, _clrf, _come, _comf, 
  _ldq, _stq, _sexw, _tim, _pshsw, _pshuw, _pulsw, _puluw,
  _ste, _stf, _adcr, _subr, _sbcr, _andr, _orr, _eorr, _cmpr,
  _asld, _comw, _lsrw, _rorw, _rolw, _decw, _incw, _tstw, _clrw,
  _subw, _cmpw, _sbcd, _andd, _bitd, _eord, _adcd, _ord, _addw,
  _band, _biand, _bor, _bior, _beor, _bieor, _ldbt, _stbt, _stw,
  _tfm, _bitmd, _ldmd, _sube, _cmpe, _adde, _subf, _cmpf, _addf,
  _divd, _divq, _muld
#endif
};

char *mne[] = {
  "???", "ABX", "ADCA", "ADCB", "ADDA", "ADDB", "ADDD", "ANDA", "ANDB",
  "ANDCC", "ASLA", "ASLB", "ASL", "ASRA", "ASRB", "ASR", "BCC", "LBCC",
  "BCS", "LBCS", "BEQ", "LBEQ", "BGE", "LBGE", "BGT", "LBGT", "BHI",
  "LBHI", "BITA", "BITB", "BLE", "LBLE", "BLS", "LBLS", "BLT", "LBLT",
  "BMI", "LBMI", "BNE", "LBNE", "BPL", "LBPL", "BRA", "LBRA", "BRN",
  "LBRN", "BSR", "LBSR", "BVC", "LBVC", "BVS", "LBVS", "CLRA", "CLRB",
  "CLR", "CMPA", "CMPB", "CMPD", "CMPS", "CMPU", "CMPX", "CMPY", "COMA",
  "COMB", "COM", "CWAI", "DAA", "DECA", "DECB", "DEC", "EORA", "EORB",
  "EXG", "INCA", "INCB", "INC", "JMP", "JSR", "LDA", "LDB", "LDD",
  "LDS", "LDU", "LDX", "LDY", "LEAS", "LEAU", "LEAX", "LEAY", "LSRA",
  "LSRB", "LSR", "MUL", "NEGA", "NEGB", "NEG", "NOP", "ORA", "ORB",
  "ORCC", "PSHS", "PSHU", "PULS", "PULU", "ROLA", "ROLB", "ROL", "RORA",
  "RORB", "ROR", "RTI", "RTS", "SBCA", "SBCB", "SEX", "STA", "STB",
  "STD", "STS", "STU", "STX", "STY", "SUBA", "SUBB", "SUBD", "SWI",
  "SWI2", "SWI3", "SYNC", "TFR", "TSTA", "TSTB", "TST", "RESET",
#ifdef H6309
  "NEGD", "COMD", "LSRD", "RORD", "ASRD", "ROLD", "DECD",
  "INCD", "TSTD", "CLRD", "OIM", "AIM", "EIM", "ADDR", "LDE", "LDF", 
  "LDW", "DECE", "INCE", "TSTE", "CLRE", "DECF", "INCF", "TSTF", "CLRF",
  "COME", "COMF", "LDQ", "STQ", "SEXW", "TIM", "PSHSW", "PSHUW", "PULSW",
  "PULUW", "STE", "STF", "ADCR", "SUBR", "SBCR", "ANDR", "ORR", "EORR",
  "CMPR", "ASLD", "COMW", "LSRW", "RORW", "ROLW", "DECW", "INCW", "TSTW",
  "CLRW", "SUBW", "CMPW", "SBCD", "ANDD", "BITD", "EORD", "ADCD", "ORD",
  "ADDW", "BAND", "BIAND", "BOR", "BIOR", "BEOR", "BIEOR", "LDBT", "STBT",
  "STW", "TFM", "BITMD", "LDMD", "SUBE", "CMPE", "ADDE", "SUBF", "CMPF", 
  "ADDF", "DIVD", "DIVQ", "MULD"
#endif
};

typedef struct
{
  UINT8 code;
  UINT8 mode;
} opcode_t;

opcode_t codes[256] = {
  // 00
  {_neg, _direct},
#ifdef H6309
  {_oim, _imm_direct},
  {_aim, _imm_direct},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_com, _direct},
  {_lsr, _direct},
#ifdef H6309
  {_eim, _direct},
#else
  {_undoc, _illegal},
#endif
  {_ror, _direct},
  {_asr, _direct},
  {_asl, _direct},
  {_rol, _direct},
  {_dec, _direct},
#ifdef H6309
  {_tim, _direct},
#else
  {_undoc, _illegal},
#endif
  {_inc, _direct},
  {_tst, _direct},
  {_jmp, _direct},
  {_clr, _direct},
  // 10
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_nop, _implied},
  {_sync, _implied},
#ifdef H6309
  {_sexw, _implied},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_lbra, _rel_word},
  {_lbsr, _rel_word},
  {_undoc, _illegal},
  {_daa, _implied},
  {_orcc, _imm_byte},
  {_undoc, _illegal},
  {_andcc, _imm_byte},
  {_sex, _implied},
  {_exg, _reg_post},
  {_tfr, _reg_post},
  // 20
  {_bra, _rel_byte},
  {_brn, _rel_byte},
  {_bhi, _rel_byte},
  {_bls, _rel_byte},
  {_bcc, _rel_byte},
  {_bcs, _rel_byte},
  {_bne, _rel_byte},
  {_beq, _rel_byte},
  {_bvc, _rel_byte},
  {_bvs, _rel_byte},
  {_bpl, _rel_byte},
  {_bmi, _rel_byte},
  {_bge, _rel_byte},
  {_blt, _rel_byte},
  {_bgt, _rel_byte},
  {_ble, _rel_byte},
  // 30
  {_leax, _indexed},
  {_leay, _indexed},
  {_leas, _indexed},
  {_leau, _indexed},
  {_pshs, _sys_post},
  {_puls, _sys_post},
  {_pshu, _usr_post},
  {_pulu, _usr_post},
  {_undoc, _illegal},
  {_rts, _implied},
  {_abx, _implied},
  {_rti, _implied},
  {_cwai, _imm_byte},
  {_mul, _implied},
  {_reset, _implied},
  {_swi, _implied},
  // 40
  {_nega, _implied},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_coma, _implied},
  {_lsra, _implied},
  {_undoc, _illegal},
  {_rora, _implied},
  {_asra, _implied},
  {_asla, _implied},
  {_rola, _implied},
  {_deca, _implied},
  {_undoc, _illegal},
  {_inca, _implied},
  {_tsta, _implied},
  {_undoc, _illegal},
  {_clra, _implied},
  // 50
  {_negb, _implied},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_comb, _implied},
  {_lsrb, _implied},
  {_undoc, _illegal},
  {_rorb, _implied},
  {_asrb, _implied},
  {_aslb, _implied},
  {_rolb, _implied},
  {_decb, _implied},
  {_undoc, _illegal},
  {_incb, _implied},
  {_tstb, _implied},
  {_undoc, _illegal},
  {_clrb, _implied},
  // 60
  {_neg, _indexed},
#ifdef H6309
  {_oim, _indexed},
  {_aim, _indexed},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_com, _indexed},
  {_lsr, _indexed},
#ifdef H6309
  {_eim, _indexed},
#else
  {_undoc, _illegal},
#endif
  {_ror, _indexed},
  {_asr, _indexed},
  {_asl, _indexed},
  {_rol, _indexed},
  {_dec, _indexed},
#ifdef H6309
  {_tim, _indexed},
#else
  {_undoc, _illegal},
#endif
  {_inc, _indexed},
  {_tst, _indexed},
  {_jmp, _indexed},
  {_clr, _indexed},
  // 70
  {_neg, _extended},
#ifdef H6309
  {_oim, _extended},
  {_aim, _extended},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_com, _extended},
  {_lsr, _extended},
#ifdef H6309
  {_eim, _extended},
#else
  {_undoc, _illegal},
#endif
  {_ror, _extended},
  {_asr, _extended},
  {_asl, _extended},
  {_rol, _extended},
  {_dec, _extended},
#ifdef H6309
  {_tim, _extended},
#else
  {_undoc, _illegal},
#endif
  {_inc, _extended},
  {_tst, _extended},
  {_jmp, _extended},
  {_clr, _extended},
  // 80
  {_suba, _imm_byte},
  {_cmpa, _imm_byte},
  {_sbca, _imm_byte},
  {_subd, _imm_word},
  {_anda, _imm_byte},
  {_bita, _imm_byte},
  {_lda, _imm_byte},
  {_undoc, _illegal},
  {_eora, _imm_byte},
  {_adca, _imm_byte},
  {_ora, _imm_byte},
  {_adda, _imm_byte},
  {_cmpx, _imm_word},
  {_bsr, _rel_byte},
  {_ldx, _imm_word},
  {_undoc, _illegal},
  // 90
  {_suba, _direct},
  {_cmpa, _direct},
  {_sbca, _direct},
  {_subd, _direct},
  {_anda, _direct},
  {_bita, _direct},
  {_lda, _direct},
  {_sta, _direct},
  {_eora, _direct},
  {_adca, _direct},
  {_ora, _direct},
  {_adda, _direct},
  {_cmpx, _direct},
  {_jsr, _direct},
  {_ldx, _direct},
  {_stx, _direct},
  // A0
  {_suba, _indexed},
  {_cmpa, _indexed},
  {_sbca, _indexed},
  {_subd, _indexed},
  {_anda, _indexed},
  {_bita, _indexed},
  {_lda, _indexed},
  {_sta, _indexed},
  {_eora, _indexed},
  {_adca, _indexed},
  {_ora, _indexed},
  {_adda, _indexed},
  {_cmpx, _indexed},
  {_jsr, _indexed},
  {_ldx, _indexed},
  {_stx, _indexed},
  // B0
  {_suba, _extended},
  {_cmpa, _extended},
  {_sbca, _extended},
  {_subd, _extended},
  {_anda, _extended},
  {_bita, _extended},
  {_lda, _extended},
  {_sta, _extended},
  {_eora, _extended},
  {_adca, _extended},
  {_ora, _extended},
  {_adda, _extended},
  {_cmpx, _extended},
  {_jsr, _extended},
  {_ldx, _extended},
  {_stx, _extended},
  // C0
  {_subb, _imm_byte},
  {_cmpb, _imm_byte},
  {_sbcb, _imm_byte},
  {_addd, _imm_word},
  {_andb, _imm_byte},
  {_bitb, _imm_byte},
  {_ldb, _imm_byte},
  {_undoc, _illegal},
  {_eorb, _imm_byte},
  {_adcb, _imm_byte},
  {_orb, _imm_byte},
  {_addb, _imm_byte},
  {_ldd, _imm_word},
#ifdef H6309
  {_ldq, _imm_quad},
#else
  {_undoc, _illegal},
#endif
  {_ldu, _imm_word},
  {_undoc, _illegal},
  // D0
  {_subb, _direct},
  {_cmpb, _direct},
  {_sbcb, _direct},
  {_addd, _direct},
  {_andb, _direct},
  {_bitb, _direct},
  {_ldb, _direct},
  {_stb, _direct},
  {_eorb, _direct},
  {_adcb, _direct},
  {_orb, _direct},
  {_addb, _direct},
  {_ldd, _direct},
  {_std, _direct},
  {_ldu, _direct},
  {_stu, _direct},
  // E0
  {_subb, _indexed},
  {_cmpb, _indexed},
  {_sbcb, _indexed},
  {_addd, _indexed},
  {_andb, _indexed},
  {_bitb, _indexed},
  {_ldb, _indexed},
  {_stb, _indexed},
  {_eorb, _indexed},
  {_adcb, _indexed},
  {_orb, _indexed},
  {_addb, _indexed},
  {_ldd, _indexed},
  {_std, _indexed},
  {_ldu, _indexed},
  {_stu, _indexed},
  // F0
  {_subb, _extended},
  {_cmpb, _extended},
  {_sbcb, _extended},
  {_addd, _extended},
  {_andb, _extended},
  {_bitb, _extended},
  {_ldb, _extended},
  {_stb, _extended},
  {_eorb, _extended},
  {_adcb, _extended},
  {_orb, _extended},
  {_addb, _extended},
  {_ldd, _extended},
  {_std, _extended},
  {_ldu, _extended},
  {_stu, _extended}
};

opcode_t codes10[256] = {
  // 00
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // 10
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // 20
  {_undoc, _illegal},
  {_lbrn, _rel_word},
  {_lbhi, _rel_word},
  {_lbls, _rel_word},
  {_lbcc, _rel_word},
  {_lbcs, _rel_word},
  {_lbne, _rel_word},
  {_lbeq, _rel_word},
  {_lbvc, _rel_word},
  {_lbvs, _rel_word},
  {_lbpl, _rel_word},
  {_lbmi, _rel_word},
  {_lbge, _rel_word},
  {_lblt, _rel_word},
  {_lbgt, _rel_word},
  {_lble, _rel_word},
  // 30
#ifdef H6309
  {_addr, _reg_reg},
  {_adcr, _reg_reg},
  {_subr, _reg_reg},
  {_sbcr, _reg_reg},
  {_andr, _reg_reg},
  {_orr, _reg_reg},
  {_eorr, _reg_reg},
  {_cmpr, _reg_reg},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
#ifdef H6309
  {_pshsw, _implied},
  {_pulsw, _implied},
  {_pshuw, _implied},
  {_puluw, _implied},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_swi2, _implied},
  // 40
#ifdef H6309
  {_negd, _implied},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_comd, _implied},
  {_lsrd, _implied},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
#ifdef H6309
  {_rord, _implied},
  {_asrd, _implied},
  {_asld, _implied},
  {_rold, _implied},
  {_decd, _implied},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
#ifdef H6309
  {_incd, _implied},
  {_tstd, _implied},
  {_undoc, _illegal},
  {_clrd, _implied},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  // 50
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_comw, _implied},
  {_lsrw, _implied},
  {_undoc, _illegal},
  {_rorw, _implied},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_rolw, _implied},
  {_decw, _implied},
  {_undoc, _illegal},
  {_incw, _implied},
  {_tstw, _implied},
  {_undoc, _illegal},
  {_clrw, _implied},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  // 60
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // 70
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // 80
#ifdef H6309
  {_subw, _imm_word},
  {_cmpw, _imm_word},
  {_sbcd, _imm_word},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_cmpd, _imm_word},
#ifdef H6309
  {_andd, _imm_word},
  {_bitd, _imm_word},
  {_ldw, _imm_word},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
#ifdef H6309
  {_eord, _imm_word},
  {_adcd, _imm_word},
  {_ord, _imm_word},
  {_addw, _imm_word},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_cmpy, _imm_word},
  {_undoc, _illegal},
  {_ldy, _imm_word},
  {_undoc, _illegal},
  // 90
#ifdef H6309
  {_subw, _direct},
  {_cmpw, _direct},
  {_sbcd, _direct},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_cmpd, _direct},
#ifdef H6309
  {_andd, _direct},
  {_bitd, _direct},
  {_ldw, _direct},
  {_stw, _direct},
  {_eord, _direct},
  {_adcd, _direct},
  {_ord, _direct},
  {_addw, _direct},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_cmpy, _direct},
  {_undoc, _illegal},
  {_ldy, _direct},
  {_sty, _direct},
  // A0
#ifdef H6309
  {_subw, _indexed},
  {_cmpw, _indexed},
  {_sbcd, _indexed},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_cmpd, _indexed},
#ifdef H6309
  {_andd, _indexed},
  {_bitd, _indexed},
  {_ldw, _indexed},
  {_stw, _indexed},
  {_eord, _indexed},
  {_adcd, _indexed},
  {_ord, _indexed},
  {_addw, _indexed},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_cmpy, _indexed},
  {_undoc, _illegal},
  {_ldy, _indexed},
  {_sty, _indexed},
  // B0
#ifdef H6309
  {_subw, _extended},
  {_cmpw, _extended},
  {_sbcd, _extended},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_cmpd, _extended},
#ifdef H6309
  {_andd, _extended},
  {_bitd, _extended},
  {_ldw, _extended},
  {_stw, _extended},
  {_eord, _extended},
  {_adcd, _extended},
  {_ord, _extended},
  {_addw, _extended},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_cmpy, _extended},
  {_undoc, _illegal},
  {_ldy, _extended},
  {_sty, _extended},
  // C0
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_lds, _imm_word},
  {_undoc, _illegal},
  // D0
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_ldq, _direct},
  {_stq, _direct},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_lds, _direct},
  {_sts, _direct},
  // E0
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_ldq, _indexed},
  {_stq, _indexed},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_lds, _indexed},
  {_sts, _indexed},
  // F0
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_ldq, _extended},
  {_stq, _extended},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_lds, _extended},
  {_sts, _extended}
};

opcode_t codes11[256] = {
  // 00
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // 10
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // 20
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // 30
#ifdef H6309
  {_band, _single_bit},
  {_biand, _single_bit},
  {_bor, _single_bit},
  {_bior, _single_bit},
  {_beor, _single_bit},
  {_bieor, _single_bit},
  {_ldbt, _single_bit},
  {_stbt, _single_bit},
  {_tfm, _blk_move},
  {_tfm, _blk_move},
  {_tfm, _blk_move},
  {_tfm, _blk_move},
  {_bitmd, _imm_byte},
  {_ldmd, _imm_byte},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_swi3, _implied},
  // 40
  {_undoc, _illegal}, /* 11 40 */
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_come, _implied},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_dece, _implied},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
#ifdef H6309
  {_ince, _implied},
  {_tste, _implied},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
#ifdef H6309
  {_clre, _implied},
#else
  {_undoc, _illegal},
#endif
  // 50
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_comf, _implied},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_decf, _implied},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
#ifdef H6309
  {_incf, _implied},
  {_tstf, _implied},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
#ifdef H6309
  {_clrf, _implied},
#else
  {_undoc, _illegal},
#endif
  // 60
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // 70
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // 80
#ifdef H6309
  {_sube, _imm_byte},
  {_cmpe, _imm_byte},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_cmpu, _imm_word},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_lde, _imm_byte},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_adde, _imm_byte},
#else
  {_undoc, _illegal},
#endif
  {_cmps, _imm_word},
#ifdef H6309
  {_divd, _imm_byte},
  {_divq, _imm_word},
  {_muld, _imm_word},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  // 90
#ifdef H6309
  {_sube, _direct},
  {_cmpe, _direct},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_cmpu, _direct},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_lde, _direct},
  {_ste, _direct},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_adde, _direct},
#else
  {_undoc, _illegal},
#endif
  {_cmps, _direct},
#ifdef H6309
  {_divd, _direct},
  {_divq, _direct},
  {_muld, _direct},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  // A0
#ifdef H6309
  {_sube, _indexed},
  {_cmpe, _indexed},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_cmpu, _indexed},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_lde, _indexed},
  {_ste, _indexed},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_adde, _indexed},
#else
  {_undoc, _illegal},
#endif
  {_cmps, _indexed},
#ifdef H6309
  {_divd, _indexed},
  {_divq, _indexed},
  {_muld, _indexed},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  // B0
#ifdef H6309
  {_sube, _extended},
  {_cmpe, _extended},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_cmpu, _extended},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_lde, _extended},
  {_ste, _extended},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_adde, _extended},
#else
  {_undoc, _illegal},
#endif
  {_cmps, _extended},
#ifdef H6309
  {_divd, _extended},
  {_divq, _extended},
  {_muld, _extended},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  // C0
#ifdef H6309
  {_subf, _imm_byte},
  {_cmpf, _imm_byte},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_ldf, _imm_byte},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_addf, _imm_byte},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // D0
#ifdef H6309
  {_subf, _direct},
  {_cmpf, _direct},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_ldf, _direct},
  {_stf, _direct},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_addf, _direct},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // E0
#ifdef H6309
  {_subf, _indexed},
  {_cmpf, _indexed},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_ldf, _indexed},
  {_stf, _indexed},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  // F0
#ifdef H6309
  {_subf, _extended},
  {_cmpf, _extended},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_ldf, _extended},
  {_stf, _extended},
#else
  {_undoc, _illegal},
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
#ifdef H6309
  {_addf, _extended},
#else
  {_undoc, _illegal},
#endif
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal},
  {_undoc, _illegal}
};

#ifdef H6309
char *reg[] = {
  "D", "X", "Y", "U", "S", "PC", "W", "V",
  "A", "B", "CC", "DP", "??", "??", "E", "F"
};
#else
char *reg[] = {
  "D", "X", "Y", "U", "S", "PC", "??", "??",
  "A", "B", "CC", "DP", "??", "??", "??", "??"
};
#endif

char index_reg[] = { 'X', 'Y', 'U', 'S' };

char *off4[] = {
  "0", "1", "2", "3", "4", "5", "6", "7",
  "8", "9", "10", "11", "12", "13", "14", "15",
  "-16", "-15", "-14", "-13", "-12", "-11", "-10", "-9",
  "-8", "-7", "-6", "-5", "-4", "-3", "-2", "-1"
};

/* Disassemble the current instruction.  Returns the number of bytes that
compose it. */
int dasm (char *buf, absolute_address_t opc)
{
  UINT8 op, am;
  char *op_str;
  absolute_address_t pc = opc;
  char R;
  int fetch1;			/* the first (MSB) fetched byte, used in macro RDWORD */

  op = fetch8();

  if (op == 0x10)
    {
      op = fetch8();
      am = codes10[op].mode;
      op = codes10[op].code;
    }
  else if (op == 0x11)
    {
      op = fetch8();
      am = codes11[op].mode;
      op = codes11[op].code;
    }
  else
    {
      am = codes[op].mode;
      op = codes[op].code;
    }

  op_str = (char *) mne[op];
  buf += sprintf (buf, "%-6.6s", op_str);

  switch (am)
    {
    case _illegal:
      sprintf (buf, "???");
      break;
    case _implied:
      break;
    case _imm_byte:
      sprintf (buf, "#$%02X", fetch8 ());
      break;
    case _imm_word:
      sprintf (buf, "#$%04X", fetch16 ());
      break;
    case _direct:
      sprintf (buf, "<%s", monitor_addr_name (fetch8 ()));
      break;
    case _extended:
      sprintf (buf, "%s", monitor_addr_name (fetch16 ()));
      break;
#ifdef H6309
    case _imm_direct:
      {
	unsigned immval = fetch8();
	const char *ea = monitor_addr_name(fetch8());
	sprintf(buf, "#$%02x,<%s", immval, ea);
      }
      break;

    case _imm_quad:
      {
	unsigned immval = fetch32();
	sprintf(buf, "#$%08x", immval);
      }
      break;

    case _reg_reg:
      {
	unsigned regs = fetch8();
	unsigned r1 = (regs & 0xf0) >> 4;
	unsigned r2 = regs & 0x0f;

	sprintf (buf, "%s,%s", reg[r1], reg[r2]);
      }
      break;

    case _single_bit:
      {
	unsigned bits = fetch8();
	unsigned regbit = bits & 0x07;
	unsigned srcbit = (bits >> 3) & 0x07;
	unsigned regnum = (bits >> 6) & 0x03;
	static char *reg_name[4] = {"CC", "A", "B", "???"};

	sprintf(buf, "%s.%d,%s.%d", reg_name[regnum], regbit, monitor_addr_name(fetch8()), srcbit);
      }
      break;

    case _blk_move:
      {
	unsigned regs = fetch8();
	unsigned src_reg = (regs >> 4) & 0x0f;
	unsigned dst_reg = regs & 0x0f;
	char *src_op = "+";
	char *dst_op = "+";

	if (op == 0x39) {
	  src_op = dst_op = "-";
	}
	else if (op == 0x3a) {
	  dst_op = "";
	}
	else if (op == 0x3b) {
	  src_op = "";
	}

	sprintf(buf, "(%s%s,%s%s)", reg[src_reg], src_op, reg[dst_reg], dst_op);
      }
      break;
#endif

    case _indexed:
      op = fetch8 ();
      R = index_reg[(op >> 5) & 0x3];

      if ((op & 0x80) == 0)
	{
	  sprintf (buf, "%s,%c", off4[op & 0x1f], R);
	  break;
	}

      switch (op & 0x1f)
	{
	case 0x00:
	  sprintf (buf, ",%c+", R);
	  break;
	case 0x01:
	  sprintf (buf, ",%c++", R);
	  break;
	case 0x02:
	  sprintf (buf, ",-%c", R);
	  break;
	case 0x03:
	  sprintf (buf, ",--%c", R);
	  break;
	case 0x04:
	  sprintf (buf, ",%c", R);
	  break;
	case 0x05:
	  sprintf (buf, "B,%c", R);
	  break;
	case 0x06:
	  sprintf (buf, "A,%c", R);
	  break;
	case 0x08:
	  sprintf (buf, "$%02X,%c", fetch8 (), R);
	  break;
	case 0x09:
	  sprintf (buf, "$%04X,%c", fetch16 (), R);
	  break;
	case 0x0B:
	  sprintf (buf, "D,%c", R);
	  break;
	case 0x0C:
	  sprintf (buf, "$%02X,PC", fetch8 ());
	  break;
	case 0x0D:
	  sprintf (buf, "$%04X,PC", fetch16 ());
	  break;
	case 0x11:
	  sprintf (buf, "[,%c++]", R);
	  break;
	case 0x13:
	  sprintf (buf, "[,--%c]", R);
	  break;
	case 0x14:
	  sprintf (buf, "[,%c]", R);
	  break;
	case 0x15:
	  sprintf (buf, "[B,%c]", R);
	  break;
	case 0x16:
	  sprintf (buf, "[A,%c]", R);
	  break;
	case 0x18:
	  sprintf (buf, "[$%02X,%c]", fetch8 (), R);
	  break;
	case 0x19:
	  sprintf (buf, "[$%04X,%c]", fetch16 (), R);
	  break;
	case 0x1B:
	  sprintf (buf, "[D,%c]", R);
	  break;
	case 0x1C:
	  sprintf (buf, "[$%02X,PC]", fetch8());
	  break;
	case 0x1D:
	  sprintf (buf, "[$%04X,PC]", fetch16());
	  break;
	case 0x1F:
	  sprintf (buf, "[%s]", monitor_addr_name (fetch16()));
	  break;
	default:
	  sprintf (buf, "???");
	  break;
	}
      break;

    case _rel_byte:
      fetch1 = ((INT8) fetch8 ());
	   sprintf (buf, "%s", absolute_addr_name (fetch1 + pc));
      break;

    case _rel_word:
	   sprintf (buf, "%s", absolute_addr_name (fetch16() + pc));
      break;

    case _reg_post:
      op = fetch8 ();
      sprintf (buf, "%s,%s", reg[op >> 4], reg[op & 15]);
      break;

    case _usr_post:
    case _sys_post:
      op = fetch8 ();

      if (op & 0x80)
	strcat (buf, "PC,");
      if (op & 0x40)
	strcat (buf, am == _usr_post ? "S," : "U,");
      if (op & 0x20)
	strcat (buf, "Y,");
      if (op & 0x10)
	strcat (buf, "X,");
      if (op & 0x08)
	strcat (buf, "DP,");
      if ((op & 0x06) == 0x06)
	strcat (buf, "D,");
      else
	{
	  if (op & 0x04)
	    strcat (buf, "B,");
	  if (op & 0x02)
	    strcat (buf, "A,");
	}
      if (op & 0x01)
	strcat (buf, "CC,");
      buf[strlen (buf) - 1] = '\0';
      break;

    }
  return pc - opc;
}

int sizeof_file(FILE * file)
{
  int size;

  fseek (file, 0, SEEK_END);
  size = ftell (file);
  rewind (file);

  return size;
}

// nac it would be nice to have an intelligent map file reader..
int load_map_file (const char *name)
{
	FILE *fp;
	char map_filename[256];
	char buf[256];
	char *tok_ptr, *value_ptr, *id_ptr;
	target_addr_t value;
	char *file_ptr;
	struct symbol *sym = NULL;

	/* Try appending the suffix 'map' to the name of the program. */
	sprintf (map_filename, "%s.map", name);
	fp = file_open (NULL, map_filename, "r");
	if (!fp)
	{
		/* If that fails, try replacing any existing suffix. */
		sprintf (map_filename, "%s", name);
		char *s = strrchr (map_filename, '.');
		if (s)
		{
			sprintf (s+1, "map");
			fp = file_open(NULL, map_filename, "r");
		}

		if (!fp)
		{
			fprintf (stderr, "warning: no symbols for %s\n", name);
			return -1;
		}
	}

	printf ("Reading symbols from '%s'...\n", map_filename);
	for (;;)
	{
	        char *bp = fgets (buf, sizeof(buf)-1, fp);
		if (feof (fp))
			break;

                tok_ptr = strtok (buf, " \t\n");
                if (0 != strcmp(tok_ptr, "Symbol:"))
                    continue;

                id_ptr =  strtok(NULL, " \t\n");
                // skip over filename
                tok_ptr = strtok (NULL, " \t\n");
                // skip over "="
                tok_ptr = strtok (NULL, " \t\n");
                value_ptr = strtok (NULL, " \t\n");
                // get value as hex string
                value = (target_addr_t) strtoul(value_ptr, NULL, 16);

		sym_add (&program_symtab, id_ptr, to_absolute (value), 0);
	}

	fclose (fp);
	return 0;
}

/* Auto-detect image file type and load it. For this to work,
   the machine must already be initialized.
*/
int load_image (const char *name)
{
  int count, addr, type;
  FILE *fp;

  fp = file_open(NULL, name, "r");
  if (fp == NULL)
    {
      printf("failed to open image file %s.\n", name);
      return 1;
    }

  if (fscanf (fp, "S%1x%2x%4x", &type, &count, &addr) == 3)
    {
        rewind(fp);
        return load_s19(fp);
    }
  else if (fscanf (fp, ":%2x%4x%2x", &count, &addr, &type) == 3)
    {
        rewind(fp);
        return load_hex(fp);
    }
  else
    {
      printf ("unrecognised format in image file %s.\n", name);
        return 1;
    }
}

int load_hex (FILE *fp)
{
  int count, addr, type, data, checksum;
  int done = 1;
  int line = 0;

  while (done != 0)
    {
      line++;

      if (fscanf (fp, ":%2x%4x%2x", &count, &addr, &type) != 3)
	{
	  printf ("line %d: invalid hex record information.\n", line);
	  break;
	}
      checksum = count + (addr >> 8) + (addr & 0xff) + type;

      switch (type)
	{
	case 0:
	  {
	    int res;

	    for (; count != 0; count--, addr++, checksum += data)
	      {
		if (fscanf(fp, "%2x", &data))
		  {
		    write8(addr, (UINT8) data);
		  }
		else
		  {
		    printf("line %d: hex record data inconsistent with count field.\n", line);
		    break;
		  }
	      }

	    checksum = (-checksum) & 0xff;

	    if ( (fscanf(fp, "%2x", &data) != 1) || (data != checksum) )
	      {
		printf("line %d: hex record checksum missing or invalid.\n", line);
		done = 0;
		break;
	      }
	    res = fscanf(fp, "%*[\r\n]"); /* skip any form of line ending */
	  }
	  break;

	case 1:
	  checksum = (-checksum) & 0xff;

          if ( (fscanf(fp, "%2x", &data) != 1) || (data != checksum) )
	    printf("line %d: hex record checksum missing or invalid.\n", line);
	  done = 0;
	  break;

	case 2:
	default:
	  printf("line %d: not supported hex type %d.\n", line, type);
	  done = 0;
	  break;
	}
    }

  (void) fclose (fp);
  return 0;
}

int load_s19(FILE *fp)
{
  int count, addr, type, data, checksum;
  int done = 1;
  int line = 0;

  while (done != 0)
    {
      line++;

      if (fscanf(fp, "S%1x%2x%4x", &type, &count, &addr) != 3)
	{
	  printf("line %d: invalid S record information.\n", line);
	  break;
	}

      checksum = count + (addr >> 8) + (addr & 0xff);

      switch (type)
	{
	case 1:
	  {
	    int res;
	    for (count -= 3; count != 0; count--, addr++, checksum += data)
	      {
		if (fscanf (fp, "%2x", &data))
                  {
		    write8 (addr, (UINT8) data);
                  }
		else
                  {
                    printf ("line %d: S record data inconsistent with count field.\n", line);
	            break;
                  }
	      }

	    checksum = (~checksum) & 0xff;

	    if ( (fscanf (fp, "%2x", &data) != 1) || (data != checksum) )
	      {
		printf ("line %d: S record checksum missing or invalid.\n", line);
		done = 0;
		break;
	      }
	    res = fscanf (fp, "%*[\r\n]"); /* skip any form of line ending */
	  }
	  break;

	case 9:
	  checksum = (~checksum) & 0xff;
	  if ( (fscanf (fp, "%2x", &data) != 1) || (data != checksum) )
	    printf ("line %d: S record checksum missing or invalid.\n", line);
	  done = 0;
	  break;

	case 0:
	case 5:
	  {
	    // Silently ignore these records
	    int res = fscanf (fp, "%*[\r\n]"); /* skip any form of line ending */
	  }
	  break;

	default:
	  printf ("line %d: S%d not supported.\n", line, type);
	  done = 0;
	  break;
	}
    }

  (void) fclose(fp);
  return 0;
}

void monitor_call (unsigned int flags)
{
#ifdef CALL_STACK
	if (current_function_call <= &fctab[MAX_FUNCTION_CALLS-1])
	{
		current_function_call++;
		current_function_call->entry_point = get_pc ();
		current_function_call->flags = flags;
	}
#endif
#if 0
	const char *id = sym_lookup (&program_symtab, to_absolute (get_pc ()));
	if (id)
	{
		// printf ("In %s now\n", id);
	}
#endif
}

void monitor_return (void)
{
#ifdef CALL_STACK
	if (current_function_call > &fctab[MAX_FUNCTION_CALLS-1])
	{
		current_function_call--;
		return;
	}

	while ((current_function_call->flags & FC_TAIL_CALL) &&
		(current_function_call > fctab))
	{
		current_function_call--;
	}

	if (current_function_call > fctab)
		current_function_call--;
#endif
}

const char* absolute_addr_name (absolute_address_t addr)
{
   static char buf[256], *bufptr;
   const char *name;

   bufptr = buf;

   bufptr += sprintf (bufptr, "%02X:0x%04X", (unsigned)(addr >> 28), (unsigned)(addr & 0xFFFFFF));

   name = sym_lookup (&program_symtab, addr);
   if (name)
      sprintf (bufptr, "  <%-16.16s>", name);

   return buf;
}

const char* monitor_addr_name (target_addr_t target_addr)
{
   static char buf[256], *bufptr;
   const char *name;
   absolute_address_t addr = to_absolute (target_addr);

   bufptr = buf;

   bufptr += sprintf (bufptr, "$%04X", target_addr);

   name = sym_lookup (&program_symtab, addr);
   if (name)
      sprintf (bufptr, "  <%s>", name);

   return buf;
}

static void monitor_signal (int sigtype)
{
   (void) sigtype;
   putchar ('\n');
   monitor_on = 1;
}

void monitor_init (void)
{
   extern int debug_enabled;

   fctab[0].entry_point = read16 (0xfffe);
   memset (&fctab[0].entry_regs, 0, sizeof (struct cpu_regs));
   current_function_call = &fctab[0];

   auto_break_insn_count = 0;
   monitor_on = debug_enabled;
   signal (SIGINT, monitor_signal);
}

int check_break (void)
{
	if (dump_every_insn)
		print_current_insn ();

	if (auto_break_insn_count > 0)
		if (--auto_break_insn_count == 0)
			return 1;
	return 0;
}

void monitor_backtrace (void)
{
	struct function_call *fc = current_function_call;
	while (fc >= &fctab[0]) {
		printf ("%s\n", monitor_addr_name (fc->entry_point));
		fc--;
	}
}

int monitor6809 (void)
{
	int rc;

	signal (SIGINT, monitor_signal);
	rc = command_loop ();
	monitor_on = 0;
	return rc;
}
