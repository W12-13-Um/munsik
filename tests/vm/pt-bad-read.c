/* Reads from a file into a bad address.
   The process must be terminated with -1 exit code. */
   //read 시스템 콜이 잘못된 주소로 데이터를 쓰려고 할때 커널이 이를 감지하고 프로세스를 종료시키는가

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  int handle;

  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  read (handle, (char *) &handle - 4096, 1);
  fail ("survived reading data into bad address");
}
