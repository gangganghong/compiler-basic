#include "lex.h"
#include "stmt.h"
#include "expr.h"
#include "decl.h"

int main(int argc, char *argv[])
{
	fp = fopen("test.c", "r");
	if(fp == NULL){
		perror("open test.c error");
		exit(-1);
	}

	NEXT_TOKEN;
	AstStmtNodePtr comNode = compound_stmt();

	return 0;
}

// todo 返回一个局部变量的值，有问题吗？
AstStmtNodePtr compound_stmt()
{
	AstStmtNodePtr comNode;
	AstStmtNodePtr *pNode;
//	NEXT_TOKEN;
	assert(current_token.kind == TK_LBRACE);
	// 希望是{
	EXPECT(TK_LBRACE);
	comNode = create_ast_stmt_node(TK_COMPOUND);
	pNode = &(comNode->next);	
	// 中间是其他语句，把它们放在一个单链表中。
	while(check_is_prefix_of_stmt(current_token.kind) == 1){
		*pNode = stmt();
		// todo 时间消耗点。抄作者的代码。
		// pNode = &(pNode.next);
		pNode = &((*pNode)->next);
	}
	// 希望是}
	assert(current_token.kind == TK_RBRACE);
	EXPECT(TK_RBRACE);
	
	return comNode;
}

// 很纠结这个函数的返回值。如果我自己设计，我不会想到把AstStmtNodePtr设计成指针。
// 后面，我再留意这样做有什么必要。
AstStmtNodePtr stmt()
{
	// todo C语言中能直接使用NULL吗？
	AstStmtNodePtr stmtNode = NULL;
	TokenKind kind = current_token.kind;

	switch(kind){
		case TK_IF:
			// 解析IF
			return if_stmt();
		case TK_WHILE:
			// 解析while
			return while_stmt();
		case TK_LBRACE:
			// 解析复合语句
			return compound_stmt();
		case TK_ID:
		case TK_INT:
			// 解析复合语句
			return expr_stmt();
		default:
			// todo 其他，待细化
			//return default_stmt();
			printf("stmt default error\n");
			exit(2);
			break;
	}
	
	return stmtNode;
}

// 解析if语句
AstStmtNodePtr if_stmt()
{
	AstStmtNodePtr stmt_node = create_ast_stmt_node(TK_IF);
	printf("parse if statement\n");
	assert(current_token.kind == TK_IF);
	EXPECT(TK_IF);
	assert(current_token.kind == TK_LPARENTHESES);
	EXPECT(TK_LPARENTHESES);
	// 处理表达式
	stmt_node->expr = expression();
	assert(current_token.kind == TK_RPARENTHESES);
	EXPECT(TK_RPARENTHESES);
	// 解析复合语句
	// todo 应该使用kids[0]还是kids[1]？
	stmt_node->then_stmt = compound_stmt();

	return stmt_node;
}
// 解析while语句
AstStmtNodePtr while_stmt()
{
	printf("parse while statement\n");
	AstStmtNodePtr stmt_node = create_ast_stmt_node(TK_WHILE);
	assert(current_token.kind == TK_WHILE);
	EXPECT(TK_WHILE);	
	EXPECT(TK_LPARENTHESES);
	// 处理表达式
	stmt_node->expr = expression();
	assert(current_token.kind == TK_RPARENTHESES);
	EXPECT(TK_RPARENTHESES);
	// 解析复合语句
	stmt_node->then_stmt = compound_stmt();
	
	return stmt_node;
}

void default_stmt()
{
	printf("parse default statement\n");
	do{
		NEXT_TOKEN;
	}while(current_token.kind != TK_RBRACE);
}

// todo 声明的返回值是AstNodePtr。
AstStmtNodePtr expr_stmt()
{
	// 只能解析 a = 5; 
	if(current_token.kind == TK_ID){
		NEXT_TOKEN;
		assert(current_token.kind == TK_ASSIGN);
		EXPECT(TK_ASSIGN);
		AstStmtNodePtr stmt_node = create_ast_stmt_node(TK_ASSIGN);
		stmt_node->expr = expression();
		assert(current_token.kind == TK_SEMICOLON);
		EXPECT(TK_SEMICOLON);
		return stmt_node;
	}else if(current_token.kind == TK_INT){
		//todo  TK_DECLARATION，这个类型太神奇了！
		AstStmtNodePtr stmt_node = create_ast_stmt_node(TK_DECLARATION);
		printf("process int\n");		
		// stmt_node->expr = declaration();	
		stmt_node->expr = declaration();	
		return stmt_node;
	}else{
		printf("expr stmt error\n");
		exit(7);
	}
	// todo 该怎么写更规范的代码？
	return NULL;
}

int check_is_prefix_of_stmt(TokenKind kind)
{
	int count = sizeof(prefix_of_stmt_token_kinds) / sizeof(prefix_of_stmt_token_kinds[0]);
	for(int i = 0; i < count; i++){
		if(kind == prefix_of_stmt_token_kinds[i]){
			return 1;
		}
	}
	return 0;
}

AstStmtNodePtr create_ast_stmt_node(TokenKind op)
{
	AstStmtNodePtr pNode = (AstStmtNodePtr)malloc(sizeof(struct astStmtNode));
	memset(pNode, 0, sizeof(*pNode));
	pNode->op = op;
	return pNode;
}
