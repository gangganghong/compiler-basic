#ifndef OPINFO
#error "You must define OPINFO macro before include this file"
#endif 

OPINFO(OP_ASSIGN,-1, "=",      Assignment,     NOP)
OPINFO(OP_ADD_ASSIGN,-1,"+=",      Assignment,     NOP)
OPINFO(OP_MINUS_ASSIGN,-1,"-=",      Assignment,     NOP)
OPINFO(OP_OR_ASSIGN,-1, "|=",      Assignment,     NOP)
OPINFO(OP_XOR_ASSIGN,-1, "^=",      Assignment,     NOP)
OPINFO(OP_MUL_ASSIGN,-1,"*=",      Assignment,     NOP)
OPINFO(OP_DIV_ASSIGN,-1,"/=",      Assignment,     NOP)
OPINFO(OP_MOD_ASSIGN,-1,"%=",      Assignment,     NOP)
OPINFO(OP_LEFT_SHIFT_ASSIGN,-1, "<<=",      Assignment,     NOP)
OPINFO(OP_RIGHT_SHIFT_ASSIGN,-1, ">>=",      Assignment,     NOP)
OPINFO(OP_AND_ASSIGN,-1, "&=",      Assignment,     NOP)
OPINFO(OP_BIT_CLEAR_ASSIGN,-1,"&^=",      Assignment,     NOP)
OPINFO(OP_INIT_ASSIGN,-1, ":=",      Assignment,     NOP)
OPINFO(OP_CONDITIONAL_OR,1,"||",      Assignment,     NOP)
OPINFO(OP_CONDITIONAL_AND,2,"&&",      Assignment,     NOP)
OPINFO(OP_EQUAL,3,"==",      Assignment,     NOP)
OPINFO(OP_NOT_EQUAL,3,"!=",      Assignment,     NOP)
OPINFO(OP_LESS,3,"<",      Assignment,     NOP)
OPINFO(OP_LESS_OR_EQUAL,3,"<=",      Assignment,     NOP)
OPINFO(OP_GREATER,3,">",      Assignment,     NOP)
OPINFO(OP_GREATER_OR_EQUAL,3,">=",      Assignment,     NOP)
OPINFO(OP_ADD,4,"+",      Assignment,     NOP)
OPINFO(OP_MINUS,4,"-",      Assignment,     NOP)
OPINFO(OP_BINARY_BITWISE_OR,4,"|",      Assignment,     NOP)
OPINFO(OP_BINARY_BITWISE_XOR,4,"^",      Assignment,     NOP)
OPINFO(OP_MUL,5,"*",      Assignment,     NOP)
OPINFO(OP_DIV,5,"/",      Assignment,     NOP)
OPINFO(OP_MOD,5,"%",      Assignment,     NOP)
OPINFO(OP_LEFT_SHIFT,5,"<<",      Assignment,     NOP)
OPINFO(OP_RIGHT_SHIFT,5,">>",      Assignment,     NOP)
OPINFO(OP_BITWISE_AND,5,"&",      Assignment,     NOP)
OPINFO(OP_BITWISE_AND_NOT,5,"&^",      Assignment,     NOP)
OPINFO(OP_SEND,-1, "<-",      Assignment,     NOP)


OPINFO(OP_POS,6,"+",      Assignment,     NOP)
OPINFO(OP_NEG,6,"-",      Assignment,     NOP)
OPINFO(OP_BITWISE_XOR,6,"^",      Assignment,     NOP)
OPINFO(OP_DEREF,6,"*",      Assignment,     NOP)
OPINFO(OP_NOT,6, "!",      Assignment,     NOP)
OPINFO(OP_INC,7, "++",      Assignment,     NOP)
OPINFO(OP_DEC,7, "--",      Assignment,     NOP)
// TODO 不知道优先级
OPINFO(OP_RECEIVE,8, "<-",      Assignment,     NOP)

// 新加的运算符。不知道放到哪里合适，先放在这里。start
OPINFO(OP_INDEX,          17,   "nop",    NOP,          NOP)
OPINFO(OP_ID,          17,   "nop",    NOP,          NOP)

// 新加的运算符。不知道放到哪里合适，先放在这里。end

OPINFO(OP_NONE,          17,   "nop",    Error,          NOP)
