/* f-dirprecheck-v1.1/regextest.c */
/* simple test code to check regexes from config.h */

#include <stdlib.h>
#include <stdio.h>
#include <regex.h>

/* our user config settings */
#include CONFIG_H

int main()
{
    int bad = 0;

#if USEREGEXLIST == 1
#define REGEXNUM	(sizeof regexlist / sizeof regexlist[0])
    {
	unsigned int n;
	regex_t reg;

	for (n = 0; n < REGEXNUM; n++) {
	    if (regexlist[n].regex) {
		if (regcomp(&reg, regexlist[n].regex, REG_EXTENDED) != 0) {
		    printf("Cannot compile: %s\n", regexlist[n].regex);
		    bad = 1;
		}
		else regfree(&reg);
	    }

	    if (regexlist[n].pathmatch) {
		if (regcomp(&reg, regexlist[n].pathmatch, REG_EXTENDED) != 0) {
		    printf("Cannot compile: %s\n", regexlist[n].pathmatch);
		    bad = 1;
		}
		else regfree(&reg);
	    }

	    if (regexlist[n].pathignore) {
		if (regcomp(&reg, regexlist[n].pathignore, REG_EXTENDED) != 0) {
		    printf("Cannot compile: %s\n", regexlist[n].pathignore);
		    bad = 1;
		}
		else regfree(&reg);
	    }
	}
    }
#endif
#if CROSSSECTIONTESTS == 1
#define CROSSNUM	(sizeof crosscheck / sizeof crosscheck[0])
    {
	unsigned int n;
	regex_t reg;

	for (n = 0; n < CROSSNUM; n++) {
	    if (!crosscheck[n].cwdregex) {
		printf("Cross-section test %d doesnt have a cwdpathregex.\n", n);
		bad = 1;		    
	    }
	    else {
		if (regcomp(&reg, crosscheck[n].cwdregex, REG_EXTENDED) != 0) {
		    printf("Cannot compile: %s\n", crosscheck[n].cwdregex);
		    bad = 1;
		}
		else regfree(&reg);
		    
	    }
	}
    }
#endif
    if (bad) return 1;
    return 0;
}	
