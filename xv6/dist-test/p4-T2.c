#ifdef CS333_P4
// Credit to Jacob Collins <jmc27@pdx.edu> who developed this test
// program and gave me permission to use it

#include "types.h"
#include "user.h"
#include "param.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

void test1();
void test2();
void test3();
void test4();
void test5();
void test6();
void test7();
void test8();
void cleanupProcs(int pid[], int max);
void sleepMessage(int time, char message[]);
int createInfiniteProc();
int createSetPrioProc(int prio);

/* Note:
     You may need to change the names of MAXPRIO and BUDGET to match the
     names you have defined in your code.
*/
const int plevels = MAXPRIO;
const int budget = BUDGET;
const int promo = TICKS_TO_PROMOTE;

void
printMenu(void)
{
  int i = 0;

  printf(1, "\n");
  printf(1, "%d. exit program\n", i++);
  printf(1, "%d. Round robin scheduling for %d queues.\n", i++, plevels);
  printf(1, "%d. Promotion should change priority levels for sleeping and running processes.\n", i++);
  printf(1, "%d. Test demotion resetting budget.\n", i++);
  printf(1, "%d. Test scheduler operation.\n", i++);
  printf(1, "%d. Test demotion setting priority level.\n", i++);
  printf(1, "%d. Test scheduler selection, promotion, and demotion.\n", i++);
  printf(1, "%d. Test setpriority.\n", i++);
  printf(1, "%d, Test getpriority.\n", i++);
}

int
main(int argc, char* argv[])
{
  int select, done;
  char buf[5];
  //void (*test[])() = {test1, test2, test3, test4, test5, test6};

  printf(1, "p4test starting with: MAXPRIO = %d, DEFAULT_BUDGET = %d, TICKS_TO_PROMOTE = %d\n\n",
      plevels, budget, promo);

  while(1) {
    done = FALSE;
    printMenu();
    printf(1, "Enter test number:\n ");
    gets(buf, 5);

    if ((buf[0] == '\n') || buf[0] == '\0') {
      continue;
    }

    select = atoi(buf);

    switch(select) {
      case 0:
        done = TRUE;
        break;
      case 1:
        test1();
        break;
      case 2:
        test2();
        break;
      case 3:
        test3();
        break;
      case 4:
        test4();
        break;
      case 5:
        test5();
        break;
      case 6:
        test6();
        break;
      case 7:
        test7();
        break;
      case 8:
        test8();
        break;
      default:
        printf(1, "Error: invalid test number.\n");
    }

    if (done) {
      break;
    }
  }

  /* for(i = 1; i <= test_num; i++) {
       (*test[atoi(argv[i])-1])();
     }
   */

  exit();
}

void
test1()
{
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Test 1 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  int i;
  int max = 8;
  int pid[max];

  for(i = 0; i < max; i++)
    pid[i] = createInfiniteProc();

  for(i = 0; i < 3; i++)
    sleepMessage(2000, "Sleeping... ctrl-p\n");

  cleanupProcs(pid, max);
  printf(1, "+=+=+=+=+=+=+=+\n");
  printf(1, "| End: Test 1 |\n");
  printf(1, "+=+=+=+=+=+=+=+\n");
}

void
test2()
{
  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 2a |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  int i;
  int pid[2];
  pid[0]  = createInfiniteProc();

  setpriority(getpid(), plevels);

  i = 0;
  while(i <= plevels) {
    sleepMessage(2000, "Sleeping... ctrl-p\n");
    i++;
  }

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 2a |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 2b |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  pid[1] = createSetPrioProc(0);
  setpriority(pid[0], plevels);

  i = 0;
  while(i <= plevels) {
    sleepMessage(2000, "Sleeping... ctrl-p\n");
    i++;
  }

  cleanupProcs(pid, 2);

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 2b |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Test 2c |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=\n");

  int cPid = getpid();
  int rc = 0;

  printf(1, "Setting priority of the current process to %d... ", plevels);

  rc = setpriority(cPid, plevels);

  if (rc == -1) {
    printf(2, "setpriority to %s failed!\n\tThis test FAILED.\n", plevels);
    goto end;
  }

  rc = getpriority(cPid);

  if (rc != plevels) {
    printf(2, "getpriority failed! %d != %d\n\tThis test FAILED.\n", plevels, rc);
    goto end;
  }

  printf(1, "This test PASSES.\n");

end:
  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 2c |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
}

void
test3()
{
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Test 3 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+\n");

  int i;
  int max = 8;
  int pid[max];

  for(i = 0; i < max; i++)
    pid[i] = createInfiniteProc();

  for(i = 0; i <= plevels; i++)
    sleepMessage(2000, "Sleeping... ctrl-p OR ctrl-r\n");

  cleanupProcs(pid, max);

  printf(1, "+=+=+=+=+=+=+=+\n");
  printf(1, "| End: Test 3 |\n");
  printf(1, "+=+=+=+=+=+=+=+\n");

}

void
test4()
{
  if(plevels == 0) {
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
    printf(1, "| Start: Test 6a |\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 2) {
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
    printf(1, "| Start: Test 4a |\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 6) {
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
    printf(1, "| Start: Test 5a |\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
  }

  int i;
  int max = 10;
  int pid[max];

  for(i = 0; i < max/2; i++)
    pid[i] = createSetPrioProc(plevels);
  for(i = max/2; i < max; i++)
    pid[i] = createSetPrioProc(0);

  for(i = 0; i < 2; i++)
    sleepMessage(2000, "Sleeping... ctrl-p\n");

  cleanupProcs(pid, max);

  if(plevels == 0) {
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "| End: Test 6a |\n");
    printf(1, "+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 2) {
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "| End: Test 4a |\n");
    printf(1, "+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 6) {
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "| End: Test 5a |\n");
    printf(1, "+=+=+=+=+=+=+=+=\n");
  }
}

void
test6()
{
  if(plevels == 0) {
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
    printf(1, "| Start: Test 6b |\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 2) {
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
    printf(1, "| Start: Test 4b |\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 6) {
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
    printf(1, "| Start: Test 5b |\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
  }

  int i;
  int max = 8;
  int pid[max];

  for(i = 0; i < max/2; i++)
    pid[i] = createInfiniteProc();

  for(i = 0; i <= plevels; i++)
    sleepMessage(2000, "Sleeping... ctrl-p OR ctrl-r\n");
  if(plevels == 0)
    sleepMessage(2000, "Sleeping... ctrl-p OR ctrl-r\n");

  if(plevels == 0) {
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "| End: Test 6b |\n");
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
    printf(1, "| Start: Test 6c |\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 2) {
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "| End: Test 4b |\n");
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
    printf(1, "| Start: Test 4c |\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 6) {
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "| End: Test 5b |\n");
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
    printf(1, "| Start: Test 5c |\n");
    printf(1, "+=+=+=+=+=+=+=+=+=\n");
  }

  for(i = max/2; i < max; i++)
    pid[i] = createInfiniteProc();

  for(i = 0; i <= plevels+1; i++)
    sleepMessage(2000, "Sleeping... ctrl-p OR ctrl-r\n");

  cleanupProcs(pid, max);

  if(plevels == 0) {
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "| End: Test 6c |\n");
    printf(1, "+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 2) {
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "| End: Test 4c |\n");
    printf(1, "+=+=+=+=+=+=+=+=\n");
  }
  else if(plevels == 6) {
    printf(1, "+=+=+=+=+=+=+=+=\n");
    printf(1, "| End: Test 5c |\n");
    printf(1, "+=+=+=+=+=+=+=+=\n");
  }

}

void
test5()
{
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Test 5 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+\n");

  int i, prio;
  int max = 10;
  int pid[max];

  for(i = 0; i < max; i++) {
    prio = i%(plevels+1);
    pid[i] = createSetPrioProc(prio);
    printf(1, "Process %d will be at priority level %d\n", pid[i], prio);
  }

  sleepMessage(2000, "Sleeping... ctrl-p\n");
  sleepMessage(2000, "Sleeping... ctrl-r\n");

  cleanupProcs(pid, max);
  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 5 |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
}

//setpriority
void
test7()
{
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Test 7 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  
  //Write-up part 2b:
  //printf(1, "\n calling setpriority(2, 0). \n\n");
  //setpriority(2, 0);
  //printf(1, "\n calling setpriority(2, 6)\n");
  //setpriority(2, 6);
  //
  //Write-up part 5:
int i, rc, max = 8;
  int pid[max];

  printf(1, "----------- Start: Test 7 -----------\n");

  for(i = 0; i < max; i++)
    pid[i] = createInfiniteProc();
  printf(1, "%d processes created.\n", max);
  sleep(3000);

  printf(1, "\nSetting priority for all new process to priority 1\n");
  for(i = 0; i < max; i++) {
    rc = setpriority(pid[i], 1);
    if(rc < 0){
      printf(1, "  Test FAILED for process %d\n", pid[i]);
    }
  }
  rc = setpriority(getpid(), 1);
  printf(1, "Subtest 5 (a) &  5 (b) restult:" );
  if(rc < 0)
    printf(1, "\t\t\tTest FAILED\n");
  else
    printf(1, "\t\t\tTest PASSED\n");
  sleep(3000);




  printf(1, "\nSetting priority to the same priority level\n");
  for(i = 0; i < max; i++) {
    rc = getpriority(pid[i]);
    rc = setpriority(pid[i], rc);
    if(rc < 0){
      printf(1, "  Test FAILED for process %d\n", pid[i]);
    }
  }

  printf(1, "Subtest 5(c) restult:" );
  // if it should not promotoe   OR   it should promote and it does return an error
  if(plevels < 2  && rc == -1)
    printf(1, "\t\t\t\tTest FAILED  rc = %d\n", rc);
  else
    printf(1, "\t\t\t\tTest PASSED\n");
   sleep(3000);





  printf(1, "\nSetting priority 0 on an INVALID PID\n");
  rc = setpriority(-1, 0);
  if(rc != -1)
    printf(1, "  Test FAILED for process %d\n", pid[i]);
  else
    printf(1, "Failed to set priority for PID %d.\t\tTest PASSED\n", -1 );
  sleep(3000);






  printf(1, "\nSetting processes to an INVALID priority of -1\n");

  for(i = 0; i < max; i++) {
    rc = setpriority(pid[i], -1);
    if(rc != -1){
      printf(1, "  Test FAILED for process %d\n", pid[i]);
    }
  }

  printf(1, "Subtest 5(d) restult:" );
  if(rc != -1)
     printf(1, "\t\t\t\tTest FAILED\n");
  else
    printf(1, "\t\t\t\tTest PASSED\n");
  sleep(3000);

  cleanupProcs(pid, max);
	printf(1, "----------- End: Test 7    -----------\n\n");






  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 7 |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");
}

//getpriority
void test8()
{
  printf(1, "+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Test 8 |\n");
  printf(1, "+=+=+=+=+=+=+=+=+\n");

  printf(1, "Current process is p4-T2\n PID = 3\nCalling getpriority on current process\n\n");
  getpriority(3);

  printf(1, "\nCalling getpriority on process with PID of 2.\n\n");
  getpriority(2);

  printf(1, "\nCalling getpriority(9999), which does not exist. \n\n");
  getpriority(9999);
  

  printf(1, "+=+=+=+=+=+=+=+=\n");
  printf(1, "| End: Test 8 |\n");
  printf(1, "+=+=+=+=+=+=+=+=\n");

}

void
sleepMessage(int time, char message[])
{
  printf(1, message);
  sleep(time);
}

int
createInfiniteProc()
{
  int pid = fork();
  if(pid == 0)
    while(1);
  printf(1, "Process %d created...\n", pid);

  return pid;
}

int
createSetPrioProc(int prio)
{
  int pid = fork();
  if(pid == 0)
    while(1)
      setpriority(getpid(), prio);

  return pid;
}

void
cleanupProcs(int pid[], int max)
{
  int i;
  for(i = 0; i < max; i++)
    kill(pid[i]);
  wait();
}

#endif

