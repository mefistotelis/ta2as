/* TA2AS INTEL(TASM) TO AT&T(AS) CONVERTER, MAIN MODULE
   COPYRIGHT 1994 FRANK VAN DIJK, LAST UPDATE M6-21-94	11:03
*/

#include <stdio.h>
#include <string.h>
#include "ta2as.h"

int main(int narg,char** argv)
{
/* Dit is een testmain*/
FILE *in,*uit;
int hoeveel,teller;
cbuf b0,b1,b2,b3;
oprd ops[32];
for(teller=0;teller<32;ops[teller++].flags=0);
if (narg!=3)
	{
	puts("Ta2As 0.8 - Copyright 1994 FRANK VAN DIJK\n\r"
	     "Converts Tasm intel assembler to AT&T syntax (GNU As)\n\r"
	     "Usage: Ta2As inputfile outputfile\n\n\r");
	return 1;
	}
if(!(in=fopen(argv[1],"r"))) return 1;
if(!(uit=fopen(argv[2],"w"))) return 1;
while(fgets(b0,255,in))
	{
	int sizesuf=0;
	*b1=*b2=*b3=0;
	hoeveel=strlen(b0);
	if(hoeveel) b0[hoeveel-1]=0;
	ChopEm(b0,b1,b2,b3,ops,&hoeveel);
	ModLine(b1,b2,b3,ops,&hoeveel);
	if(*b1||*b2)
		{
		fputs(b1,uit);
		fputc('\t',uit);
		sizesuf=0;
		for(teller=0;teller<hoeveel;teller++)
			{
			Operand2Operand(&ops[teller]);
			sizesuf|=ops[teller].flags;
			ops[teller].flags=0;
			}
		if((!strcmp(b2,"out")||!strcmp(b2,"in"))&&sizesuf!=2)
			sizesuf&=~2;
		if (sizesuf&1) strcat(b2,"b");
		if (sizesuf&2) strcat(b2,"w");
		if (sizesuf&4) strcat(b2,"l");
		fputs(b2,uit);
		fputc('\t',uit);
		if(strcmp(b2,".byte")&&strcmp(b2,".short")&&strcmp(b2,".int")&&strcmp(b1,".equ"))
			for(teller=hoeveel-1;teller>=0;)
				{
				fputs(ops[teller--].op,uit);
				if (teller>=0) fputc(',',uit);
				}
		else	for(teller=0;teller<hoeveel;)
				{
				if(*ops[teller].op=='$')
					memmove(ops[teller].op,ops[teller].op+1,strlen(ops[teller].op));
				fputs(ops[teller++].op,uit);
				if(teller<hoeveel) fputc(',',uit);
				}
		if(*b3) fputc('\t',uit);
		}
	if(*b3)
	  fputs(b3,uit);
	fputs("\n",uit);
	}
fclose(in);
fclose(uit);
return 0;
}
