/*  Ta2As, Intel(TAsm) to AT&T(As) converter.
 */
/** @file main.c
 *     Main module for the Ta2As converter executable.
 * @par Purpose:
 *     Allows to compile the converter into command line tool.
 * @author   Frank Van Dijk
 * @author   Tomasz Lis
 * @date     22 Jul 1994 - 30 Oct 2010
 * @par  Copying and copyrights:
 *     This program is copyrighted (c) 1994 Frank Van Dijk.
 *     You may freely redistribute it in a non-commercial
 *     environment, provided that NO payment is charged.
 *     See legal.txt for complete text of the license.
 */
#include <stdio.h>
#include <string.h>
#include "ta2as.h"

struct CmdOptions {
	char *in_fname;
	char *out_fname;
	int cc_embeded;
};

int getCommandOptions(struct CmdOptions *opt, int argc, char *argv[])
{
	if (argc != 3) {
	    puts("Converts Tasm intel assembler to AT&T syntax (GNU As)\n\r"
	         "Usage: Ta2As <inputfile> <outputfile>\n\n\r");
	    return 1;
    }
	opt->in_fname = argv[1];
	opt->out_fname = argv[2];
	opt->cc_embeded = 0;
	return 0;
}

int main(int argc, char *argv[])
{
	static struct CmdOptions opt;
	FILE *in,*out;
	int line_len;
	static ChBuf line_buf;
    static AsmLine ln;
    static AsmCodeProps props;
    puts("Ta2As "PROG_VERSION" - (c) 1994 Frank Van Dijk, 2010 Tomasz Lis\n\r");
	// Analyze command line parameters
	if (getCommandOptions(&opt, argc, argv) != 0)
		return 1;
    // Open files
	in = fopen(opt.in_fname,"r");
	if (!in) {
	    puts("Couldn't open input file\n\r");
		return 1;
	}
	out = fopen(opt.out_fname,"w");
	if (!out) {
		fclose(in);
	    puts("Couldn't open output file\n\r");
		return 1;
	}
    memset(&props,0,sizeof(AsmCodeProps));
	memset(line_buf,0,LINE_MAX_LENGTH+1);
	// Sweep through assembly lines and convert
	while (fgets(line_buf, LINE_MAX_LENGTH, in))
	{
	    // Initialize variables
	    memset(&ln,0,sizeof(AsmLine));
	    line_len = strlen(line_buf);
	    // Get rid of the EOL character
	    if (line_len > 0)
	    	line_buf[line_len-1] = '\0';
	    chopIntelAssemblyLine(line_buf, &ln);
	    changeAssemblyLineToAtnt(&ln, &props);
	    linkAtntAssemblyLine(&ln,line_buf);
	    if (opt.cc_embeded)
	        makeAssemblyLineCCEmbeded(line_buf);
        fputs(line_buf,out);
	    fputs("\n",out);
    }
	fclose(in);
	fclose(out);
	return 0;
}
