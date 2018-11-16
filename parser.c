/*
IFJ 2018
Adam Hostin, xhosti02
Sabína Gregušová, xgregu02
Dominik Peza, xpezad00
Adrián Tulušák, xtulus00
*/

#ifdef __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__1
#else
#define _POSIX_C_SOURCE 200809L
#endif



#include "error.h"
#include "parser.h"
#include "expression.h"
//#include "symtable.h"
#include "instructions.h"


const char* tokens[] = {
	"TYPE_EOF", 
	"TYPE_EOL", 
	"TYPE_IDENTIFIER", 
	"TYPE_KEYWORD",

	"TYPE_ASSIGN", // =
	"TYPE_NEQ", // !=
	"TYPE_LEQ", // <=
	"TYPE_LTN", // <
	"TYPE_MEQ", // >=
	"TYPE_MTN", // >
	"TYPE_EQ", // ==
	
	"TYPE_PLUS", // +
	"TYPE_MINUS", //  -
	"TYPE_MUL", // *
	"TYPE_DIV", // /
	"TYPE_QUESTION_MARK", // ?
	"TYPE_COLON", // :

	"TYPE_LEFT_BRACKET", // (
	"TYPE_RIGHT_BRACKET", // )
	"TYPE_COMMA", // ,

	"TYPE_COMMENT", // #
	"TYPE_COMMENT_START", // =begin 
	"TYPE_COMMENT_END", // =end 

	"TYPE_INT", 
	"TYPE_FLOAT", 
	"TYPE_STRING",
};



#define GET_TOKEN()   \
    do {                                                       \
        if (get_next_token(data->token) != LEXER_OK)                             \
            return(ER_LEX); \
    } while (data->token->token == TYPE_COMMENT);

#define IS_VALUE()                                                           \
    data->token->token == TYPE_INT || data->token->token == TYPE_FLOAT      \
    || data->token->token == TYPE_STRING || data->token->token == TYPE_IDENTIFIER 

#define IF_N_OK_RETURN(__func__) \
    res = __func__; \
    if (res != SYN_OK) return(res);    

#define IS_OPERAND() \
    data->token->token == TYPE_PLUS ||data->token->token == TYPE_MINUS || data->token->token == TYPE_MUL \
    || data->token->token == TYPE_DIV || data->token->token == TYPE_NEQ || data->token->token == TYPE_LEQ \
    || data->token->token == TYPE_LTN || data->token->token == TYPE_MEQ || data->token->token == TYPE_MTN \
    || data->token->token == TYPE_EQ        



/**
 * TODO: 
 * 
 * vyriešiť situáciu "func ()/func param" - bez zátvoriek
 * dorobiť kontrolu premenných a funkcií v tabulke symbolov
 *    - zmenit typ premennej po priradeni
 * 
 * urobiť <declare>
 * 
 * prerobit Errorove vystupy
 *    - if (something() != SYN_OK)
 *          return(NAVRATOVA HODNOTA of something());
 * 
 * insert_to_buffer - return value - vyriešiť napr makrom
 * 
 */

// premenna pre uchovanie ID z tokenu pre neskorsie ulozenie do TS
string_t identifier_f;
string_t identifier;

int res;
int params_cnt = 0;

tHTItem tItem;

// forward declarations
static int statement(Data_t* data);
static int declare(Data_t* data);
static int params(Data_t* data);
static int param(Data_t* data);
static int argvs(Data_t* data);
static int arg(Data_t* data);
static int value(Data_t* data);
static int function(Data_t* data);
static int print(Data_t* data); 


// Frees all the memory
int parser_error(Data_t* Data, string_t* string ){
    // frees the memory
    free_string(string);
    free(Data->token);

    return ER_SYN;
}

void save_id(string_t* identifier, Data_t* data) {
    add_string(identifier, data->token->attr.string->s);
}

// Kontrola uspesnosti lexikalnej analyzy
int get_token(Data_t* data) {
    if (get_next_token(data->token) == LEXER_OK)
        return(1);
    else
        return(0);
}

/* ****************************
 * <prog>
 * ***************************/
static int prog(Data_t* data){

    GET_TOKEN();

    printf("In <prog>, Token: %s\n", tokens[data->token->token]);

    // <prog> -> DEF ID_FUNC ( <params> ) EOL <statement> END <prog>
    if(data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_DEF){
        if (data->in_definition == false) {
            data->in_definition = true;
        } else {
            return(ER_SYN);
        }

        // ... ID_FUNC ...
        GET_TOKEN();
        save_id(&identifier_f, data);
        //printf("ID: %s\n", identifier.s);
        
        

        // ... ( ...
        GET_TOKEN();
        if (data->token->token != TYPE_LEFT_BRACKET) 
            return(ER_SYN);
        
        // ... <params> ...
        GET_TOKEN();
        params_cnt = 0;
        IF_N_OK_RETURN(params(data));
        
        // ... ) ... - nevolame GET_TOKEN
        if (data->token->token != TYPE_RIGHT_BRACKET) 
            return(ER_SYN);
    
        // ... EOL ...
        GET_TOKEN();
        if (data->token->token != TYPE_EOL) 
            return(ER_SYN);

        // priradenie ID_FUNC do tabulky
        itemupdate(&tItem, (&identifier_f)->s,  FUNCTION, true, params_cnt);
        res = htInsert(global_ST, &tItem);
        if (res != ST_OK) {
            return(res);
        }
        params_cnt = 0;

        htPrintTable(global_ST);

        
        //printf("Checkpoint 1\n");

        // ... <statement> ...
        GET_TOKEN();
        IF_N_OK_RETURN(statement(data));
        
        //printf("Checkpoint 2\n");

        // ... END ...  - nevolame GET_TOKEN, pretoze sem sa vrati zo <statement> len ak uz token == END
        if (!(data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_END))
            return(ER_SYN);

        //printf("Checkpoint 3\n");

        // ... EOL || EOF ... 
        GET_TOKEN();
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF)   
            return(prog(data));
    } 


    // <prog> -> EOL <prog>
    else if(data->token->token == TYPE_EOL){
        printf("Token in EOL loop: %d\n",data->token->token);
        return(prog(data));
    } 


    // <prog> -> EOF
    else if(data->token->token == TYPE_EOF){
        printf("EOF - exit\n");
        return SYN_OK;
    } 


    // <prog> -> <statements> <prog>
    else if(data->token->token == TYPE_KEYWORD || data->token->token == TYPE_IDENTIFIER){
        printf("INTO <statement> Token: %s\n", tokens[data->token->token]);
        int res=statement(data);
        printf("Return value of <statement> is: %d\n", res);
        return res;
        //return(statement(data));  - pouzijeme toto, ale kvoli debuggingu je to rozpisane
    }
    
        
    return ER_SYN;
}

/* ****************************
 * <statement>
 * ***************************/
static int statement(Data_t* data) {
    printf("In <statement>, in_while_of_if: %d\n", data->in_while_or_if);
    printf("Token: %s\n", tokens[data->token->token]);

    // <statement> -> IF <expression> THEN EOL <statements> ELSE EOL <statements> END EOL
    // ... IF ...
    if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_IF) {
        data->in_while_or_if++;
        // ... <expression> ...
        
        printf("Check expression\n");
        res = handle_expression(data);
        if (res == EXPRESSION_OK) {                                   
            printf("Spracoval som expression\n");
        } else 
            return(res);                     // TODO zmenit návratovú hodnotu       
        
        // ... THEN ...
        printf("Check THEN\n");
        if (data->token->token != TYPE_KEYWORD || data->token->attr.keyword != KEYWORD_THEN) {
            printf("ERR1\n");
            return(ER_SYN);
        }
    
        // ... EOL ...
        GET_TOKEN();
        printf("Check EOL\n");
        if (data->token->token != TYPE_EOL) {
            printf("ERR2\n");
            return(ER_SYN);
        }

        // ... <statements> ...
        GET_TOKEN();
        printf("Check next statement\n");
        IF_N_OK_RETURN(statement(data));
    
        // ... ELSE ... || ... EN rozsirenie - volitelna cast ELSE
        // ELSE
        printf("Check ELSE\nToken: %s\n", tokens[data->token->token]);
        if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_ELSE) {
            // ... EOL ...
            GET_TOKEN();
            printf("Check EOL\n");
            if (data->token->token != TYPE_EOL) {
                printf("ERR4\n");
                return(ER_SYN);
            }
          
            // ... <statements> ...
            GET_TOKEN();
            printf("Check next statement\n");
            IF_N_OK_RETURN(statement(data));
        }
        
        // ... END ...  - nevolame get_token(), pretoze sem sa vrati zo <statement> len ak uz token == END
        if (!(data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_END))
            return(ER_SYN);

        // ... EOL || EOF ... 
        GET_TOKEN();
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF)   
            return(prog(data));
        

        // if nothing good after IF, return ERROR  
        printf("ERR7\n");
        return(ER_SYN);
    }
    

    // <statement> -> WHILE <expression> DO EOL <statement> END EOL
    // ... WHILE ...
    else if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_WHILE) {
        data->in_while_or_if++;
        // ... <expression> ...
        res = handle_expression(data);
        if (res == EXPRESSION_OK) {      // TODO expression
            printf("Spracoval som expression\n");
        } else 
            return(res);
        
        // ... DO ...
        if (data->token->token != TYPE_KEYWORD || data->token->attr.keyword != KEYWORD_DO)
            return(ER_SYN);
    
        // ... EOL ...
        GET_TOKEN();
        if (data->token->token != TYPE_EOL)
            return(ER_SYN);
        
        // ... <statements> ...
        GET_TOKEN();
        printf("Check next statement\n");
        IF_N_OK_RETURN(statement(data));
        
        // ... END ...  - nevolame get_token(), pretoze sem sa vrati zo <statement> len ak uz token == END
        if (!(data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_END))
            return(ER_SYN);

        // ... EOL || EOF ... 
        GET_TOKEN();
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF)   
            return(prog(data));
    

        // if nothing good after WHILE, return ERROR
        return(ER_SYN);
    }

    // <statement> -> <function> EOL
    // pre vstavené funkcie
    else if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword > 8 && data->token->attr.keyword < 17) {
        printf("in <statement> -> <function>\n");
        // ... <function> ...
        IF_N_OK_RETURN(function(data));

        // ... EOL || EOF ... - token nepytame, bude vdaka nemu navratena <function>
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF)   
            return(prog(data));
    }
    

    // <statement> -> ID EOL
    // <statement> -> ID <declare> EOL
    // <statement> -> ID_FUNC ( <argvs> ) EOL
    else if (data->token->token == TYPE_IDENTIFIER) {


        // if (ID_var) then:    ************************
        printf("in <statement> ID EOL/ID <declare>/ID_FUNC...\n");
        save_id(&identifier, data);
        //*** somehow check ID in table

        GET_TOKEN();

        // <statement> -> ID_FUNC 
        if (check_define(global_ST, (&identifier)->s) == FUNCTION_DEFINED) {
            

            IF_N_OK_RETURN(argvs(data));

            return(prog(data));

        }


        // <statement> -> ID EOL || EOF 
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
            
            itemupdate(&tItem, (&identifier)->s,  VAR, false, 0);

            // ak sme v DEF
            if (data->in_definition == true) {
                res = htInsert(local_ST, &tItem);
                printf("idetmInsert returned: %d\n", res);
                if (res != ST_OK) {
                    return(res);
                }
                htPrintTable(local_ST);
            
            // ak sme na globalnej urovni
            } else {
                res = htInsert(global_ST, &tItem);
                printf("idetmInsert returned: %d\n", res);
                if (res != ST_OK) {
                    return(res);
                }
                htPrintTable(global_ST);
            }
            

            return(prog(data));
        } else



        // <statement> -> ID <declare> EOL
        // ... = ...
        if (data->token->token == TYPE_ASSIGN) {
            res = declare(data);
            if (res== SYN_OK) {
                itemupdate(&tItem, (&identifier)->s, VAR, true, 0);

                if (data->in_definition == true) {
                    res = htInsert(local_ST, &tItem);
                    printf("idetmInsert returned: %d\n", res);
                    if (res != ST_OK) {
                        return(res);
                    }
                    htPrintTable(local_ST);

                } else {
                    res = htInsert(global_ST, &tItem);
                    printf("idetmInsert returned: %d\n", res);
                    if (res != ST_OK) {
                        return(res);
                    }
                    htPrintTable(global_ST);
                }
            } else {
                return(res);
            }
            return(prog(data));
        } else

        
        

        // else if (ID_func) then :************************
        // <statement> -> ID_FUNC ( <argvs> )
        if (data->token->token == TYPE_LEFT_BRACKET) {
            /******
             * check if ID_FUNC in table
             * if (in table) {
             *    vsetko ok a pokracuj
             * } else {
             *    if (data->in_declaration == true) {
             *       save ID_FUNC in table, ID_FUNC.declared = false;
             *    } else
             *       return(ER_SYN);
             * }
             */
           
            GET_TOKEN();
            //return(argvs(data));
            IF_N_OK_RETURN(argvs(data));
            return(prog(data));

        } else {

            return(ER_SYN);
        }
    }

    

    // <statement> -> EOL <prog>
    // povoluje brat prazdne riadky ako statement - napr IF () THEN 2xEOL END
    if (data->token->token == TYPE_EOL)
        return(prog(data));

    // ****************************************
    // make <statement> return when END or ELSE
    if (data->token->token == TYPE_KEYWORD && (data->token->attr.keyword == KEYWORD_END || data->token->attr.keyword == KEYWORD_ELSE)) {
        // ak ide o IF/WHILE
        if (data->in_while_or_if != 0) {
            printf("Check in_while_or_if\n");
            if (data->token->attr.keyword == KEYWORD_END) {

                data->in_while_or_if--;
                printf("Return <statement>2\n");
                return(SYN_OK);
            }
            if (data->token->attr.keyword == KEYWORD_ELSE) {
                printf("Return <statement>1\n");
                return(SYN_OK);
            }
        } else 
        // ak ide o DEFINITION
        if (data->in_definition == true) {
            if (data->token->attr.keyword == KEYWORD_END) {
                data->in_definition = false;
                printf("Return <statement>3\n");
                return(SYN_OK);
            }
        } else
            return(ER_SYN);
    }



    printf("End <statemtnt> with ERR\n");
    return(ER_SYN);
}



/* ****************************
 * <declare>
 * ***************************/
static int declare(Data_t* data) {
    // <declare> =

    GET_TOKEN();

    // ak ID, ID_FUNC, int/flt/str
    if (IS_VALUE()) {
        insert_to_buffer(&buffer, data);

        // ... ID, ID_FUNC ...
        if (data->token->token == TYPE_IDENTIFIER) {
            // ... ID_FUNC ...
            if (check_define(global_ST, data->token->attr.string->s) == FUNCTION_DEFINED) {
                
            } else

            // ... ID ... 
            if (check_define(global_ST, data->token->attr.string->s) == PARAM_DEFINED) {
                GET_TOKEN();

                // ak nasleduje operand, vyhodnosti expression
                if (IS_OPERAND()) {
                    insert_to_buffer(&buffer, data);
                    res = handle_expression(data);
                    if (res != EXPRESSION_OK) {
                        clear_buffer(&buffer);
                        return(res);
                    }
                    clear_buffer(&buffer);
                    return(SYN_OK);

                } else

                // ... EOL || EOF ...
                if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
                    clear_buffer(&buffer);
                    return(SYN_OK);
                }

            // ak ID nie je definovane -> ERR
            } else {
                clear_buffer(&buffer);
                return(ER_SEM_VARIABLE);
            }
        } 
        // ... int/flt/str ...
        else {
            GET_TOKEN();

            // ak nasleduje operand, vyhodnosti expression
            if (IS_OPERAND()) {
                insert_to_buffer(&buffer, data);
                res = handle_expression(data);
                if (res != EXPRESSION_OK) {
                    clear_buffer(&buffer);
                    return(res);
                }
                clear_buffer(&buffer);
                return(SYN_OK);

            } else

            // ... EOL || EOF ...
            if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
                clear_buffer(&buffer);
                return(SYN_OK);
            }
        }

        clear_buffer(&buffer);
        return(SYN_OK);
    } else 

    if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword > 8 && data->token->attr.keyword < 17) {
        res = function(data);
        if (res != SYN_OK) {
            return(res);
        } else
            return(SYN_OK);
    }





    /*


    // <declare> -> = <expression>
    if (data->token->token == TYPE_IDENTIFIER) {
        insert_to_buffer(&buffer, data);
        clear_buffer(&buffer);
    } else

    // <declare> -> = ID_FUNC ( <argvs> )
    if (data->token->token == TYPE_IDENTIFIER) {
        if (check_define(global_ST, data->token->attr.string->s) == true) {
            // vyhodnotenie funkcie
        }
    }
    
    // <declare> -> = <function>
    if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword > 8 && data->token->attr.keyword < 17) {
        res = function(data);
        if (res != SYN_OK) {
            return(res);
        } else
            return(prog(data));
    } else

    // <declare> -> = <value>
    if (IS_VALUE()) {
        // prirad hodnotu a zmen typ premennej
        
    }

    */
    // <declare> -> ε
    return(ER_SYN);
}


/* ****************************
 * <params>
 * ***************************/
static int params(Data_t* data) {
    // <params> -> ID <param>
    // ... ID ...

    if (data->token->token == TYPE_IDENTIFIER) {
        printf("ID: %s\n",data->token->attr.string->s);
        int res;
        res = param(data);

        htPrintTable(local_ST);
        printf("<params> return: %d\n", res);
        return(res);
        //return(param(data));
    } else

    // <params> -> ε
    // ... ) ... - nepytame TOKEN - len pripad f()
    if (data->token->token == TYPE_RIGHT_BRACKET) {
        return(SYN_OK);
    }

    return(ER_SYN);
}

static int param(Data_t* data) {
    // <param> , ID <param>
    // ... ID ... - znovu musime kontrolovat ID, kvoli rekurzivnemu volaniu
    if (data->token->token == TYPE_IDENTIFIER) {
        printf("<param> ID: %s\n", data->token->attr.string->s);
    /**
     * somehow check ID in table
     * 
     **/
        params_cnt++;
        save_id(&identifier, data);

        res = def_ID(local_ST, (&identifier)->s);
        if (res != ST_OK) {
            return(res);
        }


        GET_TOKEN();
        // ... , ...
        if (data->token->token == TYPE_COMMA) {

            GET_TOKEN();
            return(param(data));
        
        } else
        // ... ) ...
        if (data->token->token == TYPE_RIGHT_BRACKET) {
            return(params(data));
        }
    }

    // <param> -> ε
    return(ER_SYN);
}


/* ****************************
 * <argvs>
 * ***************************/
static int argvs(Data_t* data) {
    // <argvs> -> <value> <arg>
    // ... <value> ...

    params_cnt = 0;

    printf("in <argvs>\n");
     // ... ( ... - volitelna
    if (data->token->token == TYPE_LEFT_BRACKET) {
        data->in_bracket = true;
        GET_TOKEN();
    }

    IF_N_OK_RETURN(arg(data));

    if (params_cnt != check_param_cnt((&identifier)->s)) {
        return(ER_SEM_PARAMETERS);
    }
    
    return(SYN_OK);
    // <argvs> -> ε
}


/* ****************************
 * <arg>
 * ***************************/
static int arg(Data_t* data) {
    // <arg> -> , <value> <arg>
    // <arg> -> ε

    printf("<arg>\n");
    if (IS_VALUE()) {
        params_cnt++;
        printf("in IS_VALUE\n");
    
        if (data->token->token == TYPE_IDENTIFIER) {
            if (data->in_definition == true) {
                if (check_define(local_ST, data->token->attr.string->s) != PARAM_DEFINED) {
                    return(ER_SEM_VARIABLE);
                }
            } else {
                if (check_define(global_ST, data->token->attr.string->s) != PARAM_DEFINED) {
                    return(ER_SEM_VARIABLE);
                }
            }
        }
    
        GET_TOKEN();
        // ... , ...
        if (data->token->token == TYPE_COMMA) {

            GET_TOKEN();
            return(arg(data));
        
        } else {
            // ... ) ... - len ak bola pouzita "("
            if (data->in_bracket == true) {
                if (data->token->token == TYPE_RIGHT_BRACKET) {
                    data->in_bracket = false;
                    GET_TOKEN();
                } else {
                    return(ER_SYN);
                }
            }

            // ... EOL || EOF ...
            if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
                return(SYN_OK);
            }
        }

    }


    GET_TOKEN();
        
        // ... ) ... - len ak bola pouzita "("
        if (data->in_bracket == true) {
            if (data->token->token == TYPE_RIGHT_BRACKET) {
                data->in_bracket = false;
                GET_TOKEN();
            } else {
                return(ER_SYN);
            }
        }

        // ... EOL || EOF ...
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
            return(SYN_OK);
        }

    return(ER_SYN);
}


/* ****************************
 * <value>
 * ***************************/

static int value(Data_t* data) {
    // <value> -> INT_VALUE
    if (data->token->token == TYPE_INT){

    }
    // <value> -> FLOAT_VALUE
    if (data->token->token == TYPE_FLOAT){

    }
    // <value> -> STRING_VALUE
    if (data->token->token == TYPE_STRING){

    }

    return(ER_SYN);
}


/* ****************************
 * <function>
 * ***************************/
static int function(Data_t* data) {
    printf("in <function>\n");

    // <function> -> PRINT ( <argvs> ) EOL
    if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_PRINT) {
        printf("in <function> PRINT\n");

        GET_TOKEN();

        // ... ( ... - volitelna
        if (data->token->token == TYPE_LEFT_BRACKET) {
            data->in_bracket = true;
            GET_TOKEN();
        }

        // nenulovy pocet argumentov - ak by nasledovala ")" alebo EOL/EOF -> ER_SYN
        if (IS_VALUE()) {
            IF_N_OK_RETURN(print(data));


            // ... EOL || EOF ... - uz skontrolovany token v PRINT
            return(SYN_OK);
        } else
        return(ER_SYN);
    }


    // <function> -> LENGTH ( <argvs> ) EOL
    else if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_LENGTH) {
        printf("in <function> LENGTH\n");
        
        GET_TOKEN();

        // ... ( ... - volitelna
        if (data->token->token == TYPE_LEFT_BRACKET) {
            data->in_bracket = true;
            GET_TOKEN();
        }

        // ... string ...
        if (data->token->token == TYPE_STRING) {
            // vygeneruj instrukciu pre dlzku stringu

        } else 

        // ... ID ...
        if (data->token->token == TYPE_IDENTIFIER) {
            // check ID in table - musi byt typu string
        
        } 
        
        // ak cokolvek ine -> nevyhovujuci parameter
        else {
            return(ER_SEM_TYPE);
        }

        GET_TOKEN();
        
        // ... ) ... - len ak bola pouzita "("
        if (data->in_bracket == true) {
            if (data->token->token == TYPE_RIGHT_BRACKET) {
                data->in_bracket = false;
                GET_TOKEN();
            } else {
                return(ER_SYN);
            }
        }

        // ... EOL || EOF ...
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
            return(SYN_OK);
        }

        return(ER_SYN);
    }


    // <function> -> SUBSTR ( s, i, n ) EOL
    else if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_SUBSTR) {
        printf("in <function> SUBSTR\n");
        
        GET_TOKEN();

        // ... ( ... - volitelna
        if (data->token->token == TYPE_LEFT_BRACKET) {
            data->in_bracket = true;
            GET_TOKEN();
        }

        /***
         * PRVY PARAMETER - s
         ***/
        // ... string ...
        if (data->token->token == TYPE_STRING) {
            // vyhovuje

        } else 

        // ... ID ...
        if (data->token->token == TYPE_IDENTIFIER) {
            // check ID in table - musi byt typu string
        
        } 
        
        // ak cokolvek ine -> nevyhovujuci parameter
        else {
            return(ER_SEM_TYPE);
        }

        GET_TOKEN();

        // ... , ...
        if (data->token->token == TYPE_COMMA) {
            GET_TOKEN();
        } else {
            return(ER_SYN);
        }


        /***
         * DRUHY PARAMETER - i
         ***/
        // ... int ...
        if (data->token->token == TYPE_INT) {
            // vyhovuje

        } else 

        // ... ID ...
        if (data->token->token == TYPE_IDENTIFIER) {
            // check ID in table - musi byt typu int
        
        } 
        
        // ak cokolvek ine -> nevyhovujuci parameter
        else {
            return(ER_SEM_TYPE);
        }

        GET_TOKEN();

        // ... , ...
        if (data->token->token == TYPE_COMMA) {
            GET_TOKEN();
        } else {
            return(ER_SYN);
        }


        /***
         * TRETI PARAMETER - n
         ***/
        // ... int ...
        if (data->token->token == TYPE_INT) {
            // vyhovuje

        } else 

        // ... ID ...
        if (data->token->token == TYPE_IDENTIFIER) {
            // check ID in table - musi byt typu int
        
        } 
        
        // ak cokolvek ine -> nevyhovujuci parameter
        else {
            return(ER_SEM_TYPE);
        }

        GET_TOKEN();


        // ... ) ... - len ak bola pouzita "("
        if (data->in_bracket == true) {
            if (data->token->token == TYPE_RIGHT_BRACKET) {
                data->in_bracket = false;
                GET_TOKEN();
            } else {
                return(ER_SYN);
            }
        }

        // ... EOL || EOF ...
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
            return(SYN_OK);
        }

        return(ER_SYN);
    }


    // <function> -> ORD ( s, i ) EOL
    else if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_ORD) {
        printf("in <function> ORD\n");
        
        GET_TOKEN();

        // ... ( ... - volitelna
        if (data->token->token == TYPE_LEFT_BRACKET) {
            data->in_bracket = true;
            GET_TOKEN();
        }

        /***
         * PRVY PARAMETER - s
         ***/
        // ... string ...
        if (data->token->token == TYPE_STRING) {
            // vyhovuje

        } else 

        // ... ID ...
        if (data->token->token == TYPE_IDENTIFIER) {
            // check ID in table - musi byt typu string
        
        } 
        
        // ak cokolvek ine -> nevyhovujuci parameter
        else {
            return(ER_SEM_TYPE);
        }

        GET_TOKEN();

        // ... , ...
        if (data->token->token == TYPE_COMMA) {
            GET_TOKEN();
        } else {
            return(ER_SYN);
        }


        /***
         * DRUHY PARAMETER - i
         ***/
        // ... int ...
        if (data->token->token == TYPE_INT) {
            // vyhovuje

        } else 

        // ... ID ...
        if (data->token->token == TYPE_IDENTIFIER) {
            // check ID in table - musi byt typu int
        
        } 
        
        // ak cokolvek ine -> nevyhovujuci parameter
        else {
            return(ER_SEM_TYPE);
        }

        GET_TOKEN();


        // ... ) ... - len ak bola pouzita "("
        if (data->in_bracket == true) {
            if (data->token->token == TYPE_RIGHT_BRACKET) {
                data->in_bracket = false;
                GET_TOKEN();
            } else {
                return(ER_SYN);
            }
        }

        // ... EOL || EOF ...
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
            return(SYN_OK);
        }

        return(ER_SYN);
    }


    // <function> -> CHR ( i ) EOL
    else if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_CHR) {
        printf("in <function> CHR\n");
       
        GET_TOKEN();

        // ... ( ... - volitelna
        if (data->token->token == TYPE_LEFT_BRACKET) {
            data->in_bracket = true;
            GET_TOKEN();
        }

        // ... int ...
        if (data->token->token == TYPE_INT) {
            // vyhovuje

        } else 

        // ... ID ...
        if (data->token->token == TYPE_IDENTIFIER) {
            // check ID in table - musi byt typu int
        
        } 
        
        // ak cokolvek ine -> nevyhovujuci parameter
        else {
            return(ER_SEM_TYPE);
        }

        GET_TOKEN();


        // ... ) ... - len ak bola pouzita "("
        if (data->in_bracket == true) {
            if (data->token->token == TYPE_RIGHT_BRACKET) {
                data->in_bracket = false;
                GET_TOKEN();
            } else {
                return(ER_SYN);
            }
        }

        // ... EOL || EOF ...
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
            return(SYN_OK);
        }

        return(ER_SYN);

    }


    // <function> -> INPUTS EOL
    else if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_INPUTS) {
        printf("in <function> INPUTS\n");
    
        GET_TOKEN();

        // ... ( ... - volitelna
        if (data->token->token == TYPE_LEFT_BRACKET) {
            GET_TOKEN();

            // ... ) ...
            if (data->token->token == TYPE_RIGHT_BRACKET) {
                GET_TOKEN();
            } else {
                return(ER_SYN);
            }
        }

        // ... EOL || EOF ...
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
            return(SYN_OK);
        }

        return(ER_SYN);
    }

    
    // <function> -> INPUTI EOL
    else if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_INPUTI) {
        printf("in <function> INPUTI\n");
    
        GET_TOKEN();

        // ... ( ... - volitelna
        if (data->token->token == TYPE_LEFT_BRACKET) {
            GET_TOKEN();

            // ... ) ...
            if (data->token->token == TYPE_RIGHT_BRACKET) {
                GET_TOKEN();
            } else {
                return(ER_SYN);
            }
        }

        // ... EOL || EOF ...
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
            return(SYN_OK);
        }

        return(ER_SYN);
    }


    // <function> -> INPUTF EOL
    else if (data->token->token == TYPE_KEYWORD && data->token->attr.keyword == KEYWORD_INPUTF) {
        printf("in <function> INPUTF\n");
    
        GET_TOKEN();

        // ... ( ... - volitelna
        if (data->token->token == TYPE_LEFT_BRACKET) {
            GET_TOKEN();

            // ... ) ...
            if (data->token->token == TYPE_RIGHT_BRACKET) {
                GET_TOKEN();
            } else {
                return(ER_SYN);
            }
        }

        // ... EOL || EOF ...
        if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
            return(SYN_OK);
        }

        return(ER_SYN);
    }

    return(ER_SYN);
}

/* ****************************
 * PRINT 
 * je rozpisany ako samostatna funkcia kvoli lubovolnemu poctu parametrov
 * pri kazdom dalsom parametri je rekurzivne volana znovu
 * ***************************/
static int print(Data_t* data) {
    printf("in PRINT\n");

    // ... ID ...
    if (data->token->token == TYPE_IDENTIFIER) {
        /* check in table */
    } else

    // ... INT/FLT/STR ...
    if (IS_VALUE()) {

    }

    GET_TOKEN();
    // ... , ...
    if (data->token->token == TYPE_COMMA) {
        // ... ID ...
        GET_TOKEN();
        IF_N_OK_RETURN(print(data));
        //return(print(data));
    }

    // ... ) ... - volitelna
    if (data->in_bracket == true) {
        if (data->token->token != TYPE_RIGHT_BRACKET) {
            return(ER_SYN);
            
        }
        data->in_bracket = false;
        GET_TOKEN();
    }

    // ... EOL || EOF
    if (data->token->token == TYPE_EOL || data->token->token == TYPE_EOF) {
        printf("end PRINT success\n");
        return(SYN_OK);
    }

    return(ER_SYN);
}



static bool init_struct(Data_t* data){
    data->token = malloc(sizeof(Token_t));
    data->token->token = 100;
    data->in_bracket = false;
    data->in_while_or_if = 0;
    data->in_definition = false;

    return true;
}


/**
 * PARSER
 **/
int start_parser(){

    Data_t our_data;
    string_t string;

    allocate_string(&string);
    set_string(&string);

    if(!init_struct(&our_data)){
        free_string(&string);
    }

    // inicializacia bufferu
    init_buffer(&buffer);

    //identifier = (char *)malloc(sizeof(char));
    allocate_string(&identifier);
    allocate_string(&identifier_f);

    // inicializacia tabulky symbolov
    STinits();
    iteminit(&tItem, "",  NILL, false, 0);
    

    

    /*
    tstackP *s;
    FrameStackInit(s);
    */

    int res;
    res = prog(&our_data);

    value(&our_data);




    // odstránenie bufferu
    clear_buffer(&buffer);

    // odstránenie tabulky
    htClearAlltables();
    itemfree(&tItem);
    
    free_string(&string);
    free(our_data.token);

    //free(identifier);
    free_string(&identifier);
    free_string(&identifier_f);

    /*
    DeleteStack(s);
    */

    printf("EXIT code: %d\n", res);

    if (our_data.in_while_or_if != 0)
        return(ER_SYN);
    else
        return res;
    //return 0;
}







