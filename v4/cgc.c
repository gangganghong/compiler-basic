#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "input.h"
#include "decl.h"
#include "lex.h"
#include "ast.h"
#include "packages.h"
#include "symbol.h"

int main(int argc, char *argv[])
{
	printf("I am a scanner\n");

//	printf("argc = %d\n", argc);
//	for(int i = 0; i < argc; i++){
//		printf("argv[%d] = %s\n", i, argv[i]);
//	}

	if(argc < 2){
		ERROR("need filename");
		exit(-1);
	}	

	ReadSourceFile(argv[1]);	


	// 初始化扫描器
	setupScanner();

	Token token;
	int i = 0;

	// START_CURSOR_LINK cursor_tail = (START_CURSOR_LINK)malloc(sizeof(*START_CURSOR_LINK));
	// START_CHAR_LINK char_tail = (START_CHAR_LINK)malloc(sizeof(*START_CHAR_LINK));		
	// cursor_tail = (START_CURSOR_LINK)malloc(sizeof(*START_CURSOR_LINK));
	// char_tail = (START_CHAR_LINK)malloc(sizeof(*START_CHAR_LINK));		
	// cursor_tail = (START_CURSOR_LINK)malloc(sizeof(*cursor_tail));
	// char_tail = (START_CHAR_LINK)malloc(sizeof(*char_tail));		
	cursor_tail = (START_CURSOR_LINK)malloc(sizeof(struct start_cursor_link));
	char_tail = (START_CHAR_LINK)malloc(sizeof(struct start_char_link));		

	cursor_tail->pre = cursor_tail->next = cursor_tail->start_cursor = NULL;
	char_tail->pre = char_tail->next = char_tail->start_char = NULL;

	// current_token_tail = (TOKEN_LINK)malloc(sizeof(*TOKEN_LINK));
	current_token_tail = (TOKEN_LINK)malloc(sizeof(struct token_link));
//	Token *tokenPtr = (Token *)malloc(sizeof(Token));
//	if(tokenPtr == NULL){
//		perror("init tokenPtr error\n");
//		exit(-1);
//	}
//	current_token_tail->token = tokenPtr;
//	current_token_tail->token = (Token *)malloc(sizeof(Token));
	// error: incompatible types when assigning to type 'Token' {aka 'struct token'} from type 'void *'
	current_token_tail->pre = current_token_tail->next = current_token_tail->token = NULL;
	Token *tokenPtr = (Token *)malloc(sizeof(Token));
	if(tokenPtr == NULL){
		perror("init tokenPtr error\n");
		exit(-1);
	}
	current_token_tail->token = tokenPtr;


	int loop_counter = 0;
	while(0){
		if(++loop_counter > 120){
			break;
		}
		NO_TOKEN;
		get_token();
	}
//	printf("scan token over\n");
//	exit(3);
	
		get_token();
		if(current_token.kind == TK_EOF) return 0;
		// 语法分析
		AstTranslationUnit transUnit = ParseTranslationUnit();

		InitSymbolTable();

		// 语义分析
		CheckTranslationUnit(transUnit);
	
	free(cursor_tail);
	free(char_tail);
	free(current_token_tail);

	return 0;
}

AstTranslationUnit ParseTranslationUnit(){
	
	AstTranslationUnit p;
//	CREATE_AST_NODE(p, FunctionTest);
	CREATE_AST_NODE(p, TranslationUnit);
	int i = 0;
	while(0){
		NO_TOKEN;	
		ParseAssignmentsStmt();
		printf("【%d assign over】\n", i++);
	}
	
	p = ParseSourceFile();
	LOG("parse unit over\n");

	return p;
}
