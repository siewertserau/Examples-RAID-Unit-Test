#ifndef RAIDLIB_H
#define RAIDLIB_H

#define OK (0)
#define ERROR (-1)
#define TRUE (1)
#define FALSE (0)

#define SECTOR_SIZE (512)

#define STRIP_SIZE (512)

void xorLBA(unsigned char *LBA1, unsigned char *LBA2, unsigned char *LBA3, unsigned char *LBA4, unsigned char *PLBA);
void rebuildLBA(unsigned char *LBA1, unsigned char *LBA2, unsigned char *LBA3, unsigned char *PLBA, unsigned char *RLBA);

void xorLBAArray(unsigned char LBA1[], unsigned char LBA2[], unsigned char LBA3[], unsigned char LBA4[], unsigned char PLBA[]);
void rebuildLBAArray(unsigned char LBA1[], unsigned char LBA2[], unsigned char LBA3[], unsigned char PLBA[],  unsigned char RLBA[]);

int checkEquivLBA(unsigned char *LBA1, unsigned char *LBA2);

int stripeFile(char *inputFileName, int offsetSectors);
int restoreFile(char *outputFileName, int offsetSectors, int fileLength);
#endif
