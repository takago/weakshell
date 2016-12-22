/* weak-shell2  TAKAGO_LAB. 2016,12/21

   対応： リダイレクト・パイプ
   未対応： cd

     echo hello
     cat -n  < /etc/os-release   |  grep URL |   sort |cat -n > log.txt   
	    cat -n  < /etc/os-release > log.txt 
		cat -n   -E  /etc/os-release  
		cat -E  /etc/os-release | sort | grep URL | cat -n 

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
#define MAX_ARGS 10	// 1コマンドあたりの最大の引数の個数
#define MAX_CMDS 5	// 起動する最大のコマンド数

int _M;							// 1行に含まれるコマンド数

// 10個まで引数をとることができる．クォートはだめ
char *_argv[MAX_CMDS][MAX_ARGS];	// ポインタの配列
	// parse()後はcmd_argv[コマンド番号][引数番号]で取り出せる
char _fname0[BUFSZ];
char _fname1[BUFSZ];


/* 入力行をトークンに区切り，各コマンドライン引数を_argv[][]に格納する 
 *  _argv[コマンド番号][引数番号]と_fname0/1を作成する
 */
void parseLine(char *str)
{
	int n = 0;

	static char buf[BUFSZ];
	char *line = buf;
	char *p;
	char *cmd;
	char *fp0, *fp1;			// リダイレクトで使われるファイル名処理用のポインタ

	_M = 0;
	_fname0[0] = '\0';
	_fname1[0] = '\0';

	int m;
	for (n = 0; n < MAX_ARGS; n++) {
		for (m = 0; m < MAX_CMDS; m++) {	
			_argv[m][n] = NULL;
		}
	}

	strcpy(buf, str);
	while ((p = strtok_r(line, "|", &line)) != NULL) {	/* まずパイプ部分で分離 */
		fp0 = fp1 = p;

		/* リダイレクトの部分を分離 */
		strtok_r(fp1, ">", &fp1);	// 標準出力
		strtok_r(fp0, "<", &fp0);	// 標準入力

		/* 先頭の空白文字を読み飛ばす */
		while (isspace(*p))
			p++;
		while (isspace(*fp1))
			fp1++;
		while (isspace(*fp0))
			fp0++;

		/* 末尾の空白文字を消去 */
		while (isspace(p[strlen(p) - 1]))
			p[strlen(p) - 1] = '\0';
		while (isspace(fp1[strlen(fp1) - 1]))
			fp1[strlen(fp1) - 1] = '\0';
		while (isspace(fp0[strlen(fp0) - 1]))
			fp0[strlen(fp0) - 1] = '\0';

		/* ポインタの配列に格納 */
		n = 0;
		while ((cmd = strtok_r(p, " ", &p)) != NULL) {
			_argv[_M][n++] =cmd;			
		}
		_M++;
		if (*fp1 != '\0')
			strcpy(_fname1, fp1);
		if (*fp0 != '\0')
			strcpy(_fname0, fp0);
	}
}

void viewToken()
{
	fprintf(stderr, "\x1b[31m------- COMMAND LINE -----------\n");
	int m, n;
	if (_fname0[0] == '\0')
		fprintf(stderr, "IN:\t(tty)\n");
	else
		fprintf(stderr, "IN:\t%s\n", _fname0);
	for (m = 0; m < _M; m++) {
		n = 0;
		while (_argv[m][n] != NULL) {
			fprintf(stderr, "\t%s\n", _argv[m][n]);
			n++;
		}
		if (m < _M-1)
			fprintf(stderr, "PIPE:\n");
	}
	if (_fname1[0] == '\0')
		fprintf(stderr, "OUT:\t(tty)\n\n");
	else
		fprintf(stderr, "OUT:\t%s\n\n", _fname1);
	fprintf(stderr, "--------------------------------\n\x1b[39m");
}

void execLine()
{
	int fd[MAX_CMDS + 1][2];	// dup2()で使う
	int m = 0;
	int k;

	int ffd;					// リダイレクト用のファイルを開くための一時的な変数

	// 使用するファイルディスクリプタ
	fd[0][0] = 0;	// 最初の入力は標準入力
	for (k = 1; k < _M; k++) {
		pipe(fd[k]);	// パイプはfork()する前に生成
	}
	fd[_M][1] = 1;	// 最後の出力は標準出力

	// 子プロセスの生成
	for (m = 0; m < _M; m++) {
		if (fork() == 0) {	// リダイレクトはfork()した後で設定
			if ((m == 0) && (_fname0 != NULL)) {	// リダイレクトの設定(入力)
				ffd = open(_fname0, O_RDONLY);
				dup2(ffd, 0);
				close(ffd);
			}
			if ((m == _M - 1) && (_fname1 != NULL)) {	// リダイレクトの設定(出力)
				unlink(_fname1);	// ファイルを消す       
				ffd = open(_fname1, O_WRONLY | O_CREAT, 0666);	// 書き込み先ファイルを作る
				dup2(ffd, 1);
				close(ffd);
			}

			dup2(fd[m][0], 0);	// 標準入力の設定
			dup2(fd[m + 1][1], 1);	// 標準出力の設定
			for (k = 1; k < _M; k++) {	// 使わないFD（パイプのFD）を閉じる  
				close(fd[k][0]);
				close(fd[k][1]);
			}
			execvp(_argv[m][0], _argv[m]);
			exit(1);
		}
	}
	// 使わないFD（パイプのFD）を閉じる
	for (k = 1; k < _M; k++) {
		close(fd[k][0]);
		close(fd[k][1]);
	}
	// 全子プロセスの終了を待つ
	for (m = 0; m < _M; m++) {
		wait(NULL);
	}

}

int main()
{
	char buf[BUFSZ];
	printf("wsh2>> ");
	while (fgets(buf, BUFSZ, stdin) != NULL) {
		parseLine(buf);
		viewToken();
		execLine();
		printf("wsh2>> ");
	}
	return 0;
}
