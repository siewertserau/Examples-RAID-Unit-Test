#ifndef RAIDLIB_H
#define RAIDLIB_H

#define OK (0)
#define ERROR (-1)
#define TRUE (1)
#define FALSE (0)

#define SECTOR_SIZE (512)

void xorLBA(unsigned long long *LBA1,
	    unsigned long long *LBA2,
	    unsigned long long *LBA3,
	    unsigned long long *LBA4,
	    unsigned long long *PLBA);

void rebuildLBA(unsigned long long *LBA1,
	        unsigned long long *LBA2,
	        unsigned long long *LBA3,
	        unsigned long long *PLBA,
	        unsigned long long *RLBA);


int checkEquivLBA(unsigned long long *LBA1,
		  unsigned long long *LBA2);

#endif
