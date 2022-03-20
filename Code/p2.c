/*
Cs 480
Jesse Hernandez
11/28/21
Prof Carroll
*/


#include <sys/stat.h> 
#include <sys/syscall.h> 
#include <stdio.h> 
#include <errno.h> 
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h> 
#include <dirent.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <features.h> 
#include <limits.h> 
#include <unistd.h> 


#define SEMICOLEN ";"
#define ENDLINE "\n"
#define AMPERSAND "&"
#define BAR "|"
#define GREATER ">"
#define LESS "<"
#define MAXLENGTH 255
#define MAXWORDS 100
#define CDCOMMAND "cd"
#define LSCOMMAND "ls-F"
#define EXECCOMMAND "exec"
#define OVERWRITE   ">!"

//global variables
int programTermination = 0;
int saved_stdOUT, saved_stdIN;  // holds the original file desctriptors for stdin and stdout
int Background=0;  // global flag to denote a process needs to be backgrounded
int pipeLocation[20];  // with effective malloc calls I should be able to handle many more pipes
char outFile[50] = "\0";  // these two are strings hold the name of files for redirection
char inFile[50] = "\0";
int outChanged = 0;  // Falg for main fuction to see that stdout has been changed
int inChanged = 0;  // Flag to tell the main funtion stdin has been changed and needs to be set back to
                    // the original stdin at the end of an execution
int overwrite = 0;  //global flag for recognizing >! use
int numPipes = 0;  // crucial to handling pipes incremented for every pipe encounterd
int badQuotes = 0;

void sigHandler(int signum) {

}
int main(){
    char *newargv[MAXWORDS];
    int numWords,kidpid,command, OUT_status;
    int noFork =0;
    int exec_Status = 0;
    
    setpgid(0,0);
    (void)signal(SIGTERM, sigHandler);
    
    
    // calls to parce are made to fetch a new line until termiation flag is set
    for(;;){
    // initialization and reseting everything for each call
        printf(":480: ");
        inFile[0]='\0';
        outFile[0]='\0';
        numPipes = 0;
        overwrite = 0;
        memset(newargv,'\0',5*sizeof(newargv[0])); //need to ensure the buffer is empty after each call
        // begins the call to parse the input
        numWords = parse(newargv);
        newargv[numWords+1]=NULL;
        if(programTermination){
            break;
        }
        if(badQuotes==1){  // special case for bad input
          badQuotes =0;
          fprintf(stderr,"Missmatched quotes were encountered not a valid command please try again\n");
          continue;
        }
        // empty lines are ignored
        if(numWords==0){
            continue;
        }
        
        if(numWords==-1){
            fprintf(stderr,"The previous Line had ambiguous redirection\n");
            continue;
        }
        
        
        // If the string has some value then there is input redirection
        if(strlen(inFile)>0){  
            if(inRedirect()==-1){  
                
                perror("The File does not exist");
                continue;
            }
        }
        
        // more redirection this one is out
        if(strlen(outFile)>0){  
            if((OUT_status=outRedirect(overwrite))==-1){
                perror("The file already exists cant DESTROY!");
                continue;
            }
            else if(OUT_status==-2){  // error value for attempting to overwrite a directory
              fprintf(stderr,"%s cannot be overwritten since it is a Directory not a file\n",outFile);
              continue;
            }
        }
        
        
        
        // checks for possible piping that parse has flagged
        if(numPipes>0){
            pipeTime(newargv, numWords);
        // When hte main process comes back there may be redirection present so that needs to be reset
            if(outChanged==1){
                fflush(stdout);  // before anything is changed buffers need to be cleared
                outChanged=0;  // reset flag and file string
                outFile[0]='\0';
                // reset with the saved original;
                if(dup2(saved_stdOUT, STDOUT_FILENO)==-1){  
                    return -1;
                }
                close(saved_stdOUT);
            }
            // same thing is done with stdin
            if(inChanged==1){
                inChanged = 0;
                inFile[0] = '\0';
                if(dup2(saved_stdIN, STDIN_FILENO)==-1){
                    return -1;
                }
                close(saved_stdIN);
            }
            continue;
        }
        
        
        // moving forward with main I use my cammand function to check for use of builtIN functions
        // then make the appriiate call if needed
        command= isCommand(newargv[0]);
        if(command==1){
            cdCall(newargv[1],numWords);
            noFork=1;
            
        }
        else if(command == 2){
            lsCall(newargv,numWords);
            noFork=1;
        }
        else if(command ==3){
            execFunc(newargv);
            noFork=1;
        }
        if(noFork==1){
            noFork=0;
            continue;
        }
        
        
        // Since no built-in has been found a child will be forked to exec the first word of the line
        // first buffer need to be cleared
        fflush(stdout);
        if(noFork==0){
            
            if(-1 == (kidpid=fork())){
                
                perror("fork unsucessful");
                exit(EXIT_FAILURE);
            }
             if(Background==0){
                pid_t completed_child = 0;
                while (completed_child != kidpid) {
                  completed_child = wait(NULL);
                }
            }
            else if (kidpid!=0){
                Background =0; // resets the background flag after one &
                // if redirection has taken place then stdout is now different 
                // but it only gets reset after the execvp so I am using dprintf to 
                // use the saved_stdOUT for printing the backgrounding instance
                if(outChanged>0){
                    dprintf(saved_stdOUT,"%s ",newargv[0]);
                    dprintf(saved_stdOUT,"[%d]\n",kidpid);
                }
                else{  // no redirection
                    printf("%s ",newargv[0]);
                    fprintf(stdout,"[%d]\n",kidpid);
                }
                fflush(stdout);  // some print statements ran so just to make sure its all clear
            }
            if(kidpid==0){
                exec_Status = execvp(newargv[0], newargv);
                if (exec_Status==-1){
                    perror("execVp unsucessful");
                    exit(EXIT_FAILURE);
                }
            }
            
           
        }
        
        
        // Here after something has been exec'd there need be a sanity check for input/output
        if(outChanged==1){
            fflush(stdout);
            outChanged=0;
            outFile[0]='\0';
            if(dup2(saved_stdOUT, STDOUT_FILENO)==-1){
                return -1;
            }
            close(saved_stdOUT);
        }
         if(inChanged==1){
            inChanged = 0;
            inFile[0]='\0';
            if(dup2(saved_stdIN, STDIN_FILENO)==-1){
                return -1;
            }
            close(saved_stdIN);
        }
       
    }
    killpg(getpgrp(), SIGTERM);
    printf("p2 terminated.\n");
    fflush(stdout);
    fflush(stderr);

    exit(0);

}


/*
Parse is the function where lines of words are read and stored in the buffer
also needs to set important global flags such as program termination*/
int parse(char *chars[]){
    char storageBuffer[MAXWORDS][MAXLENGTH]; //temporarily holds the words
    int i,meta;
    int outCount = 0;
    int inCount =0;
    int wordCount = 0;
    int wordLen = 0;
    int lineIncomplete = 1;
    int pipesFound = 0;

  
    // this while loop makes it so that words ar added to a line 
    // until a line terminator is encountered.
    while (lineIncomplete){
        wordLen = getword(*(storageBuffer+wordCount));
        if(wordLen==99){
            lineIncomplete = 0; // this would ensure we could only take 99 words
            // then space 100 would hold the NULL pointer
        }
        
        if(wordLen == -20){
            badQuotes=1;
        }
        // meta is called to check for meta characters 
        meta = metaWord(storageBuffer[wordCount]);
        
        if(meta == 1){
            inCount++;
            // this condition ensure the program does not take ambiguous redirection
            if(inCount==2){
                return -1;
            }
            // current tly the last thing in the buffer is < so the next word is the file I need
            wordLen = getword(*(storageBuffer+wordCount));
            strcpy(inFile, storageBuffer[wordCount]);
            // copying it to my global string so I can store it for later use and then ccontinue so 
            // that it does not get added to the buffer
            continue;
        }
        // each pipe found gets its location appended to the pipeLocation array
        else if(meta == 2){
                numPipes++;
                pipeLocation[pipesFound] = wordCount;
                pipesFound++;
        }
        // same procedure as input
        else if(meta == 3 ){
            outCount++;
            if(outCount==2){
                return -1;
            }
            wordLen =getword(*(storageBuffer+wordCount));
            strcpy(outFile, storageBuffer[wordCount]);
            continue;
        }
        
        if(wordLen==-2){
            Background = 1;
            lineIncomplete=0;
        }
        // any words of length 0 or less should end the line
        // since those are line terminators
        if (wordLen < 1){
                lineIncomplete =0;
            if(wordLen==-1){
            // specifically looking for the EOF condition
                programTermination = 1;
                lineIncomplete = 0;
            }
        }
        wordCount++;
    }
    wordCount--; // this part is important or else \n will be in the array
    // once fully parced the wanted input will be sent to the proper pointer array
    for(i=0; i<wordCount;i++){
        chars[i]=storageBuffer[i];
    }
    return wordCount; // parce always returns the number of words in a line

} 

// after parse has completed and the output flag is set then this function carries out the redirection
int outRedirect(int destroy){
    int tempFD;
    
    fflush(stdout);
    outChanged = 1;
    saved_stdOUT = dup(STDOUT_FILENO); // need to save to reset stdout later
    if(destroy == 0){
        tempFD= open(outFile, O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR);
    }
    // this one overwrites files  the first only make new files
    else{
        tempFD = open(outFile, O_RDWR| O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
        if(tempFD==-1){
        return -2;
    }
    }
    if(tempFD==-1){
        return -1;
    }
    if(dup2(tempFD,STDOUT_FILENO)==-1){
        return -1;
        
    }
    close(tempFD);
    return 0;
}


// after parse has completed and the input flag is set then this function carries out the redirection
int inRedirect(void){
    int tempFD;
    
    inChanged = 1;
    saved_stdIN = dup(STDIN_FILENO);
    tempFD = open(inFile, O_RDONLY, S_IWUSR | S_IRUSR );
    if(tempFD==-1){
        return -1;
    }
    if(dup2(tempFD,STDIN_FILENO)==-1){
        return -1;
    }
    close(tempFD);
    return 0;
}


int pipeTime(char *toPipe[],int nums){
    int kidpids[numPipes+1]; // I need to create numPipes+1 children to make this work
    int exec_Status;
    int fildes[numPipes][2];  // the first pipe created will be the last on in the input
    int numForked = 0;  // represent amount of forks used except for the first one
    int piped = 0;  // not neccesary
    int i,x;  // counter varibles for looping
    int currPipe = numPipes-1;
    int builtIN = 0;  // flag for last child to execute a builtin function and not execvp
    pid_t completed_child = 0;
    
    // this for loop makes all pipe values into null  values to be able to exec one command at a time
    for(x=0;x<numPipes;x++){
      toPipe[pipeLocation[x]] = NULL;
    }
    
   
    fflush(stdout);  // flush buffer before fork
    // the main process creates the first child
    if((kidpids[numForked]=fork())==-1){
        perror("fork unsucessful");
        exit(EXIT_FAILURE);
    }
    // this next condition only valid for the final child that will execute the first comman in the line
    else if(kidpids[numForked]==0){  // the first child creates the first pipe then the second chilf
        if(pipe(fildes[0])==-1){
            perror("pipe unsuccessful");
            exit(EXIT_FAILURE);
        }
        numForked++;
        if((kidpids[numForked]=fork())==-1){
            perror("fork unsucessful");
            exit(EXIT_FAILURE);
        }
        else if(kidpids[numForked]==0){  // second child 
            
            
            for (i = 1; i < numPipes; i++) { // no more children are created if we already have numPipes
                // Middle Children
                currPipe--;  // the current pipe since the pipes are being created from last to first
                numForked++;  // index for child processes
                if (pipe(fildes[i]) < 0) {
                    perror("\nPipe could not be initialized");
                    return;
                }

                // Fork Child 3
                fflush(stdout);
                fflush(stderr);
                kidpids[numForked] = fork();
                if (kidpids[numForked] < 0) {
                    printf("\nCould not fork");
                    return;
                }

                // Loop will continue for forked child, which will increment i and continue on
                if (kidpids[numForked] != 0) {

                  dup2(fildes[i][0], STDIN_FILENO);  // read end of pipe it created
                  dup2(fildes[i - 1][1], STDOUT_FILENO); // write end of pipe creted by parent
                  
                  // after dup 2 all open fds need to be closed 
                  // every process only knows abou the pipe it created and the previous one
                  close(fildes[i - 1][0]);
                  close(fildes[i][0]);
                  close(fildes[i - 1][1]);
                  close(fildes[i][1]);
                    
                  exec_Status=execvp(toPipe[pipeLocation[currPipe]+1],(toPipe+pipeLocation[currPipe]+1));
                    if (exec_Status==-1){
                        perror("execVp unsucessful");
                        exit(EXIT_FAILURE);
                    }
                }

            }
            // final child which executes the first command in the given line
            // the only child process that exits the loop withou becoming a parent
            if(kidpids[numPipes]==0){
                if(dup2(fildes[numPipes-1][1],STDOUT_FILENO)==-1){
                    perror("DUP2 unsucessful");
                    exit(EXIT_FAILURE);
                }
                close(fildes[numPipes-1][0]);
                close(fildes[numPipes-1][1]);
                
                if(strcmp(toPipe[0],LSCOMMAND)==0){
                    builtIN = 1;
                    lsCall(toPipe, pipeLocation[0]);
                    exit(1);
                }
                else if(strcmp(toPipe[0],EXECCOMMAND)==0){
                    builtIN = 1;
                    execFunc(toPipe);
                    exit(1);
                }
                if(builtIN == 0){
                    exec_Status = execvp(toPipe[0], toPipe);
                    if (exec_Status==-1){
                        perror("execVp unsucessful");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            
        }
        
        // back to the first child process that will handle the very last command
        if(dup2(fildes[0][0],STDIN_FILENO)==-1){
            perror("DUP2 unsucessful");
            exit(EXIT_FAILURE);
        }
        close(fildes[piped][1]);
        close(fildes[piped][0]);
        
        toPipe[pipeLocation[numPipes-1]] = NULL;
        
        exec_Status = execvp(toPipe[pipeLocation[numPipes-1]+1], (toPipe+pipeLocation[numPipes-1]+1));
        if (exec_Status==-1){
            perror("execVp unsucessful");
            exit(EXIT_FAILURE);
        }
    }
    // main waiting for the first child to finish
    
        while (completed_child != kidpids[0]) {
            completed_child = wait(NULL);
        }
    return 0;
} 


// meanwhile in parse this would detect metachars to do something special
int metaWord(char *string){
    if(strcmp(string,BAR)==0){  // each one has a specifc return val
        return 2;
    }
    else if(strcmp(string,GREATER)==0){
        return 3;
    }
    else if(strcmp(string,LESS)==0){
        return 1;
    }
    else if(strcmp(string,OVERWRITE)==0){
        overwrite=1;
        return 3;
    }
    else{
        return 0;
    }
}


// whenever "cd" is the first word in a line main calls this it relies on words 
//  make sure there are not too many arguments
// no return value but I dont want to chage this in fear of breaking
int cdCall(char *string, int words){
    int directory_exist;
    int chDirSuccess;
    
    if(words==2){
    // 2 words means the cd command and the argument
        if((directory_exist=access(string, F_OK))==-1){
            perror("The directory does not exist");
            return -1;
            // fail condition is neccessary to make sure the new directory exists
        }
        else{
            chDirSuccess = chdir(string);
        }
    }
    else if(words ==1){
    // here ther is only the cd command with no arguments
        chdir(getenv("HOME")); //Time to go back home;
        
    }
    else{
    // all that is left is a situation where there are more than 2 arguments
        fprintf(stderr,"Too many argumetns given cd only takes one argument\n");
        return -2;
    }
}

// helper function for lsCall the actual printing of entries happens here
int directoryPrint(char *toOpen, int directoryFlag, int otherDIR){
    DIR *dirp;
        struct dirent *dp;
        struct stat buff;
        char filePath[50];
        int statERROR;
        char currFilePath[50];
        
        printf("\n");
        strcpy(filePath, toOpen);
        strcat(filePath, "/"); // if we are given filePath then I need to be able to manipulate it
        
        if(directoryFlag ==0 ){ // the flag represents whether the given string is a directory or not
            dirp = opendir("."); 
            // an ls call with a file just checks for its existence in the current directory
        }
        else{
            dirp = opendir(toOpen);
        }
        if( dirp == NULL ){
            fprintf(stderr, "Could not open the directory: %s does not exist\n", toOpen);
            return -1;
        }
        // here the directly was succefully opend and will now be looked thorugh 
        else{
            int count = 0;
            while(dirp){
                if((dp = readdir(dirp)) != NULL){
                
                // the given string was a directory so all its contents need to be printed
                    if( directoryFlag ==1){
                        if(otherDIR == 1 && count>1){  // if the directory is not "."
                            
                            strcpy(currFilePath, filePath);
                            strcat(currFilePath, dp->d_name);
                            statERROR = lstat(currFilePath, &buff);
                            // currFilePath hold the full path to lookup each file
                        }
                        else{
                            statERROR = lstat(dp->d_name, &buff);  // need lstat to get info on links
                        }
                         if(statERROR==-1){
                             // errors dont affect the flow
                         }
                         else{
                             if (S_ISLNK(buff.st_mode)) {  // test case for links
                                 if (otherDIR==0){  // since we are lookin at "." 
                                    switch(stat(dp->d_name,&buff)){
                                      case 0: printf("%s@\n", dp->d_name);        break;
                                      default: printf("%s&\n", dp->d_name);
                                    }
                                 }
                                 else{
                                     switch(stat(currFilePath,&buff)){
                                      case 0: printf("%s@\n", dp->d_name);        break;
                                      default: printf("%s&\n", dp->d_name);
                                    }
                                 }
                                 continue; // file found continue to next
                             }
                         
                             // test case for files
                             if (S_ISREG(buff.st_mode)) {
                                 if(buff.st_mode & S_IXUSR){
                                     printf("%s*\n",dp->d_name);
                                 }
                                 else if(buff.st_mode & S_IXGRP){
                                     printf("%s*\n",dp->d_name);
                                 }
                                 else if(buff.st_mode & S_IXOTH){
                                     printf("%s*\n",dp->d_name);
                                 }
                                 else{
                                     printf("%s\n",dp->d_name);
                                 }
                                 continue;  // file found continue to next
                             }
                             
                             // final test case for directories
                             switch (buff.st_mode & S_IFMT){
                               case S_IFDIR: printf("%s/\n",dp->d_name);        break;
                             }
                             
                             count++;
                         }
                    }// end of directory iteration
                    
                    // just looking for existence of the single file
                    else{
                        if(strcmp(dp->d_name, toOpen)==0){
                            printf("%s\n", toOpen);
                        }
                    }
                }
                else{
                    closedir(dirp);
                    break;
                }
            }
            return 0;
        }
}

//unfinished
int lsCall(char *strings[], int words){
    int n;  // value that is 1 for other directories and 0 for "." 
    
    if( words==1 ){  // no additional input -iterate through current directory
        if((directoryPrint(".",1,0))==-1){
            exit(EXIT_FAILURE);
        }
        printf("\n");
    }
    // there is additional input to deal with
    else {
        int i;
        struct stat sb;
        // iterate through all input 
        for (i = 1;i<words;i++){
            n= 1; // assumed to be some other directory
            // first check if file is valid
            if(stat(strings[i], &sb)==-1){
                fprintf(stderr,"stat: %s is not a valid file in the current directory\n", strings[i]);
                continue;
            }
            // check for current directory value
            if (strcmp(strings[i],".")==0){
              n=0;  
            }
            // handle the two possibilites: directory and single file
            switch (sb.st_mode & S_IFMT) {
              case S_IFDIR: directoryPrint(strings[i],1,n) ;      break;
              case S_IFREG: directoryPrint(strings[i],0,0);     break;
            }
            fflush(stdout);
            
        }
    }
}

// simple helper to check a single word against the three builtin comand names
int isCommand(char *string){
    if(strcmp(string,CDCOMMAND)==0){
        return 1;
    }
    else if(strcmp(string,LSCOMMAND)==0){
        return 2;
    }
    else if(strcmp(string,EXECCOMMAND)==0){
        return 3;
    }
    else{
        return 0;
    }
}
// does not work
int execFunc(char *strings[]){
    int execStatus;
    fflush(stdout);
    fflush(stderr);
    execStatus = execvp(strings[1], strings);
    if (execStatus==-1){
        perror("execCall Failed");
    }
}

