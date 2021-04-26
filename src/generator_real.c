
#include "vslc.h"

#define MIN(a,b) (((a)<(b)) ? (a):(b))
static const char *record[6] = {
    "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
};

size_t nparms = 0, if_count = 0, while_count = 0;

static void find_variable(node_t* node, char* str);
static void handle_function_call(node_t* node);
static void handle_return(node_t* node);
static void handle_assignment(node_t* node);
static void handle_printing(node_t* node);
static void handle_expression(node_t* node);
static void handle_while_statement(node_t* node);
static void handle_if_statement(node_t* node);
static void handle_continue(node_t* node);
static void assign_variable_value(node_t* node, char* str);

static void
generate_stringtable ( void )
{
    /* These can be used to emit numbers, strings and a run-time
     * error msg. from main
     */
    puts ( ".data" );
    puts ( "intout: .asciz \"\%ld \"" );
    puts ( "strout: .asciz \"\%s \"" );
    puts ( "errint64out: .asciz \"Wrong number of arguments\"" );

    for (int i = 0; i < stringc; i++) {
        printf("STR%d:\t.string\t%s\n", i, string_list[i]);
    }

}


static void
generate_main ( symbol_t *first )
{
    puts ( ".globl main" );
    puts ( ".text" );
    puts ( "main:" );
    puts ( "\tpushq %rbp" );
    puts ( "\tmovq %rsp, %rbp" );

    puts ( "\tsubq $1, %rdi" );
    printf ( "\tcmpq\t$%zu,%%rdi\n", first->nparms );
    puts ( "\tjne ABORT" );
    puts ( "\tcmpq $0, %rdi" );
    puts ( "\tjz SKIP_ARGS" );

    puts ( "\tmovq %rdi, %rcx" );
    printf ( "\taddq $%zu, %%rsi\n", 8*first->nparms );
    puts ( "PARSE_ARGV:" );
    puts ( "\tpushq %rcx" );
    puts ( "\tpushq %rsi" );

    puts ( "\tmovq (%rsi), %rdi" );
    puts ( "\tmovq $0, %rsi" );
    puts ( "\tmovq $10, %rdx" );
    puts ( "\tcall strtol" );

    /*  Now a new argument is an integer in rax */
    puts ( "\tpopq %rsi" );
    puts ( "\tpopq %rcx" );
    puts ( "\tpushq %rax" );
    puts ( "\tsubq $8, %rsi" );
    puts ( "\tloop PARSE_ARGV" );

    /* Now the arguments are in order on stack */
    for ( int arg=0; arg<MIN(6,first->nparms); arg++ )
        printf ( "\tpopq\t%s\n", record[arg] );

    puts ( "SKIP_ARGS:" );
    printf ( "\tcall\t_%s\n", first->name );
    puts ( "\tjmp END" );
    puts ( "ABORT:" );
    puts ( "\tmovq $errout, %rdi" );
    puts ( "\tcall puts" );

    puts ( "END:" );
    puts ( "\tmovq %rax, %rdi" );
    puts ( "\tcall exit" );
}

void print_global_variables() {
	size_t n_globals = tlhash_size(global_names);
	symbol_t *global_list[n_globals];
	tlhash_values ( global_names, (void **)&global_list );
	puts(".data");
	for (int i = 0; i < n_globals; i++) {
		if (global_list[i]->type == SYM_GLOBAL_VAR) {
			printf("_%s:\t.zero\t8\n", global_list[i]->name);	//come back to this
    		}
	}
}

void print_string(int64_t string_num) {
	printf("\tmovq $STR%d, %%rsi\n", (int) string_num);
	puts("\tmovq $strout, %rdi");
	puts("\tcall printf");
}

void print_int(char* dest) {
	puts("\tmovq $intout, %rdi");
	puts("\tcall printf");
}

void print_new_line() {
	puts("\tmovq $'\\n', %rdi");
	puts("\tcall putchar");
}

void assign_variable_value(node_t* node, char* destination) {
    switch (node->entry->type) {
        int64_t seq = 0;
        case SYM_PARAMETER:
            seq = node->entry->seq;
            printf("\tmovq %s, %s%d(%%rbp)\n", destination, seq >= 6 ? "" : "-", 8 * ((int) (seq >= 6 ? seq - 4 : seq + 1)));
            break;
        case SYM_GLOBAL_VAR:
            printf("\tmovq %s, _%s\n", destination, node->entry->name);
            break;
        case SYM_LOCAL_VAR:
            printf("\tmovq %s, -%d(%%rbp)\n", destination, (int) (8 * ((int) node->entry->seq + MIN(nparms, 6) + 1)));
            break;
    }
}



void
generate_program ( void )
{
	generate_stringtable();
	print_global_variables();
	size_t n_globals = tlhash_size(global_names);
	symbol_t *global_list[n_globals];
	tlhash_values ( global_names, (void **)&global_list );
	for (int i = 0; i < n_globals; i++) {
		if (global_list[i]->type == SYM_FUNCTION && global_list[i]->seq == 0) {
			generate_main(global_list[i]);
			break;
        	}
    	}
	print_functions();
}

