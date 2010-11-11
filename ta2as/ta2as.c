/*  Ta2As, Intel(TAsm) to AT&T(As) converter.
 */
/** @file ta2as.c
 *     Conversion module the Ta2As converter.
 * @par Purpose:
 *     Stores source code of the actual converter.
 * @author   Frank Van Dijk
 * @author   Tomasz Lis
 * @date     21 Jul 1994 - 30 Oct 2010
 * @par  Copying and copyrights:
 *     This program is copyrighted (c) 1994 Frank Van Dijk.
 *     You may freely redistribute it in a non-commercial
 *     environment, provided that NO payment is charged.
 *     See legal.txt for complete text of the license.
 */
#include "ta2as.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef void (*modfunc)(AsmLine *ln);

typedef struct {
    char wrd[8];
    modfunc p;
    int waslab;
} ModStruct;

typedef struct {
    char src[8];
    char dst[8];
} ReplaceStruct;

typedef struct {
    char name[8];
    short size;
} RegStruct;

typedef struct {
    ChBuf disp;
    ChBuf base;
    ChBuf index;
    ChBuf scale;
    short flags;
} MemoryAddress;


/** List of recognized CPU registers.
 *  Must be sorted in order to make bsearch() work.
 */
const RegStruct regslist[] = {
    {"ah",1},
    {"al",1},
    {"ax",2},
    {"bh",1},
    {"bl",1},
    {"bp",2},
    {"bx",2},
    {"ch",1},
    {"cl",1},
    {"cr0",4},
    {"cr1",4},
    {"cr2",4},
    {"cr3",4},
    {"cr4",4},
    {"cx",2},
    {"dh",1},
    {"di",2},
    {"dl",1},
    {"dr0",4},
    {"dr1",4},
    {"dr2",4},
    {"dr3",4},
    {"dr4",4},
    {"dr5",4},
    {"dr6",4},
    {"dr7",4},
    {"dx",2},
    {"eax",4},
    {"ebp",4},
    {"ebx",4},
    {"ecx",4},
    {"edi",4},
    {"edx",4},
    {"esi",4},
    {"esp",4},
    {"si",2},
    {"sp",2},
    {"tr6",4},// Test register
    {"tr7",4} // Test register
};
#define REGSLIST_LEN (sizeof(regslist)/sizeof(regslist[0]))

int operandDataTypeSize(const char **op);
void eqSgnReshuffle(AsmLine *ln);
void equReshuffle(AsmLine *ln);
void textequReshuffle(AsmLine *ln);

/** List of modifier routines for specific keywords.
 *  Must be sorted in order to make bsearch() work.
 */
const ModStruct wrdmods[] = {
    {"=",eqSgnReshuffle,1},
    {"dosseg",NULL,0},
    {"end",NULL,0},
    {"equ",equReshuffle,1},
    {"extrn",NULL,0},
    {"ideal",NULL,0},
    {"model",NULL,0},
    {"p386",NULL,0},
    {"stack",NULL,0},
    {"textequ",textequReshuffle,1},
};
#define WORD_MODS_LEN (sizeof(wrdmods)/sizeof(wrdmods[0]))

/** List of pairs, to replace specific words.
 *  Must be sorted in order to make bsearch() work.
 */
const ReplaceStruct wrdreplace[] = {
    {"align",".align"},
    {"cbw","cbtw"},
    {"cdq","cltd"},
    {"cmpsd","cmpsl"},
    {"codeseg",".text"},
    {"cwd","cwtd"},
    {"cwde","cwtl"},
    {"dataseg",".data"},
    {"db",".byte"},
    {"dd",".int"}, // may also be float
    {"ddq",".octet"}, //TODO if there is 'double quad' in AT&T, put it here
    {"do",".octet"}, //TODO if there is 'double quad' in AT&T, put it here
    {"dq",".quad"}, // may also be double
    {"dt",".octet"}, // TODO check what should be here; not sure if it's double quad
    {"dw",".short"},
    {"insd","insl"},
    {"lodsd","lodsl"},
    {"movsd","movsl"},
    {"movsx","movs"},
    {"movzx","movz"},
    {"outsd","outsl"},
    {"public",".globl"},
    {"scasd","scasl"},
    {"stosd","stosl"},
};
#define WORD_REPLACE_LEN (sizeof(wrdreplace)/sizeof(wrdreplace[0]))

/** For strings in form "LABEL = NUMBER".
 */
void eqSgnReshuffle(AsmLine *ln)
{
    const char *opg;
    int idx;
    idx=0;
    if (ln->op_len != 1)
        return;
    opg = ln->op[idx].txt;
    if (operandDataTypeSize(&opg) > 0) {
        // after type, there should be 'ptr'; skip it
        opg += strspn(opg," \t"); // skip white spaces
        if ((strncasecmp(opg,"ptr",3) == 0) && isspace(opg[3]))
        {
            opg += 4;
            opg += strspn(opg," \t");
        }
        memmove(ln->op[idx].txt,opg,strlen(opg)+1);
    }
}

void equReshuffle(AsmLine *ln)
{
    strcpy(ln->com,ln->label);
    strcat(ln->com,",");
    strcpy(ln->label,".equ");
}

void textequReshuffle(AsmLine *ln)
{
    strcpy(ln->com,ln->label);
    strcat(ln->com,",");
    strcpy(ln->label,".textequ");
}

/** Uses wrdreplace[] array to replace Intel syntax words with AT&T replacements.
 */
int checkWordsReplace(AsmLine *ln)
{
    ReplaceStruct *p;
    p = (ReplaceStruct *)bsearch(ln->com,wrdreplace,WORD_REPLACE_LEN,sizeof(ReplaceStruct),(void*)strcasecmp);
    if (p != NULL)
    {
        strcpy(ln->com,p->dst);
        return 0;
    }
    p = (ReplaceStruct *)bsearch(ln->label,wrdreplace,WORD_REPLACE_LEN,sizeof(ReplaceStruct),(void*)strcasecmp);
    if (p != NULL)
    {
        strcpy(ln->label,p->dst);
        return 1;
    }
    return -1;
}

/** Uses wrdmods[] array to make advanced replacements of Intel syntax words.
 *  For every matching line, a function is called to adjust it to AT&T syntax.
 */
int checkWordsModify(AsmLine *ln)
{
    ModStruct *p;
    int waslabel;
    waslabel = -1;
    p = NULL;
    if (p == NULL)
    {
        p = (ModStruct *)bsearch(ln->com,wrdmods,WORD_MODS_LEN,sizeof(ModStruct),(void *)strcasecmp);
        if (p != NULL)
            waslabel = 0;
    }
    if (p == NULL)
    {
        p = (ModStruct *)bsearch(ln->label,wrdmods,WORD_MODS_LEN,sizeof(ModStruct),(void *)strcasecmp);
        if (p != NULL)
            waslabel = 1;
    }
    if (p != NULL)
    {
        if(p->p != NULL)
        {
            waslabel = p->waslab;
            p->p(ln);
        } else
        {
            ln->label[0] = '\0';
            ln->com[0] = '\0';
            ln->op_len = 0;
        }
    }
    return waslabel;
}

/** If given operand is a register in Intel syntax, returns the register size.
 */
int cpuRegisterSize(const ChBuf op)
{
    RegStruct *p;
    ChBuf buf;
    int len;
    len = strcspn(op," \t");
    strncpy(buf,op,len);
    buf[len]= '\0';
    p = (RegStruct *)bsearch(strlwr(buf),regslist,REGSLIST_LEN,sizeof(RegStruct),(void *)strcmp);
    if (p != NULL)
        return p->size;
    return 0;
}

/** Returns if given command is a data definition.
 */
int isDataDefinition(const ChBuf com)
{
    if (strcasecmp(com,".byte") == 0)
        return 1;
    if (strcasecmp(com,".short") == 0)
        return 1;
    if (strcasecmp(com,".int") == 0)
        return 1;
    if (strcasecmp(com,".quad") == 0)
        return 1;
    if (strcasecmp(com,".equ") == 0)
        return 1;
    if (strcasecmp(com,"=") == 0)
        return 1;
    return 0;
}

/** Returns if given command is an IO instruction.
 */
int isIOInstruction(const ChBuf com)
{
    if (strcasecmp(com,"out") == 0)
        return 1;
    if (strcasecmp(com,"in") == 0)
        return 1;
    return 0;
}

/** Returns if given command is a jump instruction.
 */
int isJumpInstruction(const ChBuf com)
{
    if ((com[0] == 'j') || (com[0] == 'J'))
        return 1;
    return 0;
}

void strTrimLeft(ChBuf op)
{
    int len = strlen(op);
    int skip;
    for (skip=0; op[skip] != '\0'; skip++)
    {
        if((op[skip] != ' ') && (op[skip] != '\t'))
            break;
    }
    if (skip > 0)
    {
        memmove(op,op+skip,len+1-skip);
    }
}

void strTrimRight(ChBuf op)
{
    int len = strlen(op)-1;
    while((op[len] == ' ') || (op[len] == '\t'))
    {
        op[len] = '\0';
        len--;
    }
}

/** If given operand starts with data size definition, returns the size.
 *  Increases 'op' pointer so that it skips the definition.
 */
int operandDataTypeSize(const char **op)
{
    static const RegStruct varkindlist[] = {
        {"byte\0",1},
        {"word\0",2},
        {"dword\0",4},
        {"qword\0",8},
    };
    int idx,len;
    for (idx = 0; idx < sizeof(varkindlist)/sizeof(varkindlist[0]); idx++)
    {
        len = strlen(varkindlist[idx].name);
        if (strncasecmp((*op),varkindlist[idx].name,len) == 0)
        {
            if (isspace((*op)[len]))
            {
                (*op) += len;
                return varkindlist[idx].size;
            }
        }
    }
    return 0;
}

int parseOperandDataType(const char **op, MemoryAddress *maddr)
{
    int len;
    len = operandDataTypeSize(op);
    switch (len)
    {
    case 1:
        maddr->flags |= Op_SizeByte;
        break;
    case 2:
        maddr->flags |= Op_SizeWord;
        break;
    case 4:
        maddr->flags |= Op_SizeDWord;
        break;
    case 8:
        maddr->flags |= Op_SizeQWord;
        break;
    case 16:
        maddr->flags |= Op_SizeOWord;
        break;
    case 0:
        return 0;
    }
    return 1;
}

/** Analyzes memory addressing operand from Intel syntax parameter.
 *  General input syntax: "TYPE [DISP+BASE+INDEX*SCALE]",
 *   ie "dword [ebx + ecx*4 + mem_location]".
 *   DISP is constant offset, BASE is register, INDEX is register
 *    and SCALE is constant number.
 */
int chopIntelMemoryAddress(const Operand *op, MemoryAddress *maddr)
{
    int len;
    ChBuf wrd;
    char last_opertr[2];
    char *mul_pos;
    const char *opg;
    memset(maddr,0,sizeof(MemoryAddress));
    strcpy(maddr->base,"$");
    strcpy(maddr->index,"$");
    last_opertr[0] = '\0'; last_opertr[1] = '\0';
    opg = op->txt;
    // first word should be length (TYPE) of the variable inside address
    // we will check for TYPE and DISP at start - it may sometimes be before '['
    // but it may also be inside (i think..) so we'll check again later
    if (parseOperandDataType(&opg,maddr)) {
        // after type, there should be 'ptr'; skip it
        opg += strspn(opg," \t"); // skip white spaces
        if ((strncasecmp(opg,"ptr",3) == 0) && isspace(opg[3]))
        {
            opg += 4;
            opg += strspn(opg," \t");
        }
    }
    len = strcspn(opg,"[");
    strncpy(wrd,opg,len);
    wrd[len] = '\0';
    if (len > 0) {
        while ((mul_pos = strchr(wrd,'(')) != NULL)
            memmove(mul_pos,mul_pos+1,strlen(mul_pos));
        while ((mul_pos = strchr(wrd,')')) != NULL)
            memmove(mul_pos,mul_pos+1,strlen(mul_pos));
        strcpy(maddr->disp,wrd);
        last_opertr[0] = '+';
    }
    opg += len+1; // skip to after the '['
    opg += strspn(opg," \t"); // and skip white spaces
    // check for TYPE again - maybe it's inside
    parseOperandDataType(&opg,maddr);
    do
    {
        // Get rid of white chars
        opg += strspn(opg," \t");
        // find an argument of +/- operation and make a separate string out of it
        len = strcspn(opg,"+-]");
        strncpy(wrd,opg,len);
        wrd[len] = '\0';
        // check if the operand is multiplication of two
        mul_pos = strchr(wrd,'*');
        if (mul_pos != NULL)
        {
            // if it is multiplication, analyze operands as offset and multiplier
            *mul_pos = '\0';
            if(cpuRegisterSize(wrd) > 0)
            {
                maddr->index[0] = '%';
                strcpy(maddr->index+1,wrd);
                strcpy(maddr->scale,mul_pos+1);
            } else
            if(cpuRegisterSize(mul_pos+1) > 0)
            {
                maddr->index[0] = '%';
                strcpy(maddr->index+1,mul_pos+1);
                strcpy(maddr->scale,wrd);
            } else
            {
                strcat(maddr->disp,last_opertr);
                strncat(maddr->disp,opg,len);
            }
        } else
        {
            // if it is single, analyze it as base, or offset with multiplier=1
            if(cpuRegisterSize(wrd) > 0)
            {
                if (maddr->base[1]) {
                    maddr->index[0] = '%';
                    strcpy(maddr->index+1,wrd);
                } else {
                    maddr->base[0] = '%';
                    strcpy(maddr->base+1,wrd);
                }
            } else
            {
                strcat(maddr->disp,last_opertr);
                strcat(maddr->disp,wrd);
            }
        }
        // if the addressing bracket closes, finish
        if (opg[len] == ']') break;
        // else store the +/- operation for next iteration
        last_opertr[0] = opg[len];
        opg += len+1;
    } while(1);
    return 1;
}

/** Converts memory address data into operand string with AT&T syntax.
 *  General output syntax: "DISP(BASE,INDEX,SCALE)", ie "mem_location(%ebx,%ecx,4)".
 */
int linkAtntMemoryAddress(Operand *op, const MemoryAddress *maddr)
{
    char *opg;
    opg=op->txt;
    *opg=0;
    op->flags |= maddr->flags;
    strcat(opg,*maddr->disp=='+'?maddr->disp+1:maddr->disp);
    if(*(maddr->base+1)||*(maddr->index+1))
    {
        strcat(opg,"(");
        if (*(maddr->base+1)) strcat(opg,maddr->base);
        if (*(maddr->index+1))
        {
            strcat(opg,",");
            strcat(opg,maddr->index);
            if(*maddr->scale)
            {
                strcat(opg,",");
                strcat(opg,maddr->scale);
            }
        }
        strcat(opg,")");
    }
    return 1;
}

/** Converts hex and bin values inside operand from Intel to AT&T syntax.
 */
void operandHexBinValues2Atnt(Operand *op)
{
    int len,num_len;
    char *opg;
    opg = op->txt;
    while (len=strcspn(opg,"0123456789"),opg+=len,isdigit(*opg))
    {
        num_len = strspn(opg,"0123456789abcdefABCDEF");
        if ((len > 0) && (isalpha(*(opg-1)) || *(opg-1)=='@' || *(opg-1)=='_'))
        {
        } else
        {
            char ch;
            if ((*(opg+num_len)&0xdf)=='H')
            {
                ch='x';
frot:
                memmove(opg+num_len+1,opg+num_len,strlen(opg+num_len)+1);
                memmove(opg+2,opg,num_len);
                *opg='0';
                *(++opg)=ch;
            } else
            if ((*(opg+num_len-1)&0xdf)=='B')
            {
                num_len--;
                ch='B';
                goto frot;
            }
        }
        opg += num_len;
    }
}

/** Converts a single instruction operand from Intel to AT&T syntax.
 */
int operandChange2Atnt(Operand *op, short com_flags)
{
    char *temp,*opg;
    int len;
    opg = op->txt;
    // skip spaces
    opg += strspn(opg," \t");
    if (strchr(opg,'[') != NULL)
    {
        MemoryAddress maddr;
        chopIntelMemoryAddress(op, &maddr);
        // merge back the elements into operand
        linkAtntMemoryAddress(op, &maddr);
        // Check if no size was provided in this operand. This may happen
        // when using memory offset constant defined with "equ" or "=".
        if ((op->flags & Op_SizeMask) == 0) {
            //TODO: we should remember label and size of the memory location, and get the data from there
            //if ((com_flags & Com_ManyOperands) == 0)
            //    op->flags |= Op_SizeDWord; -- assuming constant size would be very bad.
        }
    } else
    if ((strncasecmp(opg,"offset",6) == 0) && isspace(opg[6]))
    {
        temp = opg+7;
        temp += strspn(temp," \t");
        if ((com_flags & Com_DataDef) != 0) // if we have data definition
        { } else
        {
            temp--;
            *temp = '$';
        }
        memmove(opg,temp,strlen(temp)+1);
    } else
    if ((len = cpuRegisterSize(opg)) > 0)
    {
        if (len == 1) {
            op->flags |= Op_SizeByte;
        } else
        if (len == 2) {
            op->flags |= Op_SizeWord;
        } else
        if (len == 4) {
            op->flags |= Op_SizeDWord;
        } else
        if (len == 8) {
            op->flags |= Op_SizeQWord;
        } else
        if (len == 16) {
            op->flags |= Op_SizeOWord;
        }
        memmove(opg+1,opg,strlen(opg)+1);
        *opg = '%';
    } else
    if ((com_flags & Com_JmpInstr) != 0)
    {
        if ((strncasecmp(opg,"short",5) == 0) && isspace(opg[5]))
        {
            temp = opg+6;
            temp += strspn(temp," \t");
            memmove(opg,temp,strlen(temp)+1);
        }
    } else
    if ((com_flags & Com_DataDef) != 0) // if we have data definition
    {
        // nothing to do
    } else
    if (isdigit(*opg)) // if we have immediate numeric value
    {
        memmove(opg+1,opg,strlen(opg)+1);
        *opg = '$';
    } else
    { // then it must be some variable
        memmove(opg+1,opg,strlen(opg)+1);
        *opg = '$';
    }
    operandHexBinValues2Atnt(op);
    strTrimRight(op->txt);
    return 1;
}

/** Parses an assembly line in Intel syntax.
 *  Divides it into label, assembler command with operands, and remark.
 */
int chopIntelAssemblyLine(const ChBuf iline,AsmLine *ln)
{
    int len,op_idx;
    op_idx = 0;
    if (!isspace(iline[0]))
    { // starts with no spaces - label or variable definition
        if(*iline == ';')
            goto remark;
        len = strcspn(iline," \t;");
        strncpy(ln->label,iline,len);
        ln->label[len] = '\0';
        iline += len;
    } else
    { // there's no label
        ln->label[0] = '\0';
    }
    // now get the assembly command in this line
    iline += strspn(iline," \t"); // skip spaces and tabs
    len = strcspn(iline," \t;"); // find length until space, tab or ';'
    // if the line only contains a comment, 'len' may be 0
    strncpy(ln->com,iline,len);
    ln->com[len] = '\0';
    iline += len;
    // if remark starts here, we may skip trying the operands
    if (*iline == ';')
        goto remark;
    iline += strspn(iline," \t");
    // Do the operands
    while ((*iline != 0) && (*iline != ';') && (op_idx < LINE_MAX_OPERANDS))
    {
        len = strcspn(iline,",;");
        strncpy(ln->op[op_idx].txt, iline, len);
        ln->op[op_idx++].txt[len] = '\0';
        iline += len;
        iline += strspn(iline," \t,");
    }

    if (*iline == ';')
remark:
    {
        strcpy(ln->rem,iline);
        ln->rem[0] = '#';
    }
    ln->op_len = op_idx;
    return 1;
}

/** Converts chopped assembly line from Intel syntax into AT&T one.
 */
void changeAssemblyLineToAtnt(AsmLine *ln,AsmCodeProps *props)
{
    int waslabel;
    int label_len;
    int idx;
    waslabel = -1;
    {
        waslabel = checkWordsReplace(ln);
    }
    if (waslabel == -1)
    {
        waslabel = checkWordsModify(ln);
    }
    if (waslabel == 0) {
        label_len = strlen(ln->label);
        if ((label_len > 0) && (ln->label[label_len-1] != ':'))
        {
            ln->label[label_len++] = ':';
            ln->label[label_len] = '\0';
        }
    }
    // set flags determining what the instruction is
    if (isDataDefinition(ln->com))
        ln->com_flags |= Com_DataDef;
    if (isIOInstruction(ln->com))
        ln->com_flags |= Com_IOInstr;
    if (isJumpInstruction(ln->com))
        ln->com_flags |= Com_JmpInstr;
    if (ln->op_len > 1)
        ln->com_flags |= Com_ManyOperands;
    for(idx=0; idx < ln->op_len; idx++)
    {
        operandChange2Atnt(&ln->op[idx],ln->com_flags);
    }
}

/** Links the assembly line described in AsmLine into a single string.
 */
void linkAtntAssemblyLine(const AsmLine *ln,ChBuf oline)
{
    int idx;
    int sizesuf = 0;
    oline[0] = '\0';
    if ((ln->label[0] != '\0') || (ln->com[0] != '\0'))
    {
        strcat(oline,ln->label);
        strcat(oline,"\t");
        sizesuf=0;
        for(idx=0; idx < ln->op_len; idx++)
        {
            sizesuf |= ln->op[idx].flags;
        }
        if((ln->com_flags & Com_IOInstr) && (sizesuf != Op_SizeWord))
            sizesuf &= ~Op_SizeWord;
        strcat(oline,ln->com);
        if (sizesuf & Op_SizeByte)  strcat(oline,"b");
        if (sizesuf & Op_SizeWord)  strcat(oline,"w");
        if (sizesuf & Op_SizeDWord) strcat(oline,"l");
        if (sizesuf & Op_SizeQWord) strcat(oline,"q");
        strcat(oline,"\t");
        // Add operands - order depends on whether it's data definition or not
        if (ln->com_flags & Com_DataDef) {
            for(idx=0; idx < ln->op_len; )
            {
                strcat(oline,ln->op[idx++].txt);
                if(idx < ln->op_len) strcat(oline,",");
            }
        } else {
            for(idx=ln->op_len-1; idx >= 0; )
            {
                strcat(oline,ln->op[idx--].txt);
                if (idx >= 0) strcat(oline,",");
            }
        }
        if (ln->rem[0] != '\0')
            strcat(oline,"\t");
    }
    // If line has a remark, add it to output
    if (ln->rem[0] != '\0')
        strcat(oline,ln->rem);
}

/** Changes the assembly line into format for embedding into c/cpp files.
 */
void makeAssemblyLineCCEmbeded(ChBuf line)
{
    int idx;
    for (idx = 0; idx < strlen(line); idx++)
    {
        // change '%' into '%%' and '\' into '\\'
        if ((line[idx] == '%') || (line[idx] == '\\')) {
            memmove(line+idx+1,line+idx,strlen(line+idx)+1);
            idx++;
            continue;
        }
    }
    strcat(line,"\\n \\");
}
