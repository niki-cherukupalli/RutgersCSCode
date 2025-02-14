#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define MAX_SIZE 100


void calculate(unsigned int values[], int count, double *avg, unsigned int min, unsigned int max, char filename, char rowOrCol){
    min = UINT_MAX;
    max = 0;
    unsigned int sum = 0;

    for(int i = 0; i < count; i++){
        sum += values[i];
        if(values[i] < min){
            min = values[i];
        }
        if(values[i] > max){
            max = values[i];
        }
    }

    *avg = (double)sum / count;
}

int main(int argc, char *argv){

    if (argc != 4) {
        // checks if the command was entered in an understandable manner
        printf("Usage: %s <filename> <r|c> <integer>\n", argv[0]);
        return -1;
    }

    //check validity of command line argument, although file will actually be checked last
    if (argv[2] != 'r' || argv[2] != 'c') {
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
    char rowOrCol = argv[2];
    int index = atoi(argv[3]);

    //declare other variables
    unsigned int table2d[MAX_SIZE][MAX_SIZE];
    int rCount = 0, cCount = 0;
    char line[1024];

    // read in file
    FILE* filename = fopen(inputFile, "r");
    if (filename == NULL){
        printf("Error opening file");
        return -1;
    }

    //while loop through a row/column of file and make 2d array
    while(fgets(line, sizeof(line), filename) && rCount < MAX_SIZE){
        size_t len = strlen(line);
        if(len > 0 && line[len - 1] == '\n'){
            line[--len] = '\0';
        }

        char* token = strtok(line, ",");
        cCount = 0;
        while(token != NULL && cCount < MAX_SIZE){
            table2d[rCount][cCount++] = strtoul(token, NULL, 10);
            token = strtok(NULL, ",");
        }
        rCount++;
    }
    
    //close file
    fclose(filename);

    //more error handling
    if ((rowOrCol == 'r' && index >= rCount) || (rowOrCol == 'c' && index >= cCount)) {
        fprintf(stderr, "error in input format at line %d\n", index);
        return -1;
    }

    unsigned int values[MAX_SIZE];
    int count = 0;

    if(rowOrCol == 'r'){
        for(int j = 0; j < cCount; j++){
            values[count++] = table2d[index][j];
        }
    }
    else if(rowOrCol == 'c'){
        for(int j = 0; j < rCount; j++){
            values[count++] = table2d[j][index];
        }
    }
    else{
        //error handling
        fprintf(stderr, "Invalid mode. Use 'r' for row or 'c' for column\n");
        return -1;
    }

    double avg;
    unsigned int min, max;

    //function to calculate average, min, and max
    calculate(values, count, &avg, &min, &max, inputFile, rowOrCol);

    //row or column for results
    char* rcOutput = (rowOrCol == 'r' ? "row" : "column");

    //print results
    printf("%s %s %.2f %u %u\n", argv[1], rcOutput, avg, max, min);

    return 0;
}
