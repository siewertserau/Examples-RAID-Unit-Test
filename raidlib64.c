#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include "raidlib64.h"


// RAID-5 encoding
//
// Do XOR 64 bits at a time for efficiency - so 512 byte LBA is 64 long long
// XOR operations in C
//
// This is 80% capacity with 1/5 LBAs used for parity.
//
// Only handles single faults.
//
// PRECONDITIONS:
// 1) LBA pointeres must have memory allocated for them externally
// 2) Blocks pointer to by LBAs are initialized with data
//
// POST-CONDITIONS:
// 1) Contents of PLBA is modified and contains the computed parity using XOR
//
void xorLBA(unsigned long long *LBA1,
	    unsigned long long *LBA2,
	    unsigned long long *LBA3,
	    unsigned long long *LBA4,
	    unsigned long long *PLBA)
{
    int idx;

    // pointer aliases for pointer arithmetic to avoid modifying pointers passed in
    unsigned long long *ptrLBA1=LBA1;
    unsigned long long *ptrLBA2=LBA2;
    unsigned long long *ptrLBA3=LBA3;
    unsigned long long *ptrLBA4=LBA4;
    unsigned long long *ptrPLBA=PLBA;

    for(idx=0; idx<(SECTOR_SIZE/(sizeof(unsigned long long))); idx++)
    {
        // contents of PLBA pointer is contents of the XOR of all other blocks
        *ptrPLBA=(*ptrLBA1)^(*ptrLBA2)^(*ptrLBA3)^(*ptrLBA4);
       
        // advance all pointers by one word size, sizeof(unsigned long long)
        ptrPLBA++;ptrLBA1++;ptrLBA2++;ptrLBA3++;ptrLBA4++;
    }
}


// RAID-5 Rebuild
//
// Provide any 3 of the original LBAs and the Parity LBA to rebuild the RLBA
//
// If the Parity LBA was lost, then it can be rebuilt by simply re-encoding.
// 
void rebuildLBA(unsigned long long *LBA1,
	        unsigned long long *LBA2,
	        unsigned long long *LBA3,
	        unsigned long long *PLBA,
	        unsigned long long *RLBA)
{
    int idx;
    unsigned long long checkParity;
    unsigned long long *ptrLBA1=LBA1;
    unsigned long long *ptrLBA2=LBA2;
    unsigned long long *ptrLBA3=LBA3;
    unsigned long long *ptrPLBA=PLBA;
    unsigned long long *ptrRLBA=RLBA;

    for(idx=0; idx<(SECTOR_SIZE/(sizeof(unsigned long long))); idx++)
    {

        // Parity check word is simply XOR of remaining good LBAs
        checkParity=(*ptrLBA1)^(*ptrLBA2)^(*ptrLBA3);

        // Rebuilt LBA is simply XOR of original parity and parity check word
        // which will preserve original parity computed over the 4 LBAs
        *ptrRLBA =(*ptrPLBA)^(checkParity);
	    
        ptrPLBA++;ptrLBA1++;ptrLBA2++;ptrLBA3++;ptrRLBA++;
    }
}


int checkEquivLBA(unsigned long long *LBA1,
		  unsigned long long *LBA2)
{
    int idx;
    unsigned long long *ptrLBA1=LBA1;
    unsigned long long *ptrLBA2=LBA2;

    for(idx=0; idx<(SECTOR_SIZE/(sizeof(unsigned long long))); idx++)
    {
        if((*ptrLBA1) != (*ptrLBA2))
	{
            printf("EQUIV CHECK MISMATCH @ word %d: LBA1=0x%llx, LBA2=0x%llx\n", idx, (*ptrLBA1), (*ptrLBA2));
	    return ERROR;
	}

	ptrLBA1++;ptrLBA2++;
    }

    return OK;
}

void printLBA(unsigned long long *LBA)
{
    int idx;
    unsigned long long *ptrLBA=LBA;

    for(idx=0; idx<(SECTOR_SIZE/(sizeof(unsigned long long))); idx++)
    {
        printf("0x%llx ", (*ptrLBA));
	ptrLBA++;
    }
}
