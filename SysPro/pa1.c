#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int countRows(FILE file){ //figure out this and how to traverse columns
    int count = 0;
    
    while(fgets != NULL){

    }
}

int main(int argc, char *argv){

    if (argc !=4) {
        // checks if the file name was entered in an understandable manner
        printf("Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    //check validity 

    // assign command line variables
    char* inputFile = argv[1];
    char rowOrCol = argv[2];

    char* rcOutput;
    if(rowOrCol == 'r'){
        rcOutput = "row";
    }
    else if (rowOrCol == 'c'){
        rcOutput = "column";
    }

    int index = atoi(argv[3]);

    //declare other variables
    unsigned int sum = 0;
    unsigned int min = 0;
    unsigned int max = 0;
    double avg = 0;

    FILE* filename = fopen(inputFile, "r");
    if(filename == NULL){
        printf("Error reading file");
        return EXIT_FAILURE;
    }

    //while loop through a row/column of file and make 1d array


    //calculate min/max/avg with the values in 1d array
    int i;
    for(i = 0; i < array.length; i++){
        
        if(array[i] < min){
            min = array[i];
        }
        if(array[i] > max){
            min = array[i];
        }
        sum += array[i];
    }
    avg = (double) sum / (++i);

    printf("%s %s %f %d %d", filename, rcOutput, avg, max, min);
}