#include "smallsh.h"

int main(){
	
	char pwd[MAXBUF];
	
	static struct sigaction act1, act2;
	
	act1.sa_handler = SIG_IGN;
	act1.sa_flags = SA_NOCLDWAIT | SA_RESTART;
	sigfillset(&(act1.sa_mask));
	sigaction(SIGCHLD, &act1, NULL);

	void catchint(int);	
	
	if(sigsetjmp(position, 1)==0){
	act2.sa_handler = catchint;
        sigfillset(&(act2.sa_mask));
        sigaction(SIGINT, &act2, NULL);
	}

	getcwd(pwd, MAXBUF);
	while(userin(pwd) != EOF){
		procline();
		getcwd(pwd, MAXBUF);
	}
	
	return 0;
}
