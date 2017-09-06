#include <lib.h>

static char* _brk = NULL; //data segment break cache

int brk(void *addr){
	struct message m;
	int ret = 0;
	if(addr != _brk){
		m.p1 = addr;
		ret = _syscall(SYSCALL_BRK,&m);
		_brk = m.p1;
		if(m.i1 != 0)
			return -1;
	}
	return ret;
}

void *sbrk(int incr){
	char *newsize, *oldsize;

	if(_brk == NULL){
		brk(NULL); //initialise _brk
	}

	oldsize = _brk;
	newsize = _brk + incr;
	if ((incr > 0 && newsize < oldsize) || (incr < 0 && newsize > oldsize))
		goto return_err;
		
	if (brk(newsize) == 0)
		return(oldsize);

	return_err:
		return( (void *) -1);
		
}
