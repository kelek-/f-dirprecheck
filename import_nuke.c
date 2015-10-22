#include <hiredis.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <libgen.h>



int main(int argc, char **argv) {
	int rc;
	redisContext *c;
	redisReply *reply, *exists;
	struct timeval timeout = { 1, 500000 }; // 1,5s
	c = redisConnect("127.0.0.1", 6379);
	if (c == NULL || c->err) {
		if (c) {
			printf("Connection error: %s\r\n", c->errstr);
			redisFree(c);
		} else {
			printf("Connection error: can't allocate redis context\r\n");
		}
		exit(1);
	}

	/* select database 1 */
	reply = redisCommand(c, "SELECT 1");
	freeReplyObject(reply);
	
	/* add release if not already in database */
	exists = redisCommand(c, "SISMEMBER nuked_release %s", basename(argv[1]));
	if ( exists->integer < 1 ) {
		reply = redisCommand(c, "SADD nuked_release %s", basename(argv[1]));
		freeReplyObject(reply);
		printf("%s added to set\r\n", basename(argv[1]));
	}
	freeReplyObject(exists);
	redisFree(c);
	exit(0);
}
