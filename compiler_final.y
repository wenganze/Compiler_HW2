/* Definition section */
%{
    #include "compiler_common.h"
    #include "compiler_util.h"
    #include "main.h"

    int yydebug = 1;

    ObjectType currentVarType;
    int array_size= 0;
%}

/* Variable or self-defined structure */
%union {
    ObjectType var_type;

    bool b_var;
    int i_var;
    float f_var;
    char *s_var;

    Object object_val;
}

/* Token without return */
%token COUT
%token SHR SHL BAN BOR BNT BXO ADD SUB MUL DIV REM NOT GTR LES GEQ LEQ EQL NEQ LAN LOR
%token VAL_ASSIGN ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN REM_ASSIGN BAN_ASSIGN BOR_ASSIGN BXO_ASSIGN SHR_ASSIGN SHL_ASSIGN INC_ASSIGN DEC_ASSIGN
%token IF ELSE FOR WHILE RETURN BREAK CONTINUE

/* Token with return, which need to sepcify type */
%token <var_type> VARIABLE_T
%token <s_var> IDENT
%token <s_var> STR_LIT
%token <i_var> INT_LIT
%token <f_var> FLOAT_LIT
%token <b_var> BOOL_LIT

/* Nonterminal with return, which need to sepcify type */
%type <object_val> Expression
%type <s_var> VariableName
%type <s_var> FunctionCreateStmt
%type <s_var> IterationVariableName

%right VAL_ASSIGN ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN REM_ASSIGN SHR_ASSIGN SHL_ASSIGN BAN_ASSIGN BOR_ASSIGN BXO_ASSIGN 
%left LOR
%left LAN
%left BOR
%left BXO
%left BAN
%left EQL NEQ
%left GTR LES GEQ LEQ
%left SHR SHL
%left ADD SUB
%left MUL DIV REM
%right NEG NOT BNT CAST
%left INC_ASSIGN DEC_ASSIGN

/* Yacc will start at this nonterminal */
%start Program

%%
/* Grammar section */

Program
    : { pushScope(); } GlobalStmtList { dumpScope(); }
    | /* Empty file */
;

GlobalStmtList 
    : GlobalStmtList GlobalStmt
    | GlobalStmt
;

GlobalStmt
    : DefineVariableStmt
    | FunctionDefStmt
;

DefineVariableStmt
    : VARIABLE_T { currentVarType = $<var_type>1; } VariableList ';'
;

VariableList
    : Variable ',' VariableList
    | Variable
;

Variable
    : VariableName { createVariable(currentVarType, $<s_var>1, 0); }
    | VariableName VAL_ASSIGN Expression { if(currentVarType != OBJECT_TYPE_AUTO)createVariable(currentVarType, $<s_var>1, 0);else createAutoVariable(currentVarType, $<s_var>1, &$<object_val>3); }
    | IterationVariableName ':' Expression { if(currentVarType != OBJECT_TYPE_AUTO)createIterationVariable(currentVarType, $<s_var>1, 0);else createAutoIterationVariable(currentVarType, $<s_var>1, &$<object_val>3); } 
;

IterationVariableName
    : IDENT { $$ = $<s_var>1; printIterationVariable($<s_var>1); }
;

VariableName
    : VariableName '[' INT_LIT { printf("INT_LIT %d\n", $<i_var>3); } ']' { $$ = $<s_var>1; }
    | VariableName '[' ']' { $$ = $<s_var>1; }
    | IDENT { $$ = $<s_var>1; }

/* Function */
FunctionDefStmt
    : FunctionCreateStmt '(' FunctionParameterStmtList ')' { addReturnType($<var_type>1); } '{' StmtList '}' { dumpScope(); }
;

FunctionParameterStmtList 
    : FunctionParameterStmtList ',' FunctionParameterStmt
    | FunctionParameterStmt
;

FunctionParameterStmt
    : VARIABLE_T IDENT { pushFunParm($<var_type>1, $<s_var>2, VAR_FLAG_DEFAULT); }
    | VARIABLE_T IDENT '[' INT_LIT ']' { pushFunParm($<var_type>1, $<s_var>2, $<i_var>3); }
    | VARIABLE_T IDENT '[' ']' { pushFunParm($<var_type>1, $<s_var>2, -1); }
;

FunctionCreateStmt
    : VARIABLE_T IDENT { createFunction($<var_type>1, $<s_var>2); $$ = $<s_var>1; }

Expression
    : '(' Expression ')' { $$ = $<object_val>2; }
    | '(' VARIABLE_T ')' Expression %prec CAST { objectCast($<var_type>2, &$<object_val>4, &$$); }
    | { array_size=0; } '{' InputExprList '}' { printf("create array: %d\n", array_size); }
    | IDENT '(' InputExprList ')' { printf("IDENT (name=%s, address=-1)\n", $<s_var>1); Object *function = findVariable($<s_var>1); if(function != NULL){  $$ = (Object){ .type=funcReturn(function), .symbol=function->symbol};printFunc(function); } }
    | Expression ADD Expression { if(objectExpBinary('+', &$<object_val>1, &$<object_val>3, &$$))printf("ADD\n"); }
    | Expression SUB Expression { if(objectExpBinary('-', &$<object_val>1, &$<object_val>3, &$$))printf("SUB\n"); }
    | Expression MUL Expression { if(objectExpBinary('*', &$<object_val>1, &$<object_val>3, &$$))printf("MUL\n"); }
    | Expression DIV Expression { if(objectExpBinary('/', &$<object_val>1, &$<object_val>3, &$$))printf("DIV\n"); }
    | Expression REM Expression { if(objectExpBinary('%', &$<object_val>1, &$<object_val>3, &$$))printf("REM\n"); }
    | Expression GTR Expression { if(objectExpBoolean('>', &$<object_val>1, &$<object_val>3, &$$))printf("GTR\n"); }
    | Expression LES Expression { if(objectExpBoolean('<', &$<object_val>1, &$<object_val>3, &$$))printf("LES\n"); }
    | Expression GEQ Expression { if(objectExpBoolean('>=', &$<object_val>1, &$<object_val>3, &$$))printf("GEQ\n"); }
    | Expression LEQ Expression { if(objectExpBoolean('<=', &$<object_val>1, &$<object_val>3, &$$))printf("GTR\n"); }
    | Expression EQL Expression { if(objectExpBoolean('==', &$<object_val>1, &$<object_val>3, &$$))printf("EQL\n"); }
    | Expression NEQ Expression { if(objectExpBoolean('!=', &$<object_val>1, &$<object_val>3, &$$))printf("NEQ\n"); }
    | Expression LAN Expression { if(objectExpBoolean('&&', &$<object_val>1, &$<object_val>3, &$$))printf("LAN\n"); }
    | Expression LOR Expression { if(objectExpBoolean('||', &$<object_val>1, &$<object_val>3, &$$))printf("LOR\n"); }
    | Expression VAL_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$) || objectExpAssign('=' ,&$<object_val>1, &$<object_val>3, &$$) )printf("EQL_ASSIGN\n"); }
    | Expression ADD_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("ADD_ASSIGN\n"); }
    | Expression SUB_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("SUB_ASSIGN\n"); }
    | Expression MUL_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("MUL_ASSIGN\n"); }
    | Expression DIV_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("DIV_ASSIGN\n"); }
    | Expression REM_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("REM_ASSIGN\n"); }
    | Expression SHR_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("SHR_ASSIGN\n"); }
    | Expression SHL_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("SHL_ASSIGN\n"); }
    | Expression BAN_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("BAN_ASSIGN\n"); }
    | Expression BOR_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("BOR_ASSIGN\n"); }
    | Expression BXO_ASSIGN Expression { if(objectValueAssign( &$<object_val>1, &$<object_val>3, &$$))printf("BXO_ASSIGN\n"); }
    | Expression SHR Expression { if(objectExpression('>>', &$<object_val>1,  &$<object_val>3, &$$))printf("SHR\n"); }
    | Expression SHL Expression { if(objectExpression('<<', &$<object_val>1,  &$<object_val>3, &$$))printf("SHL\n"); }
    | Expression BAN Expression { if(objectExpression('&', &$<object_val>1,  &$<object_val>3, &$$))printf("BAN\n"); }
    | Expression BOR Expression { if(objectExpression('|', &$<object_val>1,  &$<object_val>3, &$$))printf("BOR\n"); }
    | Expression BXO Expression { if(objectExpression('^', &$<object_val>1,  &$<object_val>3, &$$))printf("BXO\n"); }
    | Expression INC_ASSIGN { if(objectIncAssign(&$<object_val>1, &$$))printf("INC_ASSIGN\n"); }
    | Expression DEC_ASSIGN { if(objectDecAssign(&$<object_val>1, &$$))printf("DEC_ASSIGN\n"); }
    | SUB Expression %prec NEG { if(objectNegExpression( &$<object_val>2, &$$))printf("NEG\n"); }
    | NOT Expression { if(objectNotBinaryExpression( &$<object_val>2, &$$))printf("NOT\n"); }
    | BNT Expression { if(objectNotExpression( &$<object_val>2, &$$ ))printf("BNT\n"); }
    | STR_LIT { printf("STR_LIT \"%s\"\n", $<s_var>1); $$ = (Object){ .type=OBJECT_TYPE_STR, .symbol=NULL};  }
    | INT_LIT { printf("INT_LIT %d\n", $<i_var>1); $$ = (Object){ .type=OBJECT_TYPE_INT, .value=$<i_var>1, .symbol=NULL}; }
    | FLOAT_LIT { printf("FLOAT_LIT %f\n", $<f_var>1); $$ = (Object){ .type=OBJECT_TYPE_FLOAT, .value=$<f_var>1, .symbol=NULL }; }
    | BOOL_LIT { printf("BOOL_LIT %s\n", $<b_var>1>0?"TRUE":"FALSE"); $$ = (Object){ .type=OBJECT_TYPE_BOOL, .value=$<b_var>1, .symbol=NULL }; }
    | IDENT  ArrayList { 
        if(strcmp($<s_var>1, "endl") == 0){ printf("IDENT (name=%s, address=-1)\n", $<s_var>1); $$ = (Object){ .type=OBJECT_TYPE_STR, .symbol=NULL}; }
        else{
            Object* object = findVariable($<s_var>1);
            if(object){
                printf("IDENT (name=%s, address=%ld)\n", $<s_var>1, object->symbol->addr);
                $$ = (Object){ .type=object->type, .symbol=object->symbol};
            }
        }
    }
    | BREAK { printf("BREAK\n"); }
    | CONTINUE { printf("CONTINUE\n"); } 
;

InputExprList
    : InputNumExpr ',' InputExprList { array_size++; }
    | InputNumExpr { array_size++; }
    |
;

InputNumExpr
    : SUB InputExpr { printf("NEG\n"); }
    | InputExpr

ArrayList
    : Array ArrayList
    | Array
;

Array
    : '['InputExpr']'
    |
;

InputExpr
    : INT_LIT { printf("INT_LIT %d\n", $<i_var>1); }
    | FLOAT_LIT { printf("FLOAT_LIT %f\n", $<f_var>1); }
    | STR_LIT { printf("STR_LIT \"%s\"\n", $<s_var>1); }
    | BOOL_LIT { printf("BOOL_LIT %s\n", $<b_var>1>0?"TRUE":"FALSE"); }
    | IDENT { 
        if(strcmp($<s_var>1, "endl") == 0){ printf("IDENT (name=%s, address=-1)\n", $<s_var>1); }
        else{
            Object *object = findVariable($<s_var>1);
            if(object){
                printf("IDENT (name=%s, address=%ld)\n", $<s_var>1, object->symbol->addr);
            }
        }
    }
;

IfStmt
    : IF '(' Expression ')' {  printf("IF\n");pushScope(); } '{' StmtList '}' { dumpScope(); }
    | IF '(' Expression ')' {  printf("IF\n"); } Stmt
    | ELSE { printf("ELSE\n");pushScope(); } '{' StmtList '}' { dumpScope(); }
;

WhileStmt
    : WHILE { printf("WHILE\n"); } '(' Expression ')' { pushScope(); } '{' StmtList '}' { dumpScope(); }
;

ForStmt
    : FOR { printf("FOR\n"); pushScope(); } '(' ForDeclare ')' '{' StmtList '}' { dumpScope(); }
;

ForDeclare
    : ForDefStmt ';' ForExpr ';' ForExpr
    | ForDefStmt

ForExpr
    :   Expression
    |
;

ForDefStmt
    :   VARIABLE_T { currentVarType = $<var_type>1; } VariableList
    |
;

/* Scope */
StmtList 
    : StmtList Stmt
    | Stmt
;
Stmt
    : Expression ';'
    | IfStmt
    | WhileStmt
    | ForStmt
    | DefineVariableStmt 
    | { stdoutClear(); } COUT CoutParmListStmt ';' { stdoutPrint(); }
    | RETURN Expression ';' { printf("RETURN\n"); }
    | RETURN ';' { printf("RETURN\n"); }
;

CoutParmListStmt
    : CoutParmListStmt SHL Expression { pushFunInParm(&$<object_val>3); }
    | SHL Expression { pushFunInParm(&$<object_val>2); }
;

%%
/* C code section */
