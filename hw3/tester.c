#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>






int main(){
    char ** s1 = calloc(1, sizeof(char*));
    *(s1) = calloc(4, sizeof(char));
    strcpy(*(s1),"hOw");
    *(s1 + 3) = '\0';

    char ** s2 = calloc(1, sizeof(char*));
    *(s2) = calloc(4, sizeof(char));
    strcpy(*(s2),"hoW");
    *(s2 + 3) = '\0';

    printf("%d\n",strcasecmp(*(s1), *(s2)));
}