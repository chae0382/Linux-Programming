#include "smallsh.h"

static char inpbuf[MAXBUF];
static char tokbuf[2*MAXBUF];
static char *ptr = inpbuf;
static char *tok = tokbuf;

static char special[] = {' ', '\t', '&', ';', '\n', '\0'};

int userin(char* p){
	int c, count;
	ptr = inpbuf;
	tok = tokbuf;

	printf("%s$ ", p);
	count = 0;

	while(1){
		if ((c = getchar()) == EOF)
			return EOF;
		if (count < MAXBUF)
			inpbuf[count++] = c;
		if (c == '\n' && count < MAXBUF) {
			inpbuf[count] = '\0';
			return count;
		}
		if (c == '\n' || count >= MAXBUF){
			printf("smallsh: input line too long\n");
			count = 0;
			printf("%s", p);
		}
	}
}

int gettok(char** outptr) {
	int type;
	*outptr = tok;
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	*tok++ = *ptr;
	switch (*ptr++) {
		case '\n':
			type = EOL;
			break;
		case '&':
			type = AMPERSAND;
			break;
		case ';':
			type = SEMICOLON;
			break;
		case '>':
			type = REDIRECTION;
			break;
		case '|':
			type = PIPE;
			break;
		default:
			type = ARG;
			while(inarg(*ptr))
				*tok++ = *ptr++;
	}
	*tok++ = '\0';
	return type;
}

int inarg (char c){
	char *wrk;

	for (wrk = special; *wrk; wrk++) {
		if (c == *wrk)
			return 0;
	}
	return 1;
}

void procline() {
        char *arg[MAXARG + 1];
        int toktype, type;
        int narg = 0;
        int locOfRedir = -1;
        int locOfPipe = -1;
        for (;;) {
                switch (toktype = gettok(&arg[narg])) {
                        case ARG:
                                if (narg < MAXARG)
                                        narg++;
                                break;
                        case REDIRECTION:
                                locOfRedir = narg;
                                if (narg < MAXARG)
                                        narg++;
                                break;
                        case PIPE:
                                locOfPipe = narg;
                                if (narg < MAXARG)
                                        narg++;
                                break;
                        case EOL:
                        case SEMICOLON:
                        case AMPERSAND:
                                if (toktype == AMPERSAND) type = BACKGROUND;
                                else type = FOREGROUND;

                                if (narg != 0) {
                                        arg[narg] = NULL;
                                        if(strcmp(arg[0], "cd")==0){
                                                changedir(arg, narg);
                                        }
                                        else if(strcmp(arg[0], "exit")==0){
                                                exit(0);
                                        }
                                        else{
                                                runcommand(arg, type, locOfRedir, locOfPipe);
                                        }
                                }
                                if (toktype == EOL) return;
                                narg = 0;
                                locOfRedir = -1;
                                break;
                }
        }
}

int changedir(char **cline, int narg){
        if(narg != 2) {
                printf("Usage: cd <directory>\n");
                return -1;
        }
        if(chdir(cline[1]) == -1){
                perror("smallsh");
        }

}

int runcommand(char **cline, int where, int locOfRedir, int locOfPipe) {
        pid_t pid;
        int status;
        static struct sigaction act;

        switch (pid = fork()) {
                case -1:
                        perror("smallsh");
                        return -1;
                case 0:
                        if(where == BACKGROUND){
                                act.sa_handler = SIG_IGN;
                                sigfillset(&(act.sa_mask));
                                sigaction(SIGINT, &act, NULL);
                        }
                        if(locOfRedir != -1){
                                int newfd = open(cline[locOfRedir+1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
                                dup2(newfd, 1);
                                cline[locOfRedir] = NULL;
                                cline[locOfRedir+1] = NULL;
                        }
                        if(locOfPipe != -1){
                                runpipe(cline, locOfPipe);
                        }
                        execvp(*cline, cline);
                        perror(*cline);
                        exit (1);
        }

        if (where == BACKGROUND) {
                printf("[Process id] %d\n", pid);
                return 0;
        }
        if (waitpid(pid, &status, 0) == -1)
                return -1;
        else
                return status;
}

int runpipe(char **cline, int locOfPipe){
        
	int p[2];
	cline[locOfPipe] = NULL;
        char *com1[locOfPipe+1];
        for(int i=0; i<locOfPipe+1; i++){
                com1[i] = cline[i];
        }
        char **com2 = &(cline[locOfPipe+1]);

        if(pipe(p) == -1){
                perror("pipe call");
                exit(1);
        }
        switch (fork()) {
                case -1:
                        perror("smallsh");
                        return -1;
                case 0:
                        dup2(p[1], 1);
                        close(p[0]);
                        close(p[1]);
                        execvp(*com1, com1);
                        perror(*com1);
                        exit(1);
                default:
                        dup2(p[0], 0);
                        close(p[0]);
                        close(p[1]);
                        execvp(*com2, com2);
                        perror(*com2);
                        exit(1);
        }
}

void catchint(int signo){
	printf("\n");
	siglongjmp(position, 1);
}
