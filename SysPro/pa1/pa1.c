#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_SIZE 100

int main(int argc, char** argv){

    if (argc != 4) {
        // checks if the command was entered in an understandable manner
        printf("Usage: %s <filename> <r|c> <integer>\n", argv[0]);
        return -1;
    }

    // check to see if the index number is at least within range
    if (atoi(argv[3]) == 0 || atoi(argv[3]) > 100) {
        printf("(exit with status -1): error in input format at line %s", argv[3]);
        return -1;
    }

    // assign command line variables
    char* inputFile = argv[1];
    char rowOrCol = argv[2][0];
    int index = atoi(argv[3]);

    //declare other variables
    unsigned int table2d[MAX_SIZE][MAX_SIZE];
    int rCount = 0;
    int cCount = 0;
    char line[1024];

    // last validation: check validity of command line argument, although file will actually be checked last
    if (rowOrCol != 'r' && rowOrCol != 'c') {
        printf("Usage: %s <filename> <r|c> <integer>\n", argv[0]);
        return -1;
    }

    // read in file
    FILE* filename = fopen(inputFile, "r");
    if (filename == NULL){
        printf("Error opening file");
        return -1;
    }

    //while loop through a row/column of file and make 2d array
    while(fgets(line, sizeof(line), filename) && rCount < MAX_SIZE){
        size_t len = strlen(line);
        if(len > 0 && line[len - 1] == '\n'){ //remove newline character
            line[--len] = '\0';
        }

        char* token = strtok(line, ","); //split line by the comma
        cCount = 0;
        while(token != NULL && cCount < MAX_SIZE){ 
            table2d[rCount][cCount++] = strtoul(token, NULL, 10); 
            token = strtok(NULL, ","); //get the next token
        }
        rCount++;
    }
    
    //close file
    fclose(filename);

    //more error handling - check if index is in range and if input is r or c
    if ((rowOrCol == 'r' && index >= rCount) || (rowOrCol == 'c' && index >= cCount)) {
        printf("error in input format at line %d\n", index);
        return -1;
    }

    unsigned int values[MAX_SIZE];
    int count = 0;

    if(rowOrCol == 'r'){ //get all values in row and add to 1d array
        for(int j = 0; j < cCount; j++){
            values[count++] = table2d[index][j];
        }
    }

    if(rowOrCol == 'c'){ //get all values in column and add to 1d array
        for(int j = 0; j < rCount; j++){
            values[count++] = table2d[j][index];
        }
    }

    double avg;
    unsigned int min = values[0];
    unsigned int max = values[0];
    unsigned long long int sum = 0;

    //calculate min, max, and sum with the values in 1d array
    for(int i = 0; i < count; i++){        
        // if values[i] is lower than the current minimum
        if(values[i] < min){
            // then it is the new minimum
            min = values[i];
        }

        // if values[i] is higher than the current maximum
        if(values[i] > max){
            // then it is the new maximum
            max = values[i];
        }
        
        // add all values together
        sum += values[i];
    }

    // find the average by dividing the sum by the number of values
    // we add 1 because the for loop undercounts by 1
    avg = (double) sum / count;

    //row or column for results
    char* rcOutput = (rowOrCol == 'r' ? "row" : "column");

    //print results
    printf("%s %s %.2f %u %u\n", argv[1], rcOutput, avg, max, min);

    return 0;
}
