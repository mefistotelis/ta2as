/*  Ta2As, Intel(TAsm) to AT&T(As) converter.
 */
/** @file ta2as.h
 *     Header file for the Ta2As converter, ta2as.c.
 * @par Purpose:
 *     Allows to use the converter from another source file.
 * @author   Frank Van Dijk
 * @author   Tomasz Lis
 * @date     21 Jul 1994 - 30 Oct 2010
 * @par  Copying and copyrights:
 *     This program is copyrighted (c) 1994 Frank Van Dijk.
 *     You may freely redistribute it in a non-commercial
 *     environment, provided that NO payment is charged.
 *     See legal.txt for complete text of the license.
 */
#define PROG_VERSION "0.8.2"
#define LINE_MAX_OPERANDS 32
#define LINE_MAX_LENGTH 255

typedef char ChBuf[LINE_MAX_LENGTH+1];

enum OpFlags {
    Op_SizeByte   = 0x01,
    Op_SizeWord   = 0x02,
    Op_SizeDWord  = 0x04,
    Op_SizeQWord  = 0x08,
    Op_SizeOWord  = 0x10,
};
#define Op_SizeMask (Op_SizeByte|Op_SizeWord|Op_SizeDWord|Op_SizeQWord|Op_SizeOWord)

/** Single operand in an assembly line. */
typedef struct {
	ChBuf txt;
	short flags;
} Operand;

enum ComFlags {
    Com_DataDef      = 0x01,
    Com_Directive    = 0x02,
    Com_IOInstr      = 0x04,
    Com_JmpInstr     = 0x08,
    Com_ManyOperands = 0x80,
};

/** Content of an assembly line. */
typedef struct {
    ChBuf label;
    ChBuf com;
    short com_flags;
    ChBuf rem;
    Operand op[LINE_MAX_OPERANDS];
    int op_len;
} AsmLine;

/** Information required for conversion which applies to the whole file. */
typedef struct {
  //TODO: Make a buffer here to store all labels in the input file.
} AsmCodeProps;

int chopIntelAssemblyLine(const ChBuf iline,AsmLine *ln);

void changeAssemblyLineToAtnt(AsmLine *ln,AsmCodeProps *props);

void linkAtntAssemblyLine(const AsmLine *ln,ChBuf oline);
void makeAssemblyLineCCEmbeded(ChBuf line);
