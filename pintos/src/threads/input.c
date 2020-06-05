//
// Created by jinho on 6/2/2020.
//

#include "input.h"
#include <stdio.h>
int get_line(char* line,int max_size){
    int c=0;
    int len = 0;
    while(len < max_size && c!='\r'){
        c=input_getc();
        
        if (c == '\b') {
            if (len > 0) {
                len--;
                printf("\b \b");
            }
        } else {
            if (c == '\r') {
                line[len]=0;
                putchar('\n');
            } else {
                line[len++] = c;
                putchar(c);
            }
        }
    }
    
    
    line[len] = '\0';
    return len;
}
