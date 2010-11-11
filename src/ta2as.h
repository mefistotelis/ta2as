/* TA2AS CONVERTER HEADER FILE,
   COPYRIGHT 1994 FRANK VAN DIJK, LAST UPDATE M6-21-94	11:04
*/

char rarray[38][4];

typedef char cbuf[256];

typedef struct
	{
	cbuf op;
	short flags;
	} oprd;

typedef void (*modfunc)(cbuf lab,cbuf com,cbuf rem,oprd *op,int hoeveel);

typedef struct
{
char wrd[8];
modfunc p;
} modstruct;

void EquReshuffle(cbuf lab,cbuf com,cbuf rem,oprd *op,int hoeveel);

void PuntErvoor(cbuf lab,cbuf com,cbuf rem,oprd *op,int hoeveel);

void ModLine(cbuf lab,cbuf com,cbuf rem,oprd *op,int *hoeveel);

int IsReg(cbuf op);

void RemoveWs(cbuf op);

void CrackMemInd(oprd *op);

int Operand2Operand(oprd *op);

int ChopEm(cbuf in,cbuf lab,cbuf com,cbuf rem,oprd *op,int *hoeveel);

