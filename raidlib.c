#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>

// added for read and write prototype system calls
// to address compiler warning
#include <unistd.h>

#include "raidlib.h"

#ifdef RAID64
#include "raidlib64.h"
#define PTR_CAST (unsigned long long *)
#else
#include "raidlib.h"
#define PTR_CAST (unsigned char *)
#endif


// RAID-5 4+1 encoding
//
// This is 80% capacity with 1/5 LBAs used for parity.
//
// Only handles single faults.
//
// PRECONDITIONS:
// 1) LBA pointeres must have memory allocated for them externally
// 2) LBAs are initialized with data
//
// POST-CONDITIONS:
// 1) Contents of PLBA is modified and contains the computed parity using XOR
//
void xorLBA(unsigned char *LBA1,
	    unsigned char *LBA2,
	    unsigned char *LBA3,
	    unsigned char *LBA4,
	    unsigned char *PLBA)
{
    int idx;

    for(idx=0; idx<SECTOR_SIZE; idx++)
        *(PLBA+idx)=(*(LBA1+idx))^(*(LBA2+idx))^(*(LBA3+idx))^(*(LBA4+idx));
}


// More readable version of xorLBA which uses array derefernces compared to pointer
// arithmetic in original version.
//
void xorLBAArray(unsigned char LBA1[],
	         unsigned char LBA2[],
	         unsigned char LBA3[],
	         unsigned char LBA4[],
	         unsigned char PLBA[])
{
    int index;

    // XOR all bits in all bytes of each of the 4 data blocks @ logical block address to form
    // the parity block.
    //
    for(index=0; index<SECTOR_SIZE; index++)
    {
        PLBA[index] = LBA1[index] ^ LBA2[index] ^ LBA3[index] ^ LBA4[index];
    }
}



// RAID-5 4+1 Rebuild
//
// Provide any 3 of the original LBAs and the Parity LBA to rebuild the RLBA
//
// If the Parity LBA was lost, then it can be rebuilt by simply re-encoding.
// 
void rebuildLBA(unsigned char *LBA1,
	        unsigned char *LBA2,
	        unsigned char *LBA3,
	        unsigned char *PLBA,
	        unsigned char *RLBA)
{
    int idx;
    unsigned char checkParity;

    for(idx=0; idx<SECTOR_SIZE; idx++)
    {
        // Parity check word is simply XOR of remaining good LBAs
        checkParity=(*(LBA1+idx))^(*(LBA2+idx))^(*(LBA3+idx));

        // Rebuilt LBA is simply XOR of original parity and parity check word
        // which will preserve original parity computed over the 4 LBAs
        *(RLBA+idx) =(*(PLBA+idx))^(checkParity);
    }
}


// More readable version of xorLBA which uses array derefernces compared to pointer
// arithmetic in original version.
//
void rebuildLBAArray(unsigned char LBA1[],
	             unsigned char LBA2[],
	             unsigned char LBA3[],
	             unsigned char PLBA[],
	             unsigned char RLBA[])
{
    int index;
    unsigned char checkParity;

    for(index=0; index<SECTOR_SIZE; index++)
    {
        // Parity check word is simply XOR of remaining good LBAs
        //
        checkParity=LBA1[index] ^ LBA2[index] ^ LBA3[index];

        // Rebuilt LBA is simply XOR of original parity and parity check word
        // which will preserve original parity computed over the 4 LBAs
        //
        RLBA[index] = PLBA[index] ^ checkParity;
    }
}



int checkEquivLBA(unsigned char *LBA1,
		  unsigned char *LBA2)
{
    int idx;

    for(idx=0; idx<SECTOR_SIZE; idx++)
    {
        if((*(LBA1+idx)) != (*(LBA2+idx)))
	{
            printf("EQUIV CHECK MISMATCH @ byte %d: LBA1=0x%x, LBA2=0x%x\n", idx, (*LBA1+idx), (*LBA2+idx));
	    return ERROR;
	}
    }

    return OK;
}


// returns bytes written or ERROR code
// 
int stripeFile(char *inputFileName, int offsetSectors)
{
    int fd[5], idx;
    FILE *fdin;
    unsigned char stripe[5*STRIP_SIZE];
    int offset=0, bread=0, btoread=(4*STRIP_SIZE), bwritten=0, btowrite=(STRIP_SIZE), sectorCnt=0, byteCnt=0;

    fdin = fopen(inputFileName, "r");
    fd[0] = open("StripeChunk1.bin", O_RDWR | O_CREAT, 00644);
    fd[1] = open("StripeChunk2.bin", O_RDWR | O_CREAT, 00644);
    fd[2] = open("StripeChunk3.bin", O_RDWR | O_CREAT, 00644);
    fd[3] = open("StripeChunk4.bin", O_RDWR | O_CREAT, 00644);
    fd[4] = open("StripeChunkXOR.bin", O_RDWR | O_CREAT, 00644);


    do
    {

        // read a stripe or to end of file
        offset=0, bread=0, btoread=(4*STRIP_SIZE);
        do
        {
            bread=fread(&stripe[offset], 1, btoread, fdin); 
            offset+=bread;
            btoread=(4*STRIP_SIZE)-bread;
        }
        while (!(feof(fdin)) && (btoread > 0));


        if((offset < (4*STRIP_SIZE)) && (feof(fdin)))
        {
            printf("hit end of file\n");
            bzero(&stripe[offset], btoread);
            byteCnt+=offset;
        }
        else
        {
            printf("read full stripe\n");
            assert(offset == (4*STRIP_SIZE));
            byteCnt+=(4*STRIP_SIZE);
        };

        // computer xor code for stripe
        //
        xorLBA(PTR_CAST &stripe[0],
               PTR_CAST &stripe[STRIP_SIZE],
               PTR_CAST &stripe[(2*STRIP_SIZE)],
               PTR_CAST &stripe[(3*STRIP_SIZE)],
               PTR_CAST &stripe[(4*STRIP_SIZE)]);


        // write out the stripe + xor code
        //
        offset=0, bwritten=0, btowrite=(STRIP_SIZE);
        do
        {
            bwritten=write(fd[0], &stripe[offset], STRIP_SIZE); 
            offset+=bwritten;
            btowrite=(STRIP_SIZE)-bwritten;
        }
        while (btowrite > 0);

        offset=STRIP_SIZE, bwritten=0, btowrite=(STRIP_SIZE);
        do
        {
            bwritten=write(fd[1], &stripe[offset], STRIP_SIZE); 
            offset+=bwritten;
            btowrite=(STRIP_SIZE)-bwritten;
        }
        while (btowrite > 0);

        offset=(2*STRIP_SIZE), bwritten=0, btowrite=(STRIP_SIZE);
        do
        {
            bwritten=write(fd[2], &stripe[offset], STRIP_SIZE); 
            offset+=bwritten;
            btowrite=(STRIP_SIZE)-bwritten;
        }
        while (btowrite > 0);

        offset=(3*STRIP_SIZE), bwritten=0, btowrite=(STRIP_SIZE);
        do
        {
            bwritten=write(fd[3], &stripe[offset], STRIP_SIZE); 
            offset+=bwritten;
            btowrite=(STRIP_SIZE)-bwritten;
        }
        while (btowrite > 0);

        offset=(4*STRIP_SIZE), bwritten=0, btowrite=(STRIP_SIZE);
        do
        {
            bwritten=write(fd[4], &stripe[offset], STRIP_SIZE); 
            offset+=bwritten;
            btowrite=(STRIP_SIZE)-bwritten;
        }
        while (btowrite > 0);

        sectorCnt+=4;

    }
    while (!(feof(fdin)));

    fclose(fdin);
    for(idx=0; idx < 5; idx++) close(fd[idx]);

    return(byteCnt);
}


// returns bytes read or ERROR code
// 
int restoreFile(char *outputFileName, int offsetSectors, int fileLength)
{
    int fd[5], idx;
    FILE *fdout;
    unsigned char stripe[5*STRIP_SIZE];
    int offset=0, bread=0, btoread=(4*STRIP_SIZE), bwritten=0, btowrite=(STRIP_SIZE), sectorCnt=fileLength/STRIP_SIZE;
    int stripeCnt=fileLength/(4*STRIP_SIZE);
    int lastStripeBytes = fileLength % (4*STRIP_SIZE);

    fdout = fopen(outputFileName, "w");
    fd[0] = open("StripeChunk1.bin", O_RDWR | O_CREAT, 00644);
    fd[1] = open("StripeChunk2.bin", O_RDWR | O_CREAT, 00644);
    fd[2] = open("StripeChunk3.bin", O_RDWR | O_CREAT, 00644);
    fd[3] = open("StripeChunk4.bin", O_RDWR | O_CREAT, 00644);
    fd[4] = open("StripeChunkXOR.bin", O_RDWR | O_CREAT, 00644);


    for(idx=0; idx < stripeCnt; idx++)
    {
        // read in the stripe + xor code
        //
        offset=0, bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[0], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        offset=STRIP_SIZE, bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[1], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        offset=(2*STRIP_SIZE), bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[2], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        offset=(3*STRIP_SIZE), bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[3], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        offset=(4*STRIP_SIZE), bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[4], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        // OPTION - check XOR here
        //

       // write a full stripe
        offset=0, bwritten=0, btowrite=(4*STRIP_SIZE);
        do
        {
            bwritten=fwrite(&stripe[offset], 1, btowrite, fdout); 
            offset+=bwritten;
            btowrite=(4*STRIP_SIZE)-bwritten;
        }
        while ((btowrite > 0));

    }


    if(lastStripeBytes)
    {
        // read in the parital stripe + xor code
        //
        offset=0, bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[0], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        offset=STRIP_SIZE, bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[1], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        offset=(2*STRIP_SIZE), bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[2], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        offset=(3*STRIP_SIZE), bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[3], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        offset=(4*STRIP_SIZE), bread=0, btoread=(STRIP_SIZE);
        do
        {
            bread=read(fd[4], &stripe[offset], STRIP_SIZE); 
            offset+=bread;
            btoread=(STRIP_SIZE)-bread;
        }
        while (btoread > 0);

        // OPTION - check XOR here
        //

       // write a partial stripe
        offset=0, bwritten=0, btowrite=(lastStripeBytes);
        do
        {
            bwritten=fwrite(&stripe[offset], 1, btowrite, fdout); 
            offset+=bwritten;
            btowrite=lastStripeBytes-bwritten;
        }
        while ((btowrite > 0));
    }


    fclose(fdout);
    for(idx=0; idx < 5; idx++) close(fd[idx]);

    return(fileLength);
}

