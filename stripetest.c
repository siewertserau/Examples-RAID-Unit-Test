#include <stdio.h>
#include <stdlib.h>

#include "raidlib.h"

int main(int argc, char *argv[])
{
    int bytesWritten, bytesRestored;

    if(argc < 3)
    {
        printf("Usage: stripetest <inputfile> <outputfile>\n");
        exit(-1);
    }

    if((bytesWritten=stripeFile(argv[1], 0) > 0))
    {
        printf("CHUNKED with bytesWritten=%d\n", bytesWritten);
    }
    else
    {
        printf("fatal error while striping\n"); exit(-1);
    }
        
    

    if((bytesRestored=restoreFile(argv[2], 0, bytesWritten) > 0))
    {
        printf("FINISHED with bytesRestored=%d\n", bytesRestored);
    }
    else
    {
        printf("fatal error while restoring\n"); exit(-1);
    }
    
  
    return(1);      
}
