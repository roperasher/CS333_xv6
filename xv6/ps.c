#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"

int main(int argc, char * argv[]){
  int max = atoi(argv[1]);
  struct uproc * utable;
  utable = (struct uproc*) malloc(sizeof(struct uproc) * max);

  int active_proc_count = getprocs(max, utable);
  
  #ifdef CS333_P4
  printf(1, "pid\tname\tuid\tgid\tppid\tPrio\telapsed\tCPU\tstate\tsize\n");
  #else
  printf(1, "pid\tname\tuid\tgid\tppid\telapsed\tCPU\tstate\tsize\n");
  #endif //CS333_P4

  for(int i = 0; i < active_proc_count; i++){
    printf(1, "%d\t%s\t%d\t%d\t%d", utable->pid, utable->name, utable->uid, utable->gid, utable->ppid);

    #ifdef CS333_P4
    printf(1, "\t%d", utable->priority);
    #endif //CS333_P4  

    printf(1,"\t%d", utable->elapsed_ticks/1000);
    if(utable->elapsed_ticks%1000 < 10)
      printf(1, ".00%d\t", utable->elapsed_ticks%1000);
    if(utable->elapsed_ticks%1000 < 100 && utable->elapsed_ticks%1000 < 10)
      printf(1, ".0%d\t", utable->elapsed_ticks%1000);
    else
      printf(1, ".%d\t", utable->elapsed_ticks%1000);
  
    printf(1, "%d", utable->CPU_total_ticks/1000);
    if(utable->CPU_total_ticks%1000 < 10)
      printf(1, ".00%d\t", utable->CPU_total_ticks%1000);
    if(utable->CPU_total_ticks%1000 < 100 && utable->CPU_total_ticks%1000 < 10)
      printf(1, ".0%d\t", utable->CPU_total_ticks%1000);
    else
      printf(1, ".%d\t", utable->CPU_total_ticks%1000);

    printf(1, "%s\t\t%d\n", utable->state, utable->size);


    utable++;
  }
  free(utable);
  exit();
}
#endif //CS333_P2
