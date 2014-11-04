#!/bin/csh -f
#
# RAID Unit level regression script
#
echo "RAID UNIT REGRESSSION TEST"


echo "checkout and build test"
make clean
make >& testresults.log
echo "" >>& testresults.log

echo "simple RAID encode, erase and rebuild test"
raidtest 1000 >>& testresults.log
echo "" >>& testresults.log

echo "REBUILD DIFF CHECK for Chunk4 rebuild" >>& testresults.log
diff Chunk1.bin Chunk4_Rebuilt.bin >>& testresults.log
diff Chunk2.bin Chunk4_Rebuilt.bin >>& testresults.log
diff Chunk3.bin Chunk4_Rebuilt.bin >>& testresults.log
diff Chunk4.bin Chunk4_Rebuilt.bin >>& testresults.log
echo "" >>& testresults.log


echo "RAID performance test"
raid_perftest 1000 >>& testresults.log
