%{
#include "ast.h"
#include "ast_symbol.h"
#include "intermediate.h"
extern int yylex(void);
extern void yyerror(char *str, ...);
extern int yydebug;
%}

%union{
    struct node_t* node;
}

%nonassoc LOWER_FAKE_MARK
%token <node> VOID INT FLOAT DOUBLE CHAR STRUCT
%left <node> COMMA
%right <node> ASSIGN ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%left <node> OR
%left <node> AND
%left <node> EQ NE
%left <node> GT LT GE LE
%left <node> ADD SUB
%left <node> MUL DIV MOD
%right <node> AUTO_INCR AUTO_DECR NOT REF
%token <node> IF FOR DO WHILE RETURN
%nonassoc <node> ELSE
%left <node> LP RP LB RB LCB RCB SEMICOLON DOT ARROW
%token <node> INUM FNUM DNUM ID CHARACTER STRING
%nonassoc HIGHER_FAKE_MARK

%type <node> type const left_unary_operator right_unary_operator subscript member_selection expr bool_expr
%type <node> initialize_list initialize_expr declare_clause pointer_declare array_declare dereference declare sentence stmt stmts code_block
%type <node> if_clause else_if_clause else_clause if_else_if_clause if_stmt
%type <node> for_clause for_expr for_stmt while_stmt do_while_stmt
%type <node> return_stmt decl_args_clause decl_args decl_func call_args call_func
%type <node> decl_struct_variable_clause decl_struct_variable decl_struct_variables decl_struct
%type <node> start program root

%%

    root: program   { 
        print_tree($1);
        gen_code($1);
        print_quadruples();
        free_tree($1);
        free_address3_pool();
        free_quadruples_array();
        }
        ;

    program: start      { $$ = new_node("program", 1, $1); }
        | start program { $$ = l_merge_node($2, $1); }
        ;

    start: declare SEMICOLON    { $$ = $1; }
        | decl_struct SEMICOLON { $$ = $1; }
        | decl_func             { $$ = $1; }
        ;

    decl_struct: STRUCT LCB decl_struct_variables RCB   { $$ = new_node("declare struct", 2, $1, $3); }
        ;

    decl_struct_variables: decl_struct_variable SEMICOLON       { $$ = new_node("declare struct variables", 1, $1); }
        | decl_struct_variables decl_struct_variable SEMICOLON  { $$ = merge_node($1, $2); }
        ;

    decl_struct_variable: type decl_struct_variable_clause          { $$ = new_node("declare", 2, $1, $2); }
        | decl_struct_variable COMMA decl_struct_variable_clause    { $$ = merge_node($1, $3); }
        | /* empty */                                               { $$ = new_node("declare", 0); }
        ;

    decl_struct_variable_clause: ID { $$ = new_node("declare clause", 1, $1); }
        | array_declare             { $$ = new_node("declare clause", 1, $1); }
        | pointer_declare           { $$ = new_node("declare clause", 1, $1); }
        ;

    decl_func: type ID LP decl_args RP code_block   { $$ = new_node("declare function", 4, $1, $2, $4, $6); }
        ;

    decl_args: decl_args_clause        { $$ = new_node("declare arguments", 1, $1); }
        | decl_args COMMA decl_args_clause  { $$ = merge_node($1, $3); }
        | /* empty */                       { $$ = new_node("declare arguments", 0); }
        ;

    decl_args_clause: type ID   { $$ = new_node("declare", 2, $1, new_node("declare clause", 1, $2)); }
        | type pointer_declare  { $$ = new_node("declare", 2, $1, new_node("declare clause", 1, $2)); }
        | type array_declare    { $$ = new_node("declare", 2, $1, new_node("declare clause", 1, $2)); }
        ;

    call_func: ID LP call_args RP   { $$ = new_node("call function", 2, $1, $3); }
        ;

    call_args: expr             { $$ = new_node("call arguments", 1, $1); }
        | call_args COMMA expr  { $$ = merge_node($1, $3); }
        | /* empty */           { $$ = new_node("call arguments", 0); }
        ;

    return_stmt: RETURN SEMICOLON   { $$ = new_node("return statement", 0); }
        | RETURN expr SEMICOLON     { $$ = new_node("return statement", 1, $2); }
        ;

    do_while_stmt: DO stmt WHILE LP bool_expr RP SEMICOLON   { $$ = new_node("do while statement", 2, $2, $5); }
        | DO code_block WHILE LP bool_expr RP SEMICOLON      { $$ = new_node("do while statement", 2, $2, $5); }
        ;

    while_stmt: WHILE LP bool_expr RP stmt   { $$ = new_node("while statement", 2, $3, $5); }
        | WHILE LP bool_expr RP code_block   { $$ = new_node("while statement", 2, $3, $5); }
        ;

    for_stmt: FOR LP for_expr RP code_block { $$ = new_node("for statement", 2, $3, $5); }
        | FOR LP for_expr RP stmt           { $$ = new_node("for statement", 2, $3, $5); }
        ;

    for_expr: for_clause SEMICOLON bool_expr SEMICOLON for_clause    { $$ = new_node("for expression", 3, $1, $3, $5); }
        ;

    for_clause: sentence    { $$ = $1; }
        | /* empty */       { $$ = new_node("empty statement", 0); }
        ;

    if_stmt: if_else_if_clause else_clause          { $$ = merge_node($1, $2); }
        | if_else_if_clause %prec LOWER_FAKE_MARK   { $$ = $1; }
        | if_clause else_clause                     { $$ = new_node("if statement", 2, $1, $2); }
        | if_clause %prec LOWER_FAKE_MARK           { $$ = new_node("if statement", 1, $1); }
        ;

    if_else_if_clause: if_clause else_if_clause { $$ = new_node("if statement", 2, $1, $2); }
        | if_else_if_clause else_if_clause      { $$ = merge_node($1, $2); }
        ;

    else_if_clause: ELSE IF LP bool_expr RP stmt %prec HIGHER_FAKE_MARK  { $$ = new_node("else if", 2, $4, $6); }
        | ELSE IF LP bool_expr RP code_block %prec HIGHER_FAKE_MARK      { $$ = new_node("else if", 2, $4, $6); }
        ;

    else_clause: ELSE stmt  { $$ = new_node("else", 1, $2); }
        | ELSE code_block   { $$ = new_node("else", 1, $2); }
        ;

    if_clause: IF LP bool_expr RP stmt %prec LOWER_FAKE_MARK     { $$ = new_node("if", 2, $3, $5); }
        | IF LP bool_expr RP code_block %prec LOWER_FAKE_MARK    { $$ = new_node("if", 2, $3, $5); }
        ;

    code_block: LCB stmts RCB { $$ = $2; $$->node_type = "code block"; }
        | LCB RCB             { $$ = new_node("code block", 0); }
        ;

    stmts: stmt         { $$ = new_node("statement list", 1, $1); }
        | stmt stmts    { $$ = l_merge_node($2, $1); }
        ;

    stmt: sentence SEMICOLON    { $$ = $1; }
        | SEMICOLON             { $$ = new_node("empty statement", 0); }
        | if_stmt               { $$ = $1; }
        | for_stmt              { $$ = $1; }
        | while_stmt            { $$ = $1; }
        | do_while_stmt         { $$ = $1; }
        | return_stmt           { $$ = $1; }
        ;

    sentence: expr      { $$ = $1; }
        | declare       { $$ = $1; }
        | decl_struct   { $$ = $1; }
        | bool_expr     { $$ = $1; }
        ;

    declare: type declare_clause        { $$ = new_node("declare", 2, $1, $2); }
        | declare COMMA declare_clause  { $$ = merge_node($1, $3); }
        ;

    declare_clause: ID                          { $$ = new_node("declare clause", 1, $1); }
        | array_declare                         { $$ = new_node("declare clause", 1, $1); }
        | pointer_declare                       { $$ = new_node("declare clause", 1, $1); }
        | ID ASSIGN initialize_expr             { $$ = new_node("declare clause", 2, $1,
                                                    new_node($2->node_type, 2, new_node_expr("expr_id", 1, new_value("id", strdup($1->value))), $3)); }
        | pointer_declare ASSIGN expr           { $$ = new_node("declare clause", 3, $1, $2, $3); }
        | array_declare ASSIGN initialize_expr  { $$ = new_node("declare clause", 3, $1, $2, $3); }
        ;

    pointer_declare: MUL ID     { $$ = new_node("pointer", 2, $1, $2); }
        | MUL pointer_declare   { $$ = l_merge_node($2, $1); }
        ;

    array_declare: ID LB expr RB    { $$ = new_node("array", 2, $1, new_node("length", 1, $3)); }
        | ID LB RB                  { $$ = new_node("array", 2, $1, new_node("length", 1, new_node("unknow", 0))); }
        | array_declare LB expr RB  { $$ = merge_node($1, new_node("length", 1, $3)); }
        | array_declare LB RB       { $$ = merge_node($1, new_node("length", 1, new_node("unknow", 0))); }
        ;

    initialize_expr: expr               { $$ = $1; }
        | LCB RCB                       { $$ = new_node("initialize list", 0); }
        | LCB initialize_list RCB       { $$ = $2; }
        | LCB initialize_list COMMA RCB { $$ = $2; }
        ;

    initialize_list: initialize_expr            { $$ = new_node("initialize list", 1, $1); }
        | initialize_list COMMA initialize_expr { $$ = merge_node($1, $3); }
        ;

    expr: const                                 { $$ = new_node_expr("expr_const", 1, $1); }
        | ID                                    { $$ = new_node_expr("expr_id", 1, $1); }
        | call_func                             { $$ = new_node("expression", 1, $1); }
        | dereference                           { $$ = new_node("expression", 1, $1); }
        | member_selection                      { $$ = new_node("expression", 1, $1); }
        | LP expr RP                            { $$ = $2; }
        | expr subscript                        { $$ = new_node("expression", 2, $1, $2); }
        | left_unary_operator expr %prec NOT    { $$ = new_node_expr($1->node_type, 1 ,$2); }
        | expr right_unary_operator %prec NOT   { $$ = new_node_expr($2->node_type, 1 ,$1); }
        | expr ADD expr                         { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr SUB expr                         { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr MUL expr                         { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr DIV expr                         { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr MOD expr                         { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr ADD_ASSIGN expr                  { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr SUB_ASSIGN expr                  { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr MUL_ASSIGN expr                  { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr DIV_ASSIGN expr                  { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr MOD_ASSIGN expr                  { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr ASSIGN expr                      { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        ;
    
    bool_expr: expr EQ expr                     { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr GT expr                          { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr LT expr                          { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr GE expr                          { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr LE expr                          { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | expr NE expr                          { $$ = new_node_expr($2->node_type, 2, $1, $3); }
        | bool_expr AND bool_expr               { $$ = new_node_bool($2->node_type, 2, $1, $3); }
        | bool_expr OR bool_expr                { $$ = new_node_bool($2->node_type, 2, $1, $3); }
        | NOT bool_expr                         { $$ = new_node_bool($1->node_type, 1 ,$2); }
        | LP bool_expr RP                       { $$ = $2; }
        ;

    member_selection: expr DOT ID   { $$ = new_node("member selection", 3, $1, $2, $3); }
        | expr ARROW ID             { $$ = new_node("member selection", 3, $1, $2, $3); }
        ;

    subscript: LB expr RB   { $$ = new_node("subscript", 1, $2); }
        ;

    dereference: MUL expr   { $$ = new_node("dereference", 2, $1, $2); }
        ;

    right_unary_operator: AUTO_INCR { $$ = $1; }
        | AUTO_DECR                 { $$ = $1; }
        ;

    left_unary_operator: REF        { $$ = $1; }
        | AUTO_INCR                 { $$ = $1; }
        | AUTO_DECR                 { $$ = $1; }
        | SUB                       { $$ = new_node_expr("minus", 1, $1); }
        ;

    const: INUM     { $$ = $1; }
        | FNUM      { $$ = $1; }
        | DNUM      { $$ = $1; }
        | CHARACTER { $$ = $1; }
        | STRING    { $$ = $1; }
        ;

    type: INT       { $$ = $1; }
        | FLOAT     { $$ = $1; }
        | DOUBLE    { $$ = $1; }
        | CHAR      { $$ = $1; }
        | VOID      { $$ = $1; }
        | STRUCT    { $$ = $1; }
        ;

%%
