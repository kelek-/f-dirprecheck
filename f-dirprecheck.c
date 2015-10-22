/* f's simple pre-mkdir checker */
/* f-dirprecheck--release--1.1 2005-10-30 */

/* lots of includes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <unistd.h>

/* our user config settings */
#include CONFIG_H

/* include a hex dump of the config file */
#include "inc-config.h"

#if (USEREGEXLIST == 1 || CROSSSECTIONTESTS == 1)
#include <regex.h>
#endif

#if REDISNUKECHECK == 1
#include <hiredis.h>
#endif

/* some Limits */
#define MAXPATH       	260

/* biggo function to decrement a date string */
/* only supports two formats */
int DecreaseDatedir(char *dir)
{
    char *lastpart, *lastparttmp;

    lastpart = dir;
    while( (lastparttmp = strstr(lastpart,"/")) != NULL ) { lastpart = lastparttmp+1; }

    /* date dirs in MMDD format */
    if (isdigit(lastpart[0]) && isdigit(lastpart[1]) && isdigit(lastpart[2]) && isdigit(lastpart[3]) && lastpart[4] == 0) {
	int month, day;

	month = (lastpart[0]-'0') * 10 + (lastpart[1]-'0');
	day = (lastpart[2]-'0') * 10 + (lastpart[3]-'0');
  
	if (day == 1 && month > 1) {
	    int dayinmonth[] = { 0, 31,28,31,30,31,30,31,31,30,31,30,31 };
	    month--;
	    day = dayinmonth[month];
	}
	else if (day == 1 && month == 1) {
	    month = 12;
	    day = 31;
	}
	else {
	    day--;
	}

	sprintf(lastpart,"%02d%02d", month, day);
	return 1;
    }
     
    /* date dirs in YYYY-MM-DD format */
    if (isdigit(lastpart[0]) && isdigit(lastpart[1]) && isdigit(lastpart[2]) && isdigit(lastpart[3]) && lastpart[4] == '-' &&
	isdigit(lastpart[5]) && isdigit(lastpart[6]) && lastpart[7] == '-' &&
	isdigit(lastpart[8]) && isdigit(lastpart[9]) && lastpart[10] == 0)
    {
	int year,month,day;

	year = (lastpart[0]-'0') * 1000 + (lastpart[1]-'0') * 100 + (lastpart[2]-'0') * 10 + (lastpart[3]-'0');
	month = (lastpart[5]-'0') * 10 + (lastpart[6]-'0');
	day = (lastpart[8]-'0') * 10 + (lastpart[9]-'0');

	if (day == 1 && month > 1) {
	    int dayinmonth[] = { 0, 31,28,31,30,31,30,31,31,30,31,30,31 };
	    month--;
	    day = dayinmonth[month];
	}
	else if (day == 1 && month == 1) {
	    month = 12;
	    day = 31;
	    year--;
	}
	else {
	    day--;
	}

	sprintf(lastpart,"%04d-%02d-%02d", year, month, day);
	return 1;
    }

    return 0;
}

/* main program */

int main(int argc, char *argv[])
{
    const char *cwd, *tomkdir;
    char *invnukedir, invnukedirbuff[MAXPATH];
    int mkdirlen;

    if (argc == 2 && strcasecmp(argv[1],"dumpconfig") == 0) {
	fwrite(confighdata, 1, sizeof confighdata, stdout);
	return 0;
    }

    if (argc < 3) {
	printf("Insufficient arguments. I should be called by glftpd as a pre_dir_check script.\n");
	return 0;
    }

    tomkdir = argv[1];
    cwd = argv[2];
    mkdirlen = strlen(tomkdir);
 
    /* Checks for annoying .2 .3 etc uploads */
    if (mkdirlen > 2 && tomkdir[mkdirlen-2] == '.' &&
	(tomkdir[mkdirlen-1] >= '1' && tomkdir[mkdirlen-1] <= '9'))
    {
	printf("This release looks like a .2 upload attempt. Stop that now!\n");
	return 2;
    }	

    /* Prevents nukes from other sites to be uploaded */
    if (strncasecmp(tomkdir,"NUKED-",6) == 0 ||
	strncasecmp(tomkdir,"NUKED_",6) == 0 ||
	strncasecmp(tomkdir,"_NUKED-",7) == 0 ||
	strncasecmp(tomkdir,"!NUKED-",7) == 0 ||
	strncasecmp(tomkdir,"!!NUKED-",8) == 0 ||
	strncasecmp(tomkdir,"[NUKED]-",8) == 0 ||
	strncasecmp(tomkdir,"(NUKED)-",8) == 0 ||
	strncasecmp(tomkdir,"[NUKED] ",8) == 0 ||
	strncasecmp(tomkdir,"[NUKED]-",8) == 0 ||
	strncasecmp(tomkdir,"![NUKED]-",9) == 0 ||
#ifdef NUKEPREFIX
	strncasecmp(tomkdir,NUKEPREFIX,strlen(NUKEPREFIX)) == 0 ||
#endif
	0)
    {
	printf("This release name looks nuked. Think again before uploading it!\n");
	return 2;
    }

    /* Prevents (incomplete)- links from other sites to be uploaded */
    if (strncasecmp(tomkdir,"(incomplete)-",13) == 0)
    {
	printf("This release name looks like its not completed yet. Think again before uploading it!\n");
	return 2;
    }

    /* Prevent mkdir of ABCDEF/ABCDEF/ABCDEF due to uploader errors */
    {
	int checkedparts = 0;
	const char *lpart, *part;
#if DEBUG == 1
	char dbgout[256];
#endif

	lpart = cwd + strlen(cwd);

	for(part = lpart; part >= cwd && checkedparts < 2; part--) {
	    if (*part == '/') {
		if (lpart-part > 1) {
#if DEBUG == 1
		    memset(dbgout,0,sizeof(dbgout));
		    strncpy(dbgout, part+1, lpart-part-1);
		    printf("Checking dirname part against tomkdir: %s\n", dbgout);
#endif
		    if (lpart-part-1 == strlen(tomkdir) &&
			strncasecmp(tomkdir, part+1, lpart-part-1) == 0) {
			printf("This directory looks exactly like its parent. No ABC/ABC/ABC/ABC/ABC/... please.\n");
			return 2;
		    }

		    checkedparts++;
		}

		lpart = part;	       
	    }
	}
    }


#ifdef NUKEPREFIX
    /* Check for a nuked version of this release */

    /* Figure out the invers nukedir's name. */
    if (strncasecmp(tomkdir,NUKEPREFIX,strlen(NUKEPREFIX)) == 0) {
	strcpy(invnukedirbuff, tomkdir);
	invnukedir = invnukedirbuff + strlen(NUKEPREFIX);
    } else {
	invnukedir = invnukedirbuff;
	strcpy(invnukedir, NUKEPREFIX);
	strcat(invnukedir, tomkdir);
    }

    {
	DIR *d;
	struct dirent *de;

	d = opendir(cwd);
	if (d) {

	    while( (de = readdir(d)) ) {

		if (strcasecmp(de->d_name, tomkdir) == 0) {

		    if (strcmp(de->d_name, tomkdir) == 0) continue;

		    printf("This release has already been uploaded today with a different caseness. No need for it again.\n");
		    return 2;
		}

		if (strcasecmp(de->d_name, invnukedir) == 0) {
		    printf("This release was already nuked today. Think before uploading again!\n");
		    return 2;
		}
	    }
	    closedir(d);
	}
    }

#ifdef LOOKBACKDAYS
    {
	int dayback;
	char daydir[MAXPATH];
	DIR *d;
	struct dirent *de;

	strcpy(daydir, cwd);
	for(dayback = 0; dayback < LOOKBACKDAYS; dayback++) {

	    if (!DecreaseDatedir(daydir)) {
		/* cant decrease day dir string. prolly not a dated dir */
		break;
	    }	

	    d = opendir(daydir);
	    if (!d) {
		/* doesnt seem to exist. oh well */
		break;
	    }

	    while( (de = readdir(d)) ) {

		if (strcasecmp(de->d_name, tomkdir) == 0) {
		    printf("This release has already been uploaded lately. No need for it again.\n");
		    return 2;
		}

		if (strcasecmp(de->d_name, invnukedir) == 0) {
		    printf("This release has already been uploaded (and nuked) lately. No need for it again.\n");
		    return 2;
		}
	    }
	    closedir(d);
	}
    }
#endif
#endif

#if USEREGEXLIST == 1
#define REGEXNUM	(sizeof regexlist / sizeof regexlist[0])
    {
	unsigned int n;
	int match;
	regex_t reg;
	regmatch_t regmatches[2];

	for (n = 0; n < REGEXNUM; n++) {
	       
	    /* pathmatch */
	    if (regexlist[n].pathmatch) {
		if (regcomp(&reg, regexlist[n].pathmatch, REG_EXTENDED | REG_ICASE) != 0) {
		    /* Cannot compile regex. This should be checked by the regextest */
		    continue;
		}
       
		if (regexec(&reg, cwd, 2, regmatches, 0) != 0) {
#if DEBUG == 1
		    printf("regex rule %d pathmatch %s doesnt match %s -> skipped",
			   n, regexlist[n].pathmatch, cwd);
#endif
		    continue;
		}
		regfree(&reg);
	    }

	    /* pathignore */
	    if (regexlist[n].pathignore) {
		if (regcomp(&reg, regexlist[n].pathignore, REG_EXTENDED | REG_ICASE) != 0) {
		    /* Cannot compile regex. This should be checked by the regextest */
		    continue;
		}
       
		if (regexec(&reg, cwd, 2, regmatches, 0) == 0) {
#if DEBUG == 1
		    printf("regex rule %d pathignore %s matches %s -> skipped",
			   n, regexlist[n].pathignore, cwd);
#endif
		    continue;
		}
		regfree(&reg);
	    }

	    /* file regex */
	    if (regcomp(&reg, regexlist[n].regex, REG_EXTENDED | REG_ICASE) != 0) {
		/* Cannot compile regex. This should be checked by the regextest */
		continue;
	    }
	       
	    match = regexec(&reg, tomkdir, 2, regmatches, 0);

	    if ( (regexlist[n].matchtype == REGEX_MUSTMATCH && match != 0) ||
		 (regexlist[n].matchtype == REGEX_NOTMATCH  && match == 0) )
	    {
		printf("%s\n",regexlist[n].message);
		return 2;		    
	    }

#if DEBUG == 1
	    printf("regex rule %d mkdir-pattern %s doesnt match %s -> skipped",
		   n, regexlist[n].regex, tomkdir);
#endif

	    regfree(&reg);
	}  
    }
#endif

#if CROSSSECTIONTESTS == 1
#define CROSSNUM	(sizeof crosscheck / sizeof crosscheck[0])
    {
	unsigned int n;
	regex_t reg;
	regmatch_t regmatches[2];
	struct stat stbuf;
	char statdir[MAXPATH];

	for (n = 0; n < CROSSNUM; n++) {

#if DEBUG == 1
	    printf("crosscheck cwdregex %s checkpath %s .\n", crosscheck[n].cwdregex, crosscheck[n].checkpath);
#endif
	       
	    if (!crosscheck[n].cwdregex) continue;
	    if (!crosscheck[n].checkpath) continue;

	    /* current pathmatch */
	    if (regcomp(&reg, crosscheck[n].cwdregex, REG_EXTENDED) != 0) {
		/* Cannot compile regex. This should be checked by the regextest */
#if DEBUG == 1
		printf("crosscheck cwdregex %s doesnt compile.\n", crosscheck[n].cwdregex);
#endif
		continue;
	    }
       
	    if (regexec(&reg, cwd, 2, regmatches, 0) != 0) {
#if DEBUG == 1
		printf("crosscheck cwdregex %d : %s doesnt match %s -> skipped\n",
		       n, crosscheck[n].cwdregex, cwd);
#endif
		continue;
	    }
	    regfree(&reg);

	    /* check if exists in checkpath */
	    snprintf(statdir, sizeof(statdir),
		     "%s/%s", crosscheck[n].checkpath, tomkdir);

	    if (stat(statdir,&stbuf) == 0) {
		printf("This release has already been uploaded in %s lately. No need for it again.\n",
		       crosscheck[n].checkpath);
		return 2;
	    }

	    snprintf(statdir, sizeof(statdir),
		     "%s/%s", crosscheck[n].checkpath, invnukedir);

	    if (stat(statdir,&stbuf) == 0) {
		printf("This release has already been uploaded in %s lately. No need for it again.\n",
		       crosscheck[n].checkpath);
		return 2;
	    }

#if DEBUG == 1
	    printf("crosscheck cwdregex %d matched. but release not found in crosspath %s\n",
		   n, crosscheck[n].checkpath);
#endif
	}
    }
#endif

#if REDISNUKECHECK == 1
        redisContext *rConnection;
        redisReply *rReply;
        rConnection = redisConnect("127.0.0.1", 6379);
        if ( rConnection == NULL || rConnection->err) {
                if (rConnection) {
                        printf("Connection error to redis server: %s\r\n", rConnection->errstr);
                } else {
                        printf("Connection error to redis server: can't allocate redis context\r\n");
                }
                return 2;
        }


        /* select redis database */
        rReply = redisCommand(rConnection, "SELECT 1");
        freeReplyObject(rReply);

        /* check if release is already in set */
        rReply = redisCommand(rConnection, "SISMEMBER nuked_release %s", argv[1]);
        if (rReply->integer > 0) {
                printf("%s was already nuked.\r\n", argv[1]);
                /* clean up */
                freeReplyObject(rReply);
                redisFree(rConnection);

                return 2;
        }

        /* clean up */
        freeReplyObject(rReply);
        redisFree(rConnection);


#endif

    /* Nothing found */
    return 0;
}
