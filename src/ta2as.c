/* TA2AS CONVERTER MODULE,
   COPYRIGHT 1994 FRANK VAN DIJK, LAST UPDATE M6-21-94	11:05
*/

#include "ta2as.h"
#include <stdlib.h>
#include <string.h>

char rarray[38][4]=
{
"ah\0",
"al\0",
"ax\0",
"bh\0",
"bl\0",
"bp\0",
"bx\0",
"ch\0",
"cl\0",
"cr0",
"cr1",
"cr2",
"cr3",
"cx\0",
"dh\0",
"di\0",
"dl\0",
"dr0",
"dr1",
"dr2",
"dr3",
"dr4",
"dr5",
"dr6",
"dr7",
"dx\0",
"eax",
"ebp",
"ebx",
"ecx",
"edx",
"edi",
"esi",
"esp",
"si\0",
"sp\0",
"tr6",
"tr7"
};

#define numofmods 8
modstruct mods[numofmods]=
{
{"dosseg\0",NULL},
{"end\0\0\0\0",NULL},
{"equ\0\0\0\0",EquReshuffle},
{"extrn\0\0",NULL},
{"ideal\0\0",NULL},
{"model\0\0",NULL},
{"p386\0\0\0",NULL},
{"stack\0\0",NULL},
};

#define numofreps 19
char replace[numofreps*2][8]=
{
"cbw","cbtw",
"cdq","cltd",
"cmpsd","cmpsl",
"codeseg",".text",
"cwd","cwtd",
"cwde","cwtl",
"dataseg",".data",
"db",".byte",
"dd",".int",
"dw",".short",
"insd","insl",
"lodsd","lodsl",
"movsd","movsl",
"movsx","movs",
"movzx","movz",
"outsd","outsl",
"public",".globl",
"scasd","scasl",
"stosd","stosl",
};

void EquReshuffle(cbuf lab,cbuf com,cbuf rem,oprd *op,int hoeveel) 
{
strcpy(com,lab);
strcat(com,",");
strcpy(lab,".equ");
}

int ReplaceMissch(cbuf lab,cbuf com,cbuf rem,oprd *op,int hoeveel)
{
char *p;
if(p=bsearch(com,replace,numofreps,16,(void*)stricmp))
	{
	strcpy(com,p+8);
	return 0;
	}
if (p=bsearch(lab,replace,numofreps,16,(void*)stricmp))
	{
	strcpy(lab,p+8);
	return 1;
	}
}

void ModLine(cbuf lab,cbuf com,cbuf rem,oprd *op,int *hoeveel)
{
modstruct *p;
int waslabel;
if(p=bsearch(com,mods,numofmods,sizeof(modstruct),(void *)stricmp))
	{
	waslabel=0;
	goto roepmodder;
	}
else if(p=bsearch(lab,mods,numofmods,sizeof(modstruct),(void *)stricmp))
	{
	waslabel=1;
roepmodder:
	if(p->p)
		{
		if(p->p==EquReshuffle) waslabel=1; /*ieuw*/
		p->p(lab,com,rem,op,*hoeveel);
		}
	else *lab=*com=*hoeveel=0;
	}
else waslabel=ReplaceMissch(lab,com,rem,op,*hoeveel);
if(!waslabel && (waslabel=strlen(lab)) && *((lab+=waslabel)-1)!=':')
	{
	*(lab)=':';
	*(lab+1)='\0';
	}
}

int IsReg(cbuf op)
{
cbuf buf;
int len;
char temp;
strncpy(buf,op,len=strcspn(op," \t"));
*(buf+len)=0;
return (int)bsearch(strlwr(buf),rarray,38,4,(void *)strcmp);
}

void RemoveWs(cbuf op)
{
int len=strlen(op);
do
	{
	if(*op==' '||*op=='\t')
		{
		memmove(op,op+1,--len+1);
		}
	} while(*(op++)!=0);
}

void CrackMemInd(oprd *op)
{
int len;
cbuf disp,base,ofs,multi;
char temp;
char oldchar[2];
char *here,*opg;
disp[0]=multi[0]=0;
((unsigned short*)base)[0]=((unsigned short*)ofs)[0]=37;
((unsigned short*)oldchar)[0]=0;
opg=op->op;
opg++;
opg+=strspn(opg," \t");
len=*((int *)opg)&0xdfdfdfdf;
if(len==*(int*)"BYTE" && isspace(*(opg+4)))
	{
	op->flags|=1;
	opg+=5;
	}
else if(len==*(int*)"WORD" && isspace(*(opg+4)))
	{
	op->flags|=2;
	opg+=5;
	}
else if(len==*(int*)"DWOR" && (*(opg+4)&0xdf)=='D' && isspace(*(opg+5)))
	{
	op->flags|=4;
	opg+=6;
	}
RemoveWs(opg);
do
	{
	len=strcspn(opg,"+-]");
	temp=*(opg+len);
	*(opg+len)=0;
	if(here=strchr(opg,'*'))
		{
		*here=0;
		if(IsReg(opg))
			{
			strcpy(ofs+1,opg);
			strcpy(multi,here+1);
			}
		else if(IsReg(here+1))
			{
			strcpy(ofs+1,here+1);
			strcpy(multi,opg);
			}
		else
			{
			*here='*';
			goto concatdisp;
			}
		}
	else
		{
		if(IsReg(opg))
			{
			if(*(base+1)) strcpy(ofs+1,opg);
			else strcpy(base+1,opg);
			}
		else
			{
concatdisp:
			strcat(disp,oldchar);
			strcat(disp,opg);
			}
		}
	if(temp==']') break;
	*oldchar=temp;
	opg+=len+1;
	} while(1);
opg=op->op;
*opg=0;
strcat(opg,*disp=='+'?disp+1:disp);
if(*(base+1)||*(ofs+1))
	{
	strcat(opg,"(");
	if (*(base+1)) strcat(opg,base);
	if (*(ofs+1))
		{
		strcat(opg,",");
		strcat(opg,ofs);
		if(*multi)
			{
			strcat(opg,",");
			strcat(opg,multi);
			}
		}
	strcat(opg,")");
	}
}

void HexBin(oprd *op)
{
int len,len2;
char *opg=op->op;
while(len=strcspn(opg,"0123456789"),opg+=len,isdigit(*opg))
	{
	len2=strspn(opg,"0123456789abcdefABCDEF");
	if(len && (isalpha(*(opg-1)) || *(opg-1)=='@' || *(opg-1)=='_'))
		{
		}
	else
		{
		char ch;
		if((*(opg+len2)&0xdf)=='H')
			{
			ch='x';
frot:
			memmove(opg+len2+1,opg+len2,strlen(opg+len2)+1);
			memmove(opg+2,opg,len2);
			*opg='0';
			*(++opg)=ch;
			}
		else if((*(opg+len2-1)&0xdf)=='B')
			{
			len2--;
			ch='B';
			goto frot;
			}
		}
	opg+=len2;
	}
}

int Operand2Operand(oprd *op)
{
char *temp,*opg;
opg=op->op;
opg+=strspn(opg," \t");
if(*opg=='[')
	{
	CrackMemInd(op);
	}
else if(!strnicmp(opg,"offset",6))
	{
	temp=opg+6;
	temp+=strspn(temp," \t");
	*--temp='$';
	memmove(opg,temp,strlen(temp)+1);
	}
else if(IsReg(opg))
	{
	int len=strcspn(opg," \t");
	if (len==2)
		{
		if (opg[len-1]=='l' || opg[len-1]=='h') op->flags|=1;
		else op->flags|=2;
		}
	else op->flags|=4;
	memmove(opg+1,opg,len+1);
	*opg='%';
	}
else
	{
	memmove(opg+1,opg,strlen(opg)+1);
	*opg='$';
	}
HexBin(op);
{
int len=strlen(opg)-1;
while(opg[len]==' '||opg[len]=='\t')
  opg[len--]=0;
}
return 1;
}

int ChopEm(cbuf in,cbuf lab,cbuf com,cbuf rem,oprd *op,int *hoeveel)
{
char *p;
int len,teller=0;
if(!isspace(in[0])) /*label*/
	{
	if(*in==';') goto restisremark;
	len=strcspn(in," \t;");
	strncpy(lab,in,len);
	*(lab+len)=0;
	in+=len;
	}
else *lab=0;

in+=strspn(in," \t");
if(*in==';') goto restisremark;
len=strcspn(in," \t;");
strncpy(com,in,len);
*(com+len)=0;
in+=len;
if(*in==';')
	goto restisremark;
in+=strspn(in," \t");
while(*in!=0 && *in!=';'&& teller<32)
	{
    len=strcspn(in,",;");
	strncpy(op[teller].op,in,len);
	*(op[teller++].op+len)=0;
	/*if (*(in+len)==',') len++;*/
    in+=strspn(in+=len," \t,");
	}

if (*in==';')
restisremark:
	{
	*in='#';
	strcpy(rem,in);
	}
*hoeveel=teller;
return 1;
}

