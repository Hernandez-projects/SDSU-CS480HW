/*
Cs 480
Jesse Hernandez
9/12/21
Prof Carroll
Credit to the original author of the inout2.c, W.D Foster for partial use of the code.
*/

#include <stdio.h>
#include <string.h>
#define backslash 92
#define quote 39
// backslash and quote are used across diffent funtions these are the ascii values for it

/* isClosed is a function created to test whether a single quote is closed as in has its pair 
or it does not have a terminating quote. when closed this will return the number of chars
 within the quotes and this returns 0 when not closed
*/
int isClosed(void){ 
    char inChars[15];
    int tempInput = 0;
    int vals = 0;  // this is going to hold the number of chars whithin the quotes
    int hold;    // inChars is going to hold the chars to be able to ungetc it all back
    int found = 0; // hold and tempInput are temporary variables to hold input or vals
    
    while( (tempInput = getchar() ) != EOF && vals != 14){
      if(tempInput == backslash){ // compares input with ascii backslash
        tempInput = getchar();
        if(tempInput == quote){  // this is for the special case \' to add that all to the array
          inChars[vals] = tempInput; 
          vals++;  
          continue;  // once the special case is confirmed and added I force the next iteration
        }
        else{
          inChars[vals] = backslash;  // when there is no special case this else handles that
          vals++; // backslash is added to the array and the loop continues like normal
        }
      }
      if(tempInput != quote){  // anything that is not a singe quote is added to the array
        inChars[vals] = tempInput;
        vals++;  // each time something is added ot inChars vals is incremented
      }
      else{
        found = 1; // this bit is imprtant for when the quotes are actually closed
        break;  // when it is closed we flag found and break out to be able to return vals
      }
    }
    hold = vals-1; // hold is neccesary so that vals is untouched for return
    while(hold != -1){ // chars taken out of the input stream are returned to the stream
      ungetc(inChars[hold], stdin); // ungetc returns a char to desrired stream
      hold--;
    }
    if( found == 1){
      return vals;
    }
    else{
      return 0; 
    }
}

/* isMetaChar is a function I created to test chars for being a meta character there are 6 possible return values. 0 is returned when the char is not a meta char, 1 is returned when the char is metaChar <, |, and &. Then 2 is return when it is a single > char, 3 is returned when the pair >! is found, next 4 is returned when there is a beggining quote found, lastly 5 is return when a ; is encountered these values are used appropriatly inside the get word function
*/
int isMetaChar(int c){
    int greater = 62;
    int less = 60;
    int bar = 124;
    int semicolon = 59;
    int ampersand = 38;
    int exclamation = 33; // all these are the ascii values of the metaChars
    int tempInput = 0;
    
    if( c == less || c == bar || c == ampersand ){ // test case for return 1 condition
        return 1;
    }
    else if( c == greater){
        tempInput = getchar();
        if( tempInput == exclamation ){
            ungetc(tempInput, stdin);  // test case for return 3 condition
            return 3;
        }
        else{
            ungetc(tempInput, stdin); // when > is found next chars is read to test ! then ungetc
            return 2;
        }
    }
    else if(c == quote){  // test case for return 4 condition
        return 4;
    }
     else if(c == semicolon){  // test case for return 5 condition
        return 5;
    }
    else{
        return 0;  // if this part is reached not a metaChar
    }
}

// main task function the other two were helper functions
int getword(char *w){
    int inputchar;
    int numchars=0;  // each time a character is added to the array this increases
    int spaceChar = 32;// these are all constants that will be used for comparison
    int newLine = 10;
    *(w+numchars) = '\0';// after each call the array needs to be empty
    int word = 0;
    int metaVals = 0;
    int numValsinQ = 0;
    int limit = 254;
    
    /* This while loop starts reading keyboard input until EOF is reached*/ 
    while( (inputchar = getchar() ) != EOF ) {
    
    if( numchars == limit){ //limit is set to 254 once this num is reached a word reset is forced
      ungetc(inputchar, stdin);
      return numchars;
    }
    
    metaVals = isMetaChar(inputchar);  // makes the call to helper to get metaVal
    if( metaVals == 1 || metaVals == 2){
        if( *w != '\0'){  // meta case 1 and 2 end the word then are treated as their own word
          ungetc(inputchar, stdin);
          return numchars;
        }
        else {
          *(w+numchars) = inputchar; // if no word before then just return single char word
          *(w+numchars+1)= '\0';
          numchars++;
          return numchars;
        }
    }
    else if( metaVals == 3){
        if( *w != '\0'){
          ungetc(inputchar, stdin);
          return numchars;
        }
        else {
          *(w+numchars) = inputchar;
          numchars++;
          inputchar = getchar();  // meta case 3 is similar to 1 and 2 but has two chars to use
          *(w+numchars) = inputchar;
          *(w+numchars+1)= '\0';
          numchars++;
          return numchars;
        }
    }
    else if( metaVals == 4){  // complex case 4 makes use of a helper function 
        numValsinQ = isClosed();
        if( numValsinQ > 0){  
          inputchar = getchar();
          // this while loop adds the chars in the quotes to the current word 
          while(numValsinQ != 0){
            *(w+numchars) = inputchar;
            numchars++;
            inputchar = getchar();
            numValsinQ--;
          }
        }
        else {
          continue;  // continue happens when the quote is not closed so it is ignored
        }
    }
    
    else if( metaVals ==5 ){  // this case 5 deals with ; treating it as a newline 
         if( *w != '\0'){
           ungetc(inputchar, stdin);
           return numchars; // the ; itself is ignored but it does terminate the word and print
         }  // and empty string
         else{
           return numchars;
         }
        }
    
    /*This if condition checks for backslashes
    by comparing the current character with a backslash
    when one is identified I skip the backslash and 
    enter the next chrarcter regardless of what it is into the array
    this happens twice for a double backslash entry if there were 
    this would need to become a function*/
    
    if(inputchar == backslash){
       inputchar = getchar();
       if (inputchar == newLine){
         ungetc(inputchar, stdin);
         return numchars;
       }
      *(w+numchars) = inputchar;
      *(w+numchars+1)= '\0';
      numchars++;
      continue;
    }
    /* Here I am testing for new line exception for word boundary
    the new line also needs to be dealt with sperately 
    when it is the word boundary*/ 
     if (inputchar == newLine){ 
         if( *w != '\0'){  // this checks to see if the array is not empty
             ungetc(inputchar, stdin);
             return numchars;
         }
         else{
             return numchars;
         }

     }
   
     /* Here in this condition any character that has not already been
     processed as long as it is not a space then it will be added to 
     the array*/
     
     if (inputchar != spaceChar){
       *(w+numchars) = inputchar;
       *(w+numchars+1)= '\0';
       numchars++;
       word = 1;
     }
     
     //once a space is found that is the word boundary so renturns count
     else if(word == 1){
        return numchars;
    }
}
  /* this final condition outside of the while loop ensures a -1 is returned
  when eof is found but also deals with the last word in the array
  when eof is found immediatly after the end of a word*/
  if(inputchar == EOF){
      if( *w != '\0'){
        return strlen(w);
      }
      else{
        return -1;
      }
  }

}