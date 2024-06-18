#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define debug printf("%s:%d: ############### debug\n", __FILE__, __LINE__)

#define toupper(_char) (_char - (char)32)

const char* objectTypeName[] = {
    [OBJECT_TYPE_UNDEFINED] = "undefined",
    [OBJECT_TYPE_VOID] = "void",
    [OBJECT_TYPE_INT] = "int",
    [OBJECT_TYPE_FLOAT] = "float",
    [OBJECT_TYPE_BOOL] = "bool",
    [OBJECT_TYPE_STR] = "string",
    [OBJECT_TYPE_FUNCTION] = "function",
};

const char* funcSigName[] = {
    [OBJECT_TYPE_BOOL] = "B",
    [OBJECT_TYPE_DOUBLE] = "D",
    [OBJECT_TYPE_INT] = "I",
    [OBJECT_TYPE_FLOAT] = "F",
    [OBJECT_TYPE_LONG] = "L",
    [OBJECT_TYPE_STR] = "Ljava/lang/String;",
    [OBJECT_TYPE_VOID] = "V"
};

const ObjectType funcReutrnType[]={
    ['I'] = OBJECT_TYPE_INT
};

struct symbolTable{
    int scopeLevel;
    int size;
    Object *layer;
    struct symbolTable *nextLayer;
    struct symbolTable *preLayer;
};

char* yyInputFileName;
bool compileError;

int indent = 0;
int scopeLevel = -1;
int funcLineNo = 0;
int variableAddress = 0;
ObjectType variableIdentType;

struct symbolTable *nowSymbolTable = NULL;
char *coutString;

void appendToNowSymbolTable(struct symbolTable *nowSymbolTable, Object object){
    if(nowSymbolTable->layer == NULL)nowSymbolTable->layer = (Object*)malloc(sizeof(Object));
    else{
        Object *newlayer = (Object*)malloc((nowSymbolTable->size+1)*sizeof(Object));
        for(int i = 0;i < nowSymbolTable->size; i++)newlayer[i] = nowSymbolTable->layer[i];
        nowSymbolTable->layer = newlayer;
    }

    nowSymbolTable->layer[nowSymbolTable->size] = object;
    nowSymbolTable->size++;
}

void pushScope() {
    scopeLevel++;
    printf("> Create symbol table (scope level %d)\n", scopeLevel);

    if(nowSymbolTable == NULL){
        nowSymbolTable = (struct symbolTable*)malloc(sizeof(struct symbolTable));
        *nowSymbolTable = (struct symbolTable){
            .scopeLevel = scopeLevel,
            .size = 0,
            .layer = NULL,
            .preLayer = NULL,
            .nextLayer = NULL
        };
    }else{
        struct symbolTable *newSymbolTable = (struct symbolTable*)malloc(sizeof(struct symbolTable));
        *newSymbolTable = (struct symbolTable){
            .scopeLevel = scopeLevel,
            .size = 0,
            .layer = NULL,
            .preLayer = nowSymbolTable,
            .nextLayer = NULL
        };
        nowSymbolTable->nextLayer  = newSymbolTable;
        nowSymbolTable = newSymbolTable;
    }
}

ObjectType funcReturn(Object *func){
    if(func->symbol != NULL){
        if(func->symbol->func_sig != NULL){
            if(func->symbol->func_sig[strlen(func->symbol->func_sig)-1] == 'I'){
                return funcReutrnType['I'];
            }
        }
    }
}

void dumpScope() {
    printf("\n> Dump symbol table (scope level: %d)\n", scopeLevel);
    printf("Index     Name                Type      Addr      Lineno    Func_sig  \n");
    for(int i = 0;i < nowSymbolTable->size;i++){
        Object *object = &nowSymbolTable->layer[i];
        printf("%-10d%-20s%-10s%-10ld%-10d%-10s\n",
            object->symbol->index,
            object->symbol->name,
            objectTypeName[object->type],
            object->symbol->addr,
            object->symbol->lineno,
            object->type == OBJECT_TYPE_FUNCTION ? object->symbol->func_sig : "-"
        );
    }
    if(nowSymbolTable->preLayer != NULL){
        struct symbolTable *deletesymbolTable = nowSymbolTable;
        nowSymbolTable->preLayer->nextLayer = NULL;
        nowSymbolTable = nowSymbolTable->preLayer;
        free(deletesymbolTable);
    }else nowSymbolTable = NULL;
    scopeLevel--;
    
}

Object* createVariable(ObjectType variableType, char* variableName, int variableFlag) {
    printf("> Insert `%s` (addr: %d) to scope level %d\n", variableName, variableAddress, scopeLevel);

    Object variable = (Object){
        .type = variableType,
        .symbol = (SymbolData*)malloc(sizeof(SymbolData)),
        .value = variableFlag
    };
    *variable.symbol = (SymbolData){
        .addr = variableAddress,
        .name = variableName, 
        .lineno = yylineno,
        .index = nowSymbolTable->size,
    };

    appendToNowSymbolTable(nowSymbolTable, variable);
    variableAddress++;
    return &nowSymbolTable->layer[nowSymbolTable->size-1];
}

Object* createAutoVariable(ObjectType variableType, char* variableName, Object* variableFlag) {
    printf("> Insert `%s` (addr: %d) to scope level %d\n", variableName, variableAddress, scopeLevel);

    Object variable = (Object){
        .type = variableFlag->type,
        .symbol = (SymbolData*)malloc(sizeof(SymbolData)),
        .value = variableFlag
    };
    *variable.symbol = (SymbolData){
        .addr = variableAddress,
        .name = variableName, 
        .lineno = yylineno,
        .index = nowSymbolTable->size,
    };

    appendToNowSymbolTable(nowSymbolTable, variable);
    variableAddress++;
    return &nowSymbolTable->layer[nowSymbolTable->size-1];
}

Object* createIterationVariable(ObjectType variableType, char* variableName, int variableFlag) {
    Object variable = (Object){
        .type = variableType,
        .symbol = (SymbolData*)malloc(sizeof(SymbolData)),
        .value = variableFlag
    };
    *variable.symbol = (SymbolData){
        .addr = variableAddress,
        .name = variableName, 
        .lineno = yylineno,
        .index = nowSymbolTable->size,
    };

    appendToNowSymbolTable(nowSymbolTable, variable);
    variableAddress++;
    return &nowSymbolTable->layer[nowSymbolTable->size-1];
}

Object* createAutoIterationVariable(ObjectType variableType, char* variableName, Object* variableFlag) {
    Object variable = (Object){
        .type = variableFlag->type,
        .symbol = (SymbolData*)malloc(sizeof(SymbolData)),
        .value = variableFlag
    };
    *variable.symbol = (SymbolData){
        .addr = variableAddress,
        .name = variableName, 
        .lineno = yylineno,
        .index = nowSymbolTable->size,
    };

    appendToNowSymbolTable(nowSymbolTable, variable);
    variableAddress++;
    return &nowSymbolTable->layer[nowSymbolTable->size-1];
}

void printIterationVariable(char *variableName) {
    printf("> Insert `%s` (addr: %d) to scope level %d\n", variableName, variableAddress, scopeLevel);
}

void pushFunParm(ObjectType variableType, char* variableName, int variableFlag) {
    
    printf("> Insert `%s` (addr: %d) to scope level %d\n", variableName, variableAddress, scopeLevel);
    
    char *func_sig = nowSymbolTable->preLayer->layer[nowSymbolTable->preLayer->size-1].symbol->func_sig;
    size_t funcSigSize = strlen(func_sig) + strlen(funcSigName[variableType]) + 1;
    if(variableFlag == -1)funcSigSize++;
    char *new_func_sig = (char*)malloc(funcSigSize);
    strcpy(new_func_sig, func_sig);
    if(variableFlag == -1)strcat(new_func_sig,"[");
    strcat(new_func_sig, funcSigName[variableType]);
    nowSymbolTable->preLayer->layer[nowSymbolTable->preLayer->size-1].symbol->func_sig = new_func_sig;
    
    Object parm = (Object){
        .type = variableType,
        .symbol = (SymbolData*)malloc(sizeof(SymbolData))
    };
    *parm.symbol = (SymbolData){
        .addr = variableAddress,
        .name = variableName,
        .lineno = yylineno,
        .index = nowSymbolTable->size
    };
    appendToNowSymbolTable(nowSymbolTable, parm);
    variableAddress++;
    
}

void printFunc(Object *function){
    /*if(function->symbol != NULL) printf("good");
    if(function->symbol->name != NULL) printf("nameNotNULL");
    if(function->symbol->func_sig != NULL) printf("func_sigNotNULLL");*/
    printf("call: %s%s\n", function->symbol->name, function->symbol->func_sig);
}

void createFunction(ObjectType variableType, char* funcName) {
    
    printf("func: %s\n", funcName);
    printf("> Insert `%s` (addr: -1) to scope level %d\n", funcName, scopeLevel);

    char *func_sig = (char*)malloc(sizeof(char));
    strcpy(func_sig, "");

    Object funcInfo;
    funcInfo = (Object){
        .symbol=(SymbolData*)malloc(sizeof(SymbolData)),
        .type = OBJECT_TYPE_FUNCTION
    };
    *funcInfo.symbol = (SymbolData){
        .addr = -1,
        .name = funcName,
        .lineno = yylineno,
        .index = nowSymbolTable->size,
        .func_sig = func_sig
    };
    appendToNowSymbolTable(nowSymbolTable, funcInfo);

    scopeLevel++;
    struct symbolTable *newSymbolTable = (struct symbolTable*)malloc(sizeof(struct symbolTable));
    *newSymbolTable = (struct symbolTable){
        .size = 0,
        .scopeLevel = scopeLevel,
        .layer = NULL,
        .nextLayer = NULL,
        .preLayer = nowSymbolTable
    };
    nowSymbolTable->nextLayer = newSymbolTable;
    nowSymbolTable = newSymbolTable;

    printf("> Create symbol table (scope level %d)\n", scopeLevel);
}

void addReturnType(ObjectType func){
    char *func_sig = nowSymbolTable->preLayer->layer[nowSymbolTable->preLayer->size-1].symbol->func_sig;
    size_t funcSigSize = strlen(func_sig) + strlen(funcSigName[func]) + 3;
    char *new_func_sig = (char*)malloc(funcSigSize);
    strcpy(new_func_sig, "(");
    strcat(new_func_sig,func_sig);
    strcat(new_func_sig,")");
    strcat(new_func_sig, funcSigName[func]);
    nowSymbolTable->preLayer->layer[nowSymbolTable->preLayer->size-1].symbol->func_sig = new_func_sig;
}

void debugPrintInst(char instc, Object* a, Object* b, Object* out) {
}

bool objectExpression(char op, Object* dest, Object* val, Object* out) {
    if((dest->type == OBJECT_TYPE_INT || dest->type == OBJECT_TYPE_LONG || dest->type == OBJECT_TYPE_CHAR)&&
       (val->type == OBJECT_TYPE_INT || val->type == OBJECT_TYPE_CHAR || val->type == OBJECT_TYPE_LONG)){
        *out = (Object){
            .type = dest->type,
            .symbol = dest->symbol
        };
        if(op == '>>')out->value = dest->value >> val->value;
        if(op == '<<')out->value = dest->value << val->value;
        if(op == '&')out->value = dest->value & val->value;
        if(op == '|')out->value = dest->value | val->value;
        if(op == '^')out->value = dest->value ^ val->value;
        return true;
    }
    return false;
}

bool objectExpBinary(char op, Object* a, Object* b, Object* out) {
    if(op == '+' || op == '-' || op == '*' || op == '/'){
        if((a->type == OBJECT_TYPE_INT || a->type == OBJECT_TYPE_FLOAT || a->type == OBJECT_TYPE_BOOL || a->type == OBJECT_TYPE_DOUBLE) &&
           (b->type == OBJECT_TYPE_INT || b->type == OBJECT_TYPE_FLOAT || b->type == OBJECT_TYPE_BOOL || a->type == OBJECT_TYPE_DOUBLE)){
            if(a->type == OBJECT_TYPE_FLOAT || b->type == OBJECT_TYPE_FLOAT) *out = (Object){ .type=OBJECT_TYPE_FLOAT, .symbol=NULL};
            else *out = (Object){ .type=OBJECT_TYPE_INT, .symbol=NULL };
            return true;
        }
    }
    if(op == '%'){
        if((a->type == OBJECT_TYPE_INT ||b->type == OBJECT_TYPE_BOOL) && (a->type == OBJECT_TYPE_INT || b->type == OBJECT_TYPE_BOOL)){
            *out = (Object){ .type=OBJECT_TYPE_INT, .symbol=NULL };
            return true;
        }
    }
    return false;
}

bool objectExpBoolean(char op, Object* a, Object* b, Object* out) {
    if((a->type == OBJECT_TYPE_INT || a->type == OBJECT_TYPE_FLOAT || a->type == OBJECT_TYPE_BOOL || a->type == OBJECT_TYPE_DOUBLE) &&
        (b->type == OBJECT_TYPE_INT || b->type == OBJECT_TYPE_FLOAT || b->type == OBJECT_TYPE_BOOL || a->type == OBJECT_TYPE_DOUBLE)){
        if(op == '==') *out = (Object){ .type=OBJECT_TYPE_BOOL, .symbol=NULL, .value=(a->value==b->value) };
        if(op == '>=') *out = (Object){ .type=OBJECT_TYPE_BOOL, .symbol=NULL, .value=(a->value>=b->value) };
        if(op == '<=') *out = (Object){ .type=OBJECT_TYPE_BOOL, .symbol=NULL, .value=(a->value<=b->value) };
        if(op == '||') *out = (Object){ .type=OBJECT_TYPE_BOOL, .symbol=NULL, .value=(a->value||b->value) };
        if(op == '&&') *out = (Object){ .type=OBJECT_TYPE_BOOL, .symbol=NULL, .value=(a->value&&b->value) };  
        if(op == '!=') *out = (Object){ .type=OBJECT_TYPE_BOOL, .symbol=NULL, .value=(a->value!=b->value) };
        if(op == '>') *out = (Object){ .type=OBJECT_TYPE_BOOL, .symbol=NULL, .value=(a->value>b->value) };
        if(op == '<') *out = (Object){ .type=OBJECT_TYPE_BOOL, .symbol=NULL, .value=(a->value<b->value) };
        return true;
    }
    return false;
}

bool objectExpAssign(char op, Object* dest, Object* val, Object* out) {
    if(dest->type == OBJECT_TYPE_STR || dest->type == OBJECT_TYPE_BOOL){
        if(op == '='){
            *out = (Object){
                .type = OBJECT_TYPE_STR,
                .symbol = NULL
            };
            return true;
        }
    }
    return false;
}

bool objectValueAssign( Object* dest, Object* val, Object* out) {
    if(dest->type == OBJECT_TYPE_FLOAT || dest->type == OBJECT_TYPE_INT || dest->type == OBJECT_TYPE_DOUBLE){
        *out = (Object){
            .type = dest->type,
            .value = dest->value,
            .symbol = dest->symbol
        };
        return true;
    }
    return false;
}

bool objectNotBinaryExpression(Object* dest, Object* out) {
    if(dest->type == OBJECT_TYPE_BOOL){
        *out = (Object){ .type=dest->type, .value=!dest->value, .symbol=NULL };
        return true;
    }
    return false;
}

bool objectNegExpression(Object* dest, Object* out) {
    if(dest->type == OBJECT_TYPE_FLOAT || dest->type == OBJECT_TYPE_INT){
        *out = (Object){ .type=dest->type, .value=(-dest->value), .symbol=NULL };
        return true;
    }
    return false;
}
bool objectNotExpression(Object* dest, Object* out) {
    if(dest->type == OBJECT_TYPE_INT || dest->type == OBJECT_TYPE_LONG || dest->type == OBJECT_TYPE_CHAR){
        *out = (Object){
            .type = dest->type,
            .value = ~dest->value,
            .symbol = dest->symbol
        };
        return true;
    }
    return false;
}

bool objectIncAssign(Object* a, Object* out) {
    if(a->type == OBJECT_TYPE_INT || a->type == OBJECT_TYPE_LONG){
        *out = (Object){
            .value = a->value + 1,
            .symbol = a->symbol,
            .type = a->type
        };
        return true;
    }
    return false;
}

bool objectDecAssign(Object* a, Object* out) {
    if(a->type == OBJECT_TYPE_INT || a->type == OBJECT_TYPE_LONG){
        *out = (Object){
            .value = a->value - 1,
            .symbol = a->symbol,
            .type = a->type
        };
        return true;
    }
    return false;
}

bool objectCast(ObjectType variableType, Object* dest, Object* out) {
    *out = (Object){
        .type = variableType,
        .symbol = dest->symbol
    };
    if(variableIdentType == OBJECT_TYPE_INT) out->value = (int)dest->value;
    else if(variableIdentType == OBJECT_TYPE_LONG) out->value = (long)dest->value;
    else if(variableIdentType == OBJECT_TYPE_FLOAT) out->value = (float)dest->value;
    else if(variableIdentType == OBJECT_TYPE_DOUBLE) out->value = (double)dest->value;
    printf("Cast to %s\n", objectTypeName[variableType]);
    return true;
}

Object* findVariable(char* variableName) {
    struct symbolTable *temp = nowSymbolTable;
    while (temp != NULL)
    {
        for(int i = 0;i < temp->size; i++){
            if(temp->layer[i].symbol != NULL)if(strcmp(temp->layer[i].symbol->name, variableName) == 0)return &temp->layer[i];
        }
        temp = temp->preLayer;
    }
    return NULL;
}

void pushFunInParm(Object* variable) {
    if(coutString == NULL){
        coutString = malloc((strlen(objectTypeName[variable->type])+2)*sizeof(char));
        strcpy(coutString, objectTypeName[variable->type]);
    }else{
        char *newCoutString = malloc((strlen(objectTypeName[variable->type]) + strlen(coutString) + 2)*sizeof(char));
        strcpy(newCoutString, coutString);
        strcat(newCoutString, " ");
        strcat(newCoutString, objectTypeName[variable->type]);
        coutString = newCoutString;
    }
}

void stdoutPrint() {
    char *newCoutString = malloc( (strlen(coutString) + 5)*sizeof(char));
    strcpy(newCoutString, "cout ");
    strcat(newCoutString, coutString);
    printf("%s\n", newCoutString);
}

void stdoutClear(){
    if(coutString != NULL){
        free(coutString);
        coutString = NULL;
    }
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        yyin = fopen(yyInputFileName = argv[1], "r");
    } else {
        yyin = stdin;
    }
    if (!yyin) {
        printf("file `%s` doesn't exists or cannot be opened\n", yyInputFileName);
        exit(1);
    }

    // Start parsing
    yyparse();
    printf("Total lines: %d\n", yylineno);
    fclose(yyin);

    yylex_destroy();
    return 0;
}
