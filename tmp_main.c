/*
IFJ 2018
Adam Hostin, xhosti02
Sabína Gregušová, xgregu02
Dominik Peza, xpezad00
Adrián Tulušák, xtulus00
*/

#include <stdio.h>

#include "parser.h"
#include "error.h"
#include "instructions.h"


int main(){
    FILE* input_file = stdin;
    int parser_result;
    get_source(input_file);

    ;

    generator_start(); // kontrolovat succes

    parser_result = start_parser();
    if(parser_result == ER_SYN){
        clear_code();
        return 1;
    }
    
    if(parser_result == SYN_OK){
        //FILE *f;
        //f = fopen("/home/adrian/Plocha/Škola/IFJ/IFJ/tmp_output", "w");
        flush_code(stdout);
        //fclose(f);
    }else
    {
        clear_code();
    }
   

    return 0;
}