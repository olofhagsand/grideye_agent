Grideye tests 
=============
Read about what they do, how to build them and how to run them in 
the corresponding .c files.

cycles
++++++
This is a basic BogoMips test running a specified number of empty loops

compile:
   gcc -O2 -Wall -o cycles cycles.c cycles_test.s
run: 
   ./cycles

dhrystones
++++++++++
For every test:
   - run dhrystone test for a specificed number of dhrystones and measure 
     latency
compile:
   gcc -O2 -Wall -o dhrystones dhrystones.c dhry_1.c dhry_2.c
run: 
   ./dhrystones 30000

mem_read
++++++++
This is a basic memory read test: Create a large chunk of memory,
the read from that memory from random locations and measure latency

compile:
   gcc -O2 -Wall -o mem_read mem_read.c mem_read_test.c
run: 
   ./mem_read 

diskio_read
+++++++++++
Random I/O disk read
Precondition: a large file, typically 2*ram size
For every test:
   - Open the existing file.
   - Read 'ior' bytes from a random position using lseek(2)
   - Close the file
compile:
   gcc -O2 -Wall -o diskio_read diskio_read.c
run: 
   /bin/dd if=/dev/zero of=GRIDEYE_LARGEFILE bs=1M count=1K # optional
   ./diskio_read GRIDEYE_LARGEFILE 1024

diskio_write
++++++++++++
Sequential disk I/O write
Provide a filename
For every test:
 - Create and open the file
 - Write 'iow' bytes to the file sequentially.
 - Close and unlink the file.
compile:
   gcc -O2 -Wall -o diskio_write diskio_write.c
run: 
   diskio_write GRIDEYE_WRITEFILE 1024

diskio_write_rnd
++++++++++++++++
Random disk write
Precondition: a large file, typically 2*ram size
For every test:
  - Open the existing file.
  - Write 'iow' bytes to a random position using lseek to the file.
  - Close the file.
compile:
   gcc -O2 -Wall -o diskio_write_rnd diskio_write_rnd.c
run: 
   /bin/dd if=/dev/zero of=GRIDEYE_LARGEFILE bs=1M count=1K # optional
   diskio_write_rnd GRIDEYE_LARGEFILE 1024


