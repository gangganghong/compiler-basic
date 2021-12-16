#include "lex.h"
#include "ast.h"
#include "decl.h"
#include "expr.h"

/**
 * ExpressionList = Expression { "," Expression } .
 *
 */
AstExpression ParseExpressionList(){
	LOG("%s\n", "parse ExpressionList");

	// todo 创建一个链表，竟然还要想比较长时间。
	AstExpression expr;
	CREATE_AST_NODE(expr, Expression);
	AstExpression curExpr;
	CREATE_AST_NODE(curExpr, Expression); 
	AstExpression preExpr;
	CREATE_AST_NODE(preExpr, Expression); 
	
	int count = 0;
	expr = ParseExpression();
	// todo 有问题吗？我希望的效果是：解析出一个表达式后就统计一次次数。
	count++;
	preExpr = expr;
	while(current_token.kind==TK_COMMA){
		NEXT_TOKEN;
//		memset(curExpr, 0, sizeof(*curExpr));
		curExpr = ParseExpression();
		curExpr->next = NULL;
		count++;
		preExpr->next = (AstNode)curExpr;
		preExpr = curExpr;
	}
	// preExpr->next = NULL;
	
	expr->variable_count = count;

	return expr;
}

/**
 * Expression = UnaryExpr | Expression binary_op Expression .
UnaryExpr  = PrimaryExpr | unary_op UnaryExpr .

binary_op  = "||" | "&&" | rel_op | add_op | mul_op .
rel_op     = "==" | "!=" | "<" | "<=" | ">" | ">=" .
add_op     = "+" | "-" | "|" | "^" .
mul_op     = "*" | "/" | "%" | "<<" | ">>" | "&" | "&^" .

unary_op   = "+" | "-" | "!" | "^" | "*" | "&" | "<-" .

产生式的文档在：https://golang.org/ref/spec#Operators

 */
AstExpression ParseExpression(){
	LOG("%s\n", "parse Expression");
//	ParseUnaryExpr() || ParseBinaryExpr();
//	if(TK_POSITIVE <= current_token.kind && current_token.kind <= TK_RECEIVE){
//	if(IsUnaryOp() == 1){
//		ParseUnaryExpr();
//	}else{
//		Token pre_token = current_token;
//		
//
//		ParseBinaryExpr();
//
//	}
	AstExpression expr;
//	CREATE_AST_NODE(expr, Expression);
	// expr = ParseBinaryExpr(4);
	// todo 参数prec如何确定？
//	expr = ParseBinaryExpr(2);
	// expr = ParseBinaryExpr(7);
	// OP_CONDITIONAL_OR 是优先级最低的二元运算符号。
	expr = ParseBinaryExpr(Prec[OP_CONDITIONAL_OR]);
//	expr = ParseBinaryExpr(4);

	return expr;
}

// todo 这个函数难理解。
AstExpression ParseBinaryExpr(int prec){
	LOG("%s\n", "parse BinaryExpr");
//	ParseExpression();
//	// todo 如何处理binary_op？
//	ParseBinaryOp();
//	ParseExpression();
	// #define HIGHEST_BIN_PREC Prec[TK_MUL - TK_CONDITIONAL_OR]	
	// OP_MUL是优先级最高的二元运算符号。
	#define HIGHEST_BIN_PREC Prec[OP_MUL]	

	AstExpression binExpr;
	AstExpression expr;

	CREATE_AST_NODE(binExpr, Expression);
	CREATE_AST_NODE(expr, Expression);

	//dump_token_number();	

	if(prec == HIGHEST_BIN_PREC){
		expr = ParseUnaryExpr();
	}else{
	//	binExpr->op = BINARY_OP;
	//	binExpr->kids[0] = expr;
	//	NEXT_TOKEN;
	//	binExpr->kids[1] = ParseBinaryExpr(prec + 1);
	//	expr = binExpr;
		ParseBinaryExpr(prec + 1);
	}

	while(IsBinaryOp() == 1 && Prec[BINARY] == prec){
		NEXT_TOKEN;
		if(prec == HIGHEST_BIN_PREC){
			expr = ParseUnaryExpr();
		}else{
			binExpr->op = BINARY_OP;
			binExpr->kids[0] = expr;
//			NEXT_TOKEN;
			binExpr->kids[1] = ParseBinaryExpr(prec + 1);
			expr = binExpr;
		}
	}

	return expr;
}
/**
 * UnaryExpr  = PrimaryExpr | unary_op UnaryExpr .
 * unary_op   = "+" | "-" | "!" | "^" | "*" | "&" | "<-" .
 */
AstExpression ParseUnaryExpr(){
	//dump_token_number();	

//	LOG("%s\n", "parse UnaryExpr");
//	AstExpression expr;	
//	CREATE_AST_NODE(expr, Expression);
//	// if(TK_POSITIVE <= current_token.kind && current_token.kind <= TK_RECEIVE){
//	// TODO IsUnaryOp 需要重构。有可能根本就不正确。
//	// if(IsUnaryOp() == 1){
//	char flag = 0;
//	if(flag == 1){
//		expr->op = UNARY_OP;
//		NEXT_TOKEN;
//		expr->kids[0] = ParseUnaryExpr();
//	}else{
//		expr = ParsePrimaryExpr();
//	}

	LOG("%s\n", "parse UnaryExpr");
	AstExpression expr;	
	CREATE_AST_NODE(expr, Expression);

	switch (current_token.kind){
		case TK_ADD:			// +
		case TK_MINUS:			// -
		case TK_NOT:			// !
		case TK_BINARY_BITWISE_XOR:	// ^
		case TK_MUL:			// *
		case TK_BITWISE_AND:		// &
		case TK_RECEIVE:		// <-
			expr->op = UNARY_OP;
			NEXT_TOKEN;
			expr->kids[0] = ParseUnaryExpr();
			break;
		default:
			expr = ParsePrimaryExpr();
	}
	return expr;
}

AstNode ParseConversion(){
	LOG("%s\n", "parse Conversion");
	NEXT_TOKEN;
	expect_token(TK_LPARENTHESES);
	ParseExpression();
	if(current_token.kind == TK_COMMA) expect_token(TK_COMMA);
	expect_token(TK_RPARENTHESES);
}

// todo 暂时不处理后缀表达式。
int IsPostfix(TokenKind kind){
//	return 0;
	return (kind == TK_DOT || kind == TK_LBRACKET || kind == TK_LPARENTHESES);
	// return (kind == TK_DOT || kind == TK_LBRACKET);
} 

/**
 * Selector       = "." identifier .
 * TypeAssertion  = "." "(" Type ")" .
 */
AstNode ParseSelectorTypeAssertion(){
	NEXT_TOKEN;
	if(current_token.kind == TK_LPARENTHESES){
		NEXT_TOKEN;
		ExpectDataType();
		expect_token(TK_RPARENTHESES);
	}else{
		expect_token(TK_ID);
	}
}

/**
 * Arguments      = "(" [ ( ExpressionList | Type [ "," ExpressionList ] ) [ "..." ] [ "," ] ] ")" .
 * ()
 * (abc, def)
 * (uint abc, def,name)
 * (uint abc, def,name) ...
 */
AstNode ParseArguments(){
	expect_token(TK_LPARENTHESES);
	
	while(current_token.kind != TK_RPARENTHESES){
		if(IsDataType(current_token.value.value_str) == 1){
			NEXT_TOKEN;
		}else{
			ParseExpressionList();
		}

		// 第一个循环
		while(current_token.kind == TK_COMMA){
			NEXT_TOKEN;
			ParseExpressionList();
		}

		// 第二个循环
		while(current_token.kind == TK_ELLIPSIS){
			NEXT_TOKEN;
		}

		// 第三个循环
		while(current_token.kind == TK_COMMA){
			NEXT_TOKEN;
		}
	}	

	expect_token(TK_RPARENTHESES);
}

/**
 * Index          = "[" Expression "]" .
Slice          = "[" [ Expression ] ":" [ Expression ] "]" |
                 "[" [ Expression ] ":" Expression ":" Expression "]" .
 */
AstNode ParseIndexSlice(){
	NEXT_TOKEN;
	
	int expr_count = 0;
	// TokenKind kind = current_token.kind;	
	TokenKind kind  = current_token.kind;	
	while(kind != TK_RBRACKET && kind != TK_COLON){ 	
		NO_TOKEN;
		ParseExpression();
		kind  = current_token.kind;
		expr_count++;
	}

	// if(expr_count == 1 && kind == TK_RBRACKET){
	if(kind == TK_RBRACKET){
		printf("%s\n", "It is a Index");
		expect_token(TK_RBRACKET);
		 // NEXT_TOKEN;
	//	expect_token();

	}else if(kind == TK_COLON){
		printf("%s\n", "It is a Slice");
		NEXT_TOKEN;

		int expr_count = 0;
		kind = current_token.kind;	
		// while(kind != TK_COLON){ 	
		while(kind != TK_RBRACKET && kind != TK_COLON){ 	
			NO_TOKEN;
			ParseExpression();
			kind = current_token.kind;	
			expr_count++;
		}

		if(kind == TK_COLON){
			NEXT_TOKEN;
			ParseExpression();
			// expect_token(TK_RBRACKE);
		}else{
			// NEXT_TOKEN;
			// todo 可以去掉这个else
		}
		expect_token(TK_RBRACKET);
	}else{
		ERROR("ParseIndexSlice error\n");
	}
}

/**
 * PrimaryExpr =
	Operand |
	Conversion |
	MethodExpr |
	PrimaryExpr Selector |
	PrimaryExpr Index |
	PrimaryExpr Slice |
	PrimaryExpr TypeAssertion |
	PrimaryExpr Arguments .

Selector       = "." identifier .
Index          = "[" Expression "]" .
Slice          = "[" [ Expression ] ":" [ Expression ] "]" |
                 "[" [ Expression ] ":" Expression ":" Expression "]" .
TypeAssertion  = "." "(" Type ")" .
Arguments      = "(" [ ( ExpressionList | Type [ "," ExpressionList ] ) [ "..." ] [ "," ] ] ")" .
 */
AstExpression ParsePrimaryExpr(){
	LOG("%s\n", "parse PrimaryExpr");

//	//dump_token_number();	

	AstExpression expr;	
	CREATE_AST_NODE(expr, Expression);
	if(IsDataType(current_token.value.value_str) == 1){
		ParseConversion();
	}else{
//		NEXT_TOKEN;
		// todo 解析 Operand、MethodExpr。目前，只解析Operand。
		expr = ParseOperand();
	}

	while(IsPostfix(current_token.kind)){
		switch(current_token.kind){
			case TK_DOT:
				ParseSelectorTypeAssertion();
				break; 
			case TK_LPARENTHESES:
				ParseArguments();
				break; 
			case TK_LBRACKET:
				ParseIndexSlice();
				break; 
			default:
				break;
		}		
	}

	return expr;
}

AstNode ParseBinaryOp(){
	LOG("%s\n", "parse binary_op");
	// return (TK_CONDITIONAL_OR <= current_token.kind && current_token.kind <= TK_BITWISE_AND_NOT);
	if(TK_CONDITIONAL_AND <= current_token.kind && current_token.kind <= TK_BITWISE_AND_NOT){
		NEXT_TOKEN;
	}else{
		// TODO 这是啥意思？
//		ParseRelOp() || ParseAddOp() || ParseMulOp(); 
		ERROR("expect a binary op, but give %d\n", current_token.kind);
	}
}


AstNode ParseRelOp(){
	LOG("%s\n", "parse rel_op");
	if(TK_EQUAL <= current_token.kind && current_token.kind <= TK_GREATER_OR_EQUAL){
		NEXT_TOKEN;
	}else{
		ERROR("expect a real op, but give %d\n", current_token.kind);
	}
}


AstNode ParseAddOp(){
	LOG("%s\n", "parse add_op");
	if(TK_ADD <= current_token.kind && current_token.kind <= TK_BINARY_BITWISE_XOR){
		NEXT_TOKEN;
	}else{
		ERROR("expect a add op, but give %d\n", current_token.kind);
	}
}


AstNode ParseMulOp(){
	LOG("%s\n", "parse mul_op");
	if(TK_MUL <= current_token.kind && current_token.kind <= TK_BITWISE_AND_NOT){
		NEXT_TOKEN;
	}else{
		ERROR("expect a mul op, but give %d\n", current_token.kind);
	}
}

int IsRelOp(){
	return (TK_EQUAL <= current_token.kind && current_token.kind <= TK_GREATER_OR_EQUAL);
}

int IsAddOp(){
	return (TK_ADD <= current_token.kind && current_token.kind <= TK_BINARY_BITWISE_XOR);
}

int IsMulOp(){
	return (TK_MUL <= current_token.kind && current_token.kind <= TK_BITWISE_AND_NOT);
}

int IsUnaryOp(){
	return (TK_EQUAL <= current_token.kind && current_token.kind <= TK_GREATER_OR_EQUAL);
}

int IsBinaryOp(){
	return (TK_CONDITIONAL_OR <= current_token.kind && current_token.kind <= TK_BITWISE_AND_NOT);
//	return ((TK_CONDITIONAL_AND <= current_token.kind && current_token.kind <= TK_CONDITIONAL_OR)
//		|| (IsRelOp() == 1)
//		|| (IsAddOp() == 1)
//		|| (IsMulOp() == 1)
	//	|| current_token.kind == TK_POSITIVE
	//	|| current_token.kind == TK_NEGATIVE
	//	|| current_token.kind == TK_POINTER
	//	|| current_token.kind == TK_ADDRESS
//		);
}

/**
 * Operand     = Literal | OperandName | "(" Expression ")" .
Literal     = BasicLit | CompositeLit | FunctionLit .
BasicLit    = int_lit | float_lit | imaginary_lit | rune_lit | string_lit .
OperandName = identifier | QualifiedIdent .
 */
AstExpression ParseOperand(){
	// dump_token_number();	
	AstExpression expr;
	CREATE_AST_NODE(expr, Expression);
	if(current_token.kind == TK_NUM){
		expr = ParseLiteral();
	}else if(current_token.kind == TK_ID){
		expr = ParseOperandName();
	}
	return expr;
}

AstExpression ParseIntLit(){
	// todo 不完善。
//	// todo 应该使用malloc分配内存空间才更妥当吗？
	AstExpression expr;
	CREATE_AST_NODE(expr, Expression);
	if(current_token.kind == TK_NUM){
		expr = ParseBasicLit();
	}
	
	return expr;
}

AstExpression ParseBasicLit(){
	//dump_token_number();	
//	ParseIntLit();
//	// todo 应该使用malloc分配内存空间才更妥当吗？
	AstExpression expr;
	CREATE_AST_NODE(expr, Expression);
	if(current_token.kind == TK_NUM){
		// expr->op = OP_CONST;
		// TODO 这是不正确的。临时这样做。
		expr->op = OP_NONE;
		union value v = {current_token.value.value_num,0};
		expr->val = v;
		NEXT_TOKEN;
	}

	return expr;
}

AstExpression ParseLiteral(){

	//dump_token_number();	
	AstExpression expr;
	CREATE_AST_NODE(expr, Expression);
	expr = ParseBasicLit();

	return expr;
}

AstExpression ParseOperandName(){
	//dump_token_number();	
	AstExpression expr;
	CREATE_AST_NODE(expr, Expression);
	if(current_token.kind == TK_ID){
		// expr->op = OP_ID;
		// TODO 这是不正确的。临时这样做。
		expr->op = OP_NONE;
//		union value v = (void *)(current_token.value.value_str);
//		union value v.p = (void *)(current_token.value.value_str);
//		union value v;
//		v.p = (void *)(current_token.value.value_str);
//		union value v
//		memcpy(v, current_token.value.value_str, MAX_NAME_LEN);
//		*(expr->val.p) = *(current_token.value.value_str);
//		Program received signal SIGSEGV, Segmentation fault
//		*(char *)(expr->val.p) = *(current_token.value.value_str);
//
//		memcpy((char *)(expr->val.p), current_token.value.value_str, MAX_NAME_LEN);
//		union value *v = &(expr->val);
//		memcpy((char *)(v->p), current_token.value.value_str, MAX_NAME_LEN);
		expr->val.p = (void *)malloc(sizeof(char) * MAX_NAME_LEN);
		strcpy((char *)(expr->val.p), current_token.value.value_str);
		NEXT_TOKEN;
	}

	return expr;
}


// AstExpression ParseDecimalDigit(){
// 
// }
// AstExpression ParseDecimalDigits(){
// 
// 
// }
// AstExpression ParseDecimalLit(){
// 
// }
