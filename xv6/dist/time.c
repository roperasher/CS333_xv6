#ifdef CS333_P2
#include "types.h"
#include "user.h"

int main(int argc, char * argv[])
{
  int current_ticks = uptime();
  int pid = fork();

  if(pid == 0) //Child
  {
      exec(argv[1], &argv[1]);
  }
  else if(pid > 0) //Parent
  {
    wait(); //waiting for child process to terminateA
    current_ticks = uptime() - current_ticks; //ticks now = time of the process running.
    if(current_ticks%1000 < 10)
      printf(1, "%s ran in, %d.00%d seconds.\n", argv[1], current_ticks/1000, current_ticks%1000);

    else if(current_ticks%1000 < 100 && current_ticks%1000 > 10) 
      printf(1, "%s ran in, %d.0%d seconds.\n", argv[1], current_ticks/1000, current_ticks%1000);
    else
      printf(1, "%s ran in, %d.%d seconds.\n", argv[1], current_ticks/1000, current_ticks%1000);
  }
  else
    printf(1, "FORK FAILED. ERROR!\n");
  exit();
}
#endif
