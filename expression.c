/*
IFJ 2018
Adam Hostin, xhosti02
Sabína Gregušová, xgregu02
Dominik Peza, xpezad00
Adrián Tulušák, xtulus00
*/

#include "expression.h"

#define GET_TOKEN()                                     \
    if (get_next_token(data->token) != LEXER_OK)        \
        return(ER_LEX)


/**
 * Checks type mismatch in numerical operations
 */
#define CHECK_NUMERICAL_OPERATIONS()              \
    if(!check_operations(stack, to_pop, operand)){             \
        return_code = OPERATION_TYPE_MISMATCH;                                 \
        return(false);                                                         \
    }                                                                          \


/**
 * 
 * Keď by bolo že a + 3 - func(argvs), 
 * ako prideme na to, že čo vracia func?
 * Jedna sa o rozšírenie FUNEXP
 * 
 * Konverzia int -> float
 * Kontrola typov
 * IDIV
 * Token buffer
 * Generating code
 * 
*/


Symbol_stack_t stack;
bool finished = false;
int return_code = EXPRESSION_OK;

/***********************************************************
 * 
 *                  "main" of expression.c
 * 
 **********************************************************/

/**
 * Handles the epxression, get the first token an token that symbolizes end of expression 
 */
int handle_expression(Data_t* data){
    bool can_get_token = true;
    bool is_reduced = false;
    Data_type to_push_type;
    Precedential_table_symbol to_push_symbol;
    Precedential_table_rule current_rule;
    Symbol_item_t* tmp;

    printf("\n\n");

    // initialize stack
    init_stack(&stack);

    // stack contains : $
    if(!push_stack(&stack, NIL, DOLLAR)){ // push dollar
        return expression_error(&stack, ER_INTERNAL);
    }
    printf("Som zásobník a obsahujem zatial snad iba dolár\n");
    print_current_stack(&stack);
    

    GET_TOKEN(); // get first token

    // We iterate through all the tokens and check their syntax
    // When the rule is SHIFT or EQUAL, we can get new token
    // When the rule is REDUCE, we don't get any new tokens
    // UDEFINED symbolizes either gramatical error or succesfull reduction of expression 
    while(!is_reduced){ 
        current_rule = get_indexes_and_rule(&stack, data);  // get current rule
        to_push_type = get_data_type(data);                 // get type of token
        to_push_symbol = get_symbol_from_token(data);       // get symbol from token
        
        if(current_rule == S){  // SHIFT rule
            can_get_token = true;
            if(!add_after_first_terminal(&stack, NIL, START)){      // push start symbol after first terminal
                return expression_error(&stack, ER_INTERNAL);
            }
            if(!push_stack(&stack, to_push_type, to_push_symbol)){ // push token symbol
                return expression_error(&stack, ER_INTERNAL);
            }
        }
        if(current_rule == R){  // REDUCE rule
            can_get_token = false;                                  // don't get any new token
            if(!reduce_by_rule(&stack))                             // reduce it
                return expression_error(&stack, return_code); 
        }
        if(current_rule == E){  // EQUAL rule
            can_get_token = true;                                   // can get new token
            if(!push_stack(&stack, to_push_type, to_push_symbol)){  // push token symbol
                return expression_error(&stack, ER_INTERNAL);
            }
        }
        if(current_rule == U){  // UNDEFINED rule
            Precedential_table_symbol last_symbol = get_first_term(&stack);
            to_push_symbol = get_symbol_from_token(data);
            if(finished && last_symbol == DOLLAR && to_push_symbol == DOLLAR){ // we are at the and reduced everything succesfully
                break;
            }
            else{
                return expression_error(&stack, OTHER_SYNTACTICAL_ERRORS);
            }    
        }
        if(can_get_token)
            GET_TOKEN();
        print_current_stack(&stack); // DEBUG
    }
    printf("While has finished succesfully!\n"); // DEBUG
    print_token(data);

    // if we come here, the syntax and semantics were correct
    free_stack(&stack);
    return EXPRESSION_OK;
}

/**
 * Checks possible operations with strings
 */
bool allowed_string_operations(Precedential_table_symbol symbol){
    switch(symbol){
        case PLUS:
        case EQL:
        case NEQ:
        case LEQ:
        case LTN:
        case MEQ:
        case MTN:
            return true;
        default:
            return false;
    }
}

/**
 * Checks types of operation and whether there is mismatch or no
 */
bool check_operations(Symbol_stack_t* stack, int to_pop, Precedential_table_symbol operand){
    Data_type current_data;
    Symbol_item_t* first_symbol = stack->top;
    Symbol_item_t* third_symbol = stack->top->next->next;

    bool same_type = false;


    // Computes what kinds of type will be the type after operation    
    if(first_symbol->type == third_symbol->type){
        if(first_symbol->type == INT){
            same_type = true;
            current_data = INT;
        }
        else if(first_symbol->type == FLT){
            same_type = true;
            current_data = FLT;
            }
        else if(first_symbol->type == STR){
            if(allowed_string_operations(operand)){
                same_type = true;
                current_data = STR;
            }
            else{
                return false;
            }
        }
        else{
            return false;
        }
    }
    else if(first_symbol->type == INT && first_symbol->type == FLT){
        current_data = FLT;
    }
    else if(first_symbol->type == FLT && first_symbol->type == INT){
        current_data = FLT;
    }
    else{
        return false;
    }

    pop_count(to_pop);
    push_stack(stack, current_data, NON_TERMINAL);
    return true;
}

/**
 * Reduce it by the rule
 */
bool reduce_by_rule(Symbol_stack_t* stack){
    int count = count_to_reduce(stack);     // how many symbols are we reducing
    int to_pop = count + 1;                 // we have to pop the count + the start symbol
    Symbol_item_t* tmp_first = stack->top;  // first item from the stack
    Data_type current_data_type = tmp_first->type;
    Precedential_table_symbol operand;

    // we are reducing 1 symbol
    if(count == 1){ // i -> E
        if(tmp_first->symbol == ID){
            pop_count(to_pop);
            push_stack(stack, current_data_type, NON_TERMINAL);
        }
    } // we are reducing 3 symbols
    else if(count == 3){
        Symbol_item_t* tmp_second = stack->top->next;
        Symbol_item_t* tmp_third = stack->top->next->next;

        // first and third symbols are nonterminals, so we just check the second operand to make sure the grammar is correct
        if(tmp_first->symbol == NON_TERMINAL && tmp_third->symbol == NON_TERMINAL){ // E X E
            operand = tmp_second->symbol;
            if(operand == PLUS){         // E + E
                CHECK_NUMERICAL_OPERATIONS(); 
            }
            else if(operand == MINUS){   // E - E
                CHECK_NUMERICAL_OPERATIONS();
            }
            else if(operand == MUL){     // E * E
                CHECK_NUMERICAL_OPERATIONS();
            }
            else if(operand == DIV){     // E / E
                CHECK_NUMERICAL_OPERATIONS();
            }
            else if(operand == EQL){     // E == E
                CHECK_NUMERICAL_OPERATIONS();
            }
            else if(operand == NEQ){     // E != E
                CHECK_NUMERICAL_OPERATIONS();
            }
            else if(operand == LEQ){     // E <= E
                CHECK_NUMERICAL_OPERATIONS();
            }
            else if(operand == LTN){     // E < E
                CHECK_NUMERICAL_OPERATIONS();
            }
            else if(operand == MEQ){     // E >= E
                CHECK_NUMERICAL_OPERATIONS();
            }
            else if(operand == MTN){     // E > E
                CHECK_NUMERICAL_OPERATIONS();
            }
            else{
                return_code = OTHER_SYNTACTICAL_ERRORS;
                return false;
            }
        } 
        else{ // the non_terminal is in brackets (E) -> E
            if(tmp_first->symbol == RIGHT_B && tmp_third->symbol == LEFT_B){
                if(tmp_second->symbol == NON_TERMINAL){
                    current_data_type = tmp_second->type;
                    pop_count(to_pop);
                    push_stack(stack, current_data_type, NON_TERMINAL);
                }
                else{
                    return_code = OTHER_SYNTACTICAL_ERRORS;
                    return false;
                }
            }
            else{
                return_code = OTHER_SYNTACTICAL_ERRORS;
                return false;
            }
        }
    } // other count
    else{
        return_code = OTHER_SYNTACTICAL_ERRORS;
        return false;
    }

    // if it passed all the grammar tests, it was correct
    return true;
}

/**
 * Adds symbol after first terminal symbol
 */
bool add_after_first_terminal(Symbol_stack_t* stack, Data_type type, Precedential_table_symbol symbol){
    bool found = false;
    Symbol_item_t* tmp = stack->top;
    Symbol_item_t* add_after;
    Symbol_item_t* non_terminal;
    Symbol_item_t* new_item = malloc(sizeof(Symbol_item_t));
    if(new_item == NULL){
        return false;
    }

    new_item->symbol = symbol;
    new_item->type = type;    
    
    if(is_term(stack->top->symbol)){ // terminal is on top
        new_item->next = stack->top;
        stack->top = new_item;
        return true;
    }
    else{                           // not on top, search through the stack
        while(!found && tmp != NULL){
            if(is_term(tmp->next->symbol)){
                add_after = tmp;
                non_terminal = tmp->next;
                add_after->next = new_item;
                new_item->next = non_terminal;
                return true;
            }
            tmp = tmp->next;
        }    
    }
    return false;
}

/**
 * Evaluates, whether symbol is non-terminal or not
 */
bool is_term(Precedential_table_symbol symbol){
    switch(symbol){
        case NON_TERMINAL:
        case START:
            return false;
        default:
            return true;
    }
}

/**
 * Gets first terminal from the table
 */
Precedential_table_symbol get_first_term(Symbol_stack_t* stack){
    bool found = false;
    Symbol_item_t* tmp = stack->top;

    while(!found){
        if(is_term(tmp->symbol)){
            return tmp->symbol;
        }
        tmp = tmp->next;
    }
}

/**
 * Computes the rows and columns value and gets the rule
 */
Precedential_table_rule get_indexes_and_rule(Symbol_stack_t* stack, Data_t* data){
    int rows = get_index(get_first_term(stack));
    int columns = get_index(get_symbol_from_token(data));
    return get_rule(rows, columns);
}


/**
 * Calculates how many symbols we have to pop
 */
int count_to_reduce(Symbol_stack_t* stack){
    int count = 0;
    Symbol_item_t* tmp = stack->top;

    while(tmp->symbol != START){
        count++;
        tmp = tmp->next;
    }
    return count;
}

/**
 * Pops n symbols from the stack
 */
void pop_count(int n){
    for(int i = 0; i < n; i++){
        pop_stack(&stack);
    }
}

/**
 * Gets rule according to columns and rows
 */
Precedential_table_rule get_rule(Precedential_table_symbol rows, Precedential_table_symbol columns){
    //printf("Rows: %d\tColumns: %d\tRule: %s\n", rows, columns, rules[precedential_table[rows][columns]]);
    if(rows == (table_size-1) && columns == (table_size-1))
        finished = true;
    return precedential_table[rows][columns];
}

/**
 * Returns error_type when comething goes wrong
 */
int expression_error(Symbol_stack_t* stack, int error_type){
    free_stack(stack);
    printf("EXPRESSION ERROR!\n");
    return error_type;
}

/**
 * Debug functions
 */
void print_token(Data_t* data){
    if(data->token->token)
        printf("%s\n", tokens[data->token->token]);
}

/**
 * Debug functions
 */
void print_current_stack(Symbol_stack_t* stack){ // 
    if(stack->top == NULL)
        return;
    else{
        Symbol_item_t* tmp = stack->top;
        int i = 0;
        while(tmp != NULL){
            printf("%d : %s\n", i, symbols[tmp->symbol]);
            i++;
            tmp = tmp->next;
        }
        printf("\n");
    }
}

/**
 * Evaluates what data type of token
 */
Data_type get_data_type(Data_t* data){ // TODO: search identifier from table
    if(data->token->token == TYPE_INT)
        return INT;
    else if(data->token->token == TYPE_FLOAT)
        return FLT;
    else if(data->token->token == TYPE_STRING)
        return STR;
    else if(data->token->token == TYPE_IDENTIFIER){
        // search the table to get the type
    }
    else
        return NIL;
}

/**
 * Returns symbol of precedential table from token 
 */
Precedential_table_symbol get_symbol_from_token(Data_t* data){
    switch(data->token->token){
        case(TYPE_PLUS):
            return PLUS;
        case(TYPE_MINUS):
            return MINUS;
        case(TYPE_MUL):
            return MUL;
        case(TYPE_DIV):
            return DIV;
        case(TYPE_LEFT_BRACKET):
            return LEFT_B;
        case(TYPE_RIGHT_BRACKET):
            return RIGHT_B;
        case(TYPE_INT):
        case(TYPE_STRING):
        case(TYPE_FLOAT):
        case(TYPE_IDENTIFIER):
            return ID;
        case(TYPE_EQ):
            return EQL;
        case(TYPE_NEQ):
            return NEQ;
        case(TYPE_LEQ):
            return LEQ;
        case(TYPE_LTN):
            return LTN;
        case(TYPE_MEQ):
            return MEQ;
        case(TYPE_MTN):
            return MTN;
        default:
            return DOLLAR;
    }
}

/**
 * Gets index number from symbol
 */
Precedential_table_index get_index(Precedential_table_symbol symbol){
    switch(symbol){
        case(PLUS):
            return INDEX_PLUS;
        case(MINUS):
            return INDEX_MINUS;
        case(MUL):
            return INDEX_MUL;
        case(DIV):
            return INDEX_DIV;
        case(LEFT_B):
            return INDEX_LEFT_BRACKET;
        case(RIGHT_B):
            return INDEX_RIGHT_BRACKET;
        case(ID):
            return INDEX_IDENTIFIER;
        case(EQL):
        case(NEQ):
        case(LEQ):
        case(LTN):
        case(MEQ):
        case(MTN):
            return INDEX_RELATIONAL_OPERATION;
        case(DOLLAR):
            return INDEX_DOLLAR;
    }
}

/***********************************************************
 * 
 *                  STACK FUNCTIONS
 * 
 **********************************************************/

/**
 * Initializes top of stack to null
 */
void init_stack(Symbol_stack_t* stack){
    stack->top = NULL;
}

/**
 * Pushes symbol to stack
 */
bool push_stack(Symbol_stack_t* stack, Data_type type, Precedential_table_symbol symbol){
    Symbol_item_t* new_item = malloc(sizeof(Symbol_item_t));

    if(new_item == NULL){
        return false;
    }
    else{
        new_item->next = stack->top;
        new_item->symbol = symbol;
        new_item->type = type;
        stack->top = new_item;
        return true;
    }  
}

/**
 * Pops symbol from stack
 */
bool pop_stack(Symbol_stack_t* stack){
    Symbol_item_t* tmp = stack->top;
    if(tmp == NULL){
        return true;
    }
    else if(tmp->next == NULL){
        free(tmp);
    }
    else{
        stack->top = stack->top->next;
        free(tmp);
        tmp = NULL;
    }
    return true;
}

/**
 * Frees all the memory of stack - used at the end of expression or when error
 */
void free_stack(Symbol_stack_t* stack){
    if(stack->top == NULL){
        return;
    }
    while(stack->top != NULL){
        Symbol_item_t* tmp = stack->top;
        stack->top = tmp->next;
        free(tmp);
    }
}