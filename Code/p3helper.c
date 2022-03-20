/* p3helper.c
   Program 3 CS480 SDSU Fall 2021
   Jesse Hernandez
   Prof. Carroll
   11/8/2021
   I worked on my own this is all my own code
   Credit to Tanenbaum for use of code from textbook 
   Also credit to the author of semex.c for things I took from semex.c

   This is the ONLY file you are allowed to change. (In fact, the other
   files should be symbolic links to
     ~cs480/Three/p3main.c
     ~cs480/Three/p3robot.c
     ~cs480/Three/p3.h
     ~cs480/Three/makefile
     ~cs480/Three/CHK.h    )
   */
#include "p3.h"
#include <math.h>
#define NUMROWS(n) round(sqrt((n)*2-trunc(sqrt((n)))))  // eqution for row amount
#define MAXVALS(n) ((n)*((n)+1))/2    // function for amount of values with amount of rows
/* You may put declarations/definitions here.
   In particular, you will probably want access to information
   about the job (for details see the assignment and the documentation
   in p3robot.c):
     */
extern int nrRobots;
extern int quota;
extern int seed;
sem_t *pmutx;
char semaphoreMutx[SEMNAMESIZE];
int minNumRows,vals,fdvals,fdRows;
double totalWidgets;
int finalVal = 0;
 // these global variables dont need to have the same value but jsut so that each call to functions they arent being
 //constantly created again so they can have different values for each process but i just need the names to call them

/* General documentation for the following functions is in p3.h
   Here you supply the code, and internal documentation:
   */
void initStudentStuff(void) {
  sprintf(semaphoreMutx,"%s%ldmutx",COURSEID,(long)getuid());
     /* semaphore guarding access to shared data */
     // the mutex is only created once by the first process to get here if the mutex is already created sem_failed is
     // return thanks to the o_EXCL flag
  if(SEM_FAILED == (pmutx = sem_open(semaphoreMutx,O_RDWR|O_CREAT|O_EXCL,S_IRUSR|S_IWUSR,1))){
        (pmutx = sem_open(semaphoreMutx,O_RDWR,S_IRUSR|S_IWUSR,1));
        // since it has failed then the process just needs to have access to use the mutex
  }
  else{
  // here I inizialize the first values to my count and row files(variables)
    int maxNums;
    CHK(sem_wait(pmutx)); /* request access to critical section so that no access is made before initialization*/
    totalWidgets = nrRobots*quota;
    minNumRows = NUMROWS(totalWidgets); //NUMROWS expects a double
    maxNums = MAXVALS(minNumRows);
    if(totalWidgets == maxNums){
        vals = minNumRows;
    }
    else{
        if(totalWidgets<maxNums){
            vals = minNumRows-(maxNums-totalWidgets);
        }
        else{
            minNumRows++;
            vals = totalWidgets-maxNums;
        }
    }
    // Creating count file 
    CHK(fdvals = open("countfile",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR));
    CHK(lseek(fdvals,0,SEEK_SET));
    assert(sizeof(vals) == write(fdvals,&vals,sizeof(vals)));
        
    // Creating row file
    CHK(fdRows = open("rowfile",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR));
    CHK(lseek(fdRows,0,SEEK_SET));
    assert(sizeof(minNumRows) == write(fdRows,&minNumRows,sizeof(minNumRows)));
    
    // Dont wnat to leave them just open after use they get closed since they are forced closed when the process ends 
    //anyway
    close(fdvals);
    close(fdRows);
    CHK(sem_post(pmutx)); /* release critical section */
        
  }
}

/* In this braindamaged version of placeWidget, the widget builders don't
   coordinate at all, and merely print a random pattern. You should replace
   this code with something that fully follows the p3 specification. */
void placeWidget(int n) {
  CHK(sem_wait(pmutx)); /* request access to critical section */
  CHK(fdvals = open("countfile",O_RDWR,S_IRUSR|S_IWUSR));
  CHK(fdRows = open("rowfile",O_RDWR,S_IRUSR|S_IWUSR));
  // the files are gaurenteed to be created already so only need to read and write 
  printeger(n);
  
  CHK(lseek(fdvals,0,SEEK_SET));
  CHK(lseek(fdRows,0,SEEK_SET));
  read(fdvals,&vals,sizeof(vals));//Every call to this function needs to update its value of the varibles 
  read(fdRows,&minNumRows,sizeof(minNumRows));
  
  vals--; // vals represent total values to print in the current row
  // so vals-- means we have printed a value
  if(vals==0){
      minNumRows--;
      vals = minNumRows;
      if(minNumRows==0){  // check for no more rows to print
          printf("F\n");
          finalVal = 1;// set flag to close the semaphore
      }
      else{// there are still more rows to print
          printf("N\n");
      }
  }
  
  CHK(lseek(fdvals,0,SEEK_SET));
  assert(sizeof(vals) == write(fdvals,&vals,sizeof(vals)));
  CHK(lseek(fdRows,0,SEEK_SET));
  assert(sizeof(minNumRows) == write(fdRows,&minNumRows,sizeof(minNumRows)));
  
  fflush(stdout);// to ensure proper printing all print statement hav already been made in this process
  close(fdvals);
  close(fdRows); // to keep things clean each time the function is used the files are close so they arent forced closed
  // by the ending of the process
  CHK(sem_post(pmutx)); /* release critical section */
  if(finalVal){ // flag encounterd time to end program
      CHK(sem_close(pmutx));
      CHK(sem_unlink(semaphoreMutx));
  }
}

/* If you feel the need to create any additional functions, please
   write them below here, with appropriate documentation:
   */
