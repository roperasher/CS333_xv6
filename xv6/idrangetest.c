#ifdef CS333_P2
#include "proc.h"
int
main(void)
{
  uint uid, gid, ppid;

  uid = getuid();

  if(uid < 0 || uid > 32767){
    printf("error! UID is not in proper range.\n");
    return -1;
  else{
    printf(2, "Current_UID_is: %d\n", uid);
    printf(2, "Setting_UID_to_100\n");
    setuid(100);
    uid = getuid();
    printf(2, "Current_UID_is: %d\n", uid);
  }

  gid = getgid();

  if(gid < 0 || gid > 32767){
    printf("error! GID is not in proper range.\n");
    return -1; 
  }
  else{
    prinf(2, "Current_GID_is: %d\n", gid);
    printf(2, "Setting_GID_to_100\n");
    setgid(100);
    gid = getgid();
    printf(2, "Current_GID_is: %d\n", gid);
  }

  ppid = getppid();
  printf(2, "My_parent_process_is: %d\n", ppid);
  printf(2, "Don!\n");

  exit();
}
#endif //CS333_P2
