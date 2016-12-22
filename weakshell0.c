/* weak-shell0  TAKAGO_LAB. 2016,12/21

   未対応： リダイレクト・パイプ
   未対応： cd

     echo hello
     cat -n  /etc/os-release  

*/

#include<string.h>
#include<stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSZ 256
#define MAX_ARGS 10

// 10個まで引数をとることができる．
char *_argv[MAX_ARGS];			// ポインタの配列


/* 入力行をトークンに区切り，各コマンドライン引数を_argv[]に格納する 
 *  _argv[引数番号]を作成する
 */
void parseLine(char *str)
{
	int n = 0;

	static char buf[BUFSZ];
	char *p = buf;
	char *cmd;
	for (n = 0; n < MAX_ARGS; n++) {
		_argv[n] = NULL;
	}
	strcpy(buf, str);
	n = 0;
	while ((cmd = strtok_r(p, " \n", &p)) != NULL) {	/* まずスペース部分で分離 */
		/* ポインタの配列に格納 */
		_argv[n++] = cmd;
	}
}

void viewToken()
{
	fprintf(stderr, "\x1b[31m------- COMMAND LINE -----------\n");
	int n;
	n = 0;
	while (_argv[n] != NULL) {
		fprintf(stderr, "\t%s\n", _argv[n]);
		n++;
	}
	fprintf(stderr, "--------------------------------\n\x1b[39m");
}

void execLine()
{
	// 子プロセスの生成
	if (fork() == 0) {
		execvp(_argv[0], _argv);
		exit(1);
	}
	wait(NULL);
}

int main()
{
	char buf[BUFSZ];
	printf("wsh0>> ");
	while (fgets(buf, BUFSZ, stdin) != NULL) {
		parseLine(buf);
		viewToken();
		execLine();
		printf("wsh0>> ");
	}
	return 0;
}
