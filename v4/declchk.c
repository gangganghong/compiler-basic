#include "symbol.h"
#include "ast.h"
#include "stmt.h"
#include "decl.h"
#include "expr.h"
#include "stmtchk.h"
#include "declchk.h"
// #include "symbol.h"

void CheckTranslationUnit(AstTranslationUnit transUnit)
{
	printf("%s\n", "Start Check");

	AstDeclaration p = transUnit->decls;
	VariableSymbol sym;

	while(p){
		if(p->kind == NK_Function){
			printf("%s\n", "Check function");
			CURRENT = (AstFunction)p;
			CURRENT->loops = (StmtVector)MALLOC(sizeof(struct stmtVector));
			CURRENT->loops->index = -1;

			CURRENT->breakable = (StmtVector)MALLOC(sizeof(struct stmtVector));
			CURRENT->breakable->index = -1;

			CheckFunction((AstFunction)p);
		}else{
			printf("%s\n", "Check global declaration");
			CheckGlobalDeclaration(p);
			
			// TODO 这样是查不到f对应的符号的。因为查询符号使用的是变量的内存地址而不是变量名。
			// sym = LookupID("f");
		}
		p = p->next;
	}

	// 获取了所有接口和函数的信息，可以处理接口和函数的关系了。
//	AstFunction func = FUNCTION_LIST;
//	// TODO 时间复杂度极高，有办法优化吗？
//	while(func){
//		if(func->fdec && func->fdec->receiver != NULL && func->fdec->receiver->paramDecls != NULL){
//			AppendMethod(func);
//		}
//		func = func->next;
//	}

	printf("%s\n", "End Check");
}

void CheckLocalDeclaration(AstDeclaration decls)
{
	CheckDeclaration(decls);
}

int CheckBlock(AstBlock block)
{
	AstStatement stmt = block->stmt;
	AstStatement lastStmt = NULL;
	while(stmt != NULL){
//		if(stmt->kind == NK_CompoundStatement){
//			PRINTF("check local variable\n");
//			AstCompoundStatement compoundStmt = (AstCompoundStatement)stmt;
//			// 处理局部变量
//			AstDeclaration decls = (AstDeclaration)compoundStmt->decls;
//			if(decls){
//				CheckLocalDeclaration(decls);
//			}
////			// 处理其他
////			// TODO 待完善
////			AstLabelStmt labeledStmt = compoundStmt->labeledStmt;
////			AstStatement simpleStmt = compoundStmt->stmts;	
////			if(simpleStmt){
////				compoundStmt->stmts = CheckStatement(simpleStmt);
////			}
//
//			CheckStatement(stmt);
//		}else{
//			stmt = CheckStatement(stmt);
//		}
		stmt = CheckStatement(stmt);

		if(stmt){
			lastStmt = stmt;
			stmt = stmt->next;
		}
	}

	if(lastStmt && lastStmt->kind == NK_ReturnStatement){
		return 1;
	}

	return 0;
}

void CheckFunction(AstFunction p)
{
	FunctionSymbol fsym;
	AstFunctionDeclarator fdec =  p->fdec;

	Signature sig = (Signature)MALLOC(sizeof(struct signature));
	int paramIndex = sig->paramSize - 1;
	int resultIndex = sig->resultSize - 1;

	// TODO 只有接口方法才有receiver。receiver有且仅有一个，把它当成列表，目的是扩展。
	AstParameterTypeList receiverTyList = fdec->receiver;
	if(receiverTyList != NULL){
		AstParameterDeclaration receiverDecls = receiverTyList->paramDecls;
		AstParameterDeclaration receiverDecl = receiverDecls;
		if(receiverDecl != NULL){
			CheckDeclarationSpecifiers(receiverDecl->specs);
			AstDeclarator dec = receiverDecl->dec;
			SignatureElement sigElement = (SignatureElement)MALLOC(sizeof(struct signatureElement));
			sigElement->id = dec->id;
			sigElement->ty = receiverDecl->specs->ty;
			sig->receiver = sigElement;
		}
	}

	AstParameterTypeList paramTyList = fdec->paramTyList;
	AstParameterDeclaration paramDecls = paramTyList->paramDecls;
	AstParameterDeclaration paramDecl = paramDecls;
	while(paramDecl){
		CheckDeclarationSpecifiers(paramDecl->specs);
		AstDeclarator dec = paramDecl->dec;
		SignatureElement sigElement = (SignatureElement)MALLOC(sizeof(struct signatureElement));
		sigElement->id = dec->id;
		sigElement->ty = paramDecl->specs->ty;
		sig->params[++paramIndex] = sigElement;

		// TODO 就在把参数加入变量列表。
		AddVariable(dec->id, paramDecl->specs->ty);

		paramDecl = paramDecl->next;
	}

	sig->paramSize = paramIndex + 1;

	VariableSymbol receiver;
	VariableSymbol receiverHead = (VariableSymbol)MALLOC(sizeof(struct variableSymbol));
	VariableSymbol *lastReceiver = &(receiverHead->next);
	AstParameterTypeList resultList = fdec->result;
	AstParameterDeclaration resultParamDecls = resultList->paramDecls;
	AstParameterDeclaration resultParamDecl = resultParamDecls;
	while(resultParamDecl){
		CheckDeclarationSpecifiers(resultParamDecl->specs);
		AstDeclarator dec = resultParamDecl->dec;
		SignatureElement sigElement = (SignatureElement)MALLOC(sizeof(struct signatureElement));
		if(dec != NULL){
			sigElement->id = dec->id;
		}
		sigElement->ty = resultParamDecl->specs->ty;
		sig->results[++resultIndex] = sigElement;

		sigElement = (SignatureElement)MALLOC(sizeof(struct signatureElement));
		if(dec != NULL){
			sigElement->id = dec->id;
		}
		sigElement->ty = resultParamDecl->specs->ty;
		sig->params[++paramIndex] = sigElement;

		receiver = (VariableSymbol)MALLOC(sizeof(struct variableSymbol));
		if(dec != NULL){
			receiver->name = dec->id;
		}else{
			// TODO 生成一个临时名称。
		}
		// 返回值的kind是什么？暂定为SK_Variable。
		receiver->kind = SK_Variable;
		receiver->ty = resultParamDecl->specs->ty;
		*lastReceiver = receiver;
		lastReceiver = &(receiver->next);	

		resultParamDecl = resultParamDecl->next;
	}

	sig->resultSize = resultIndex + 1;
	sig->newParamSize = paramIndex + 1;

	// if(fsym == LookupFunction(fdec) == NULL){
	// if(fsym = LookupFunction(fdec) == NULL){
	if((fsym = LookupFunction(fdec)) == NULL){
		AstDeclarator fname = fdec->dec;
		fsym = AddFunction(fname->id, sig);
		fsym->receivers = (Symbol)receiverHead->next;
		fsym->paramCount = sig->paramSize;
		fsym->receiverCount = sig->resultSize;
		FSYM = fsym;
		p->fsym = fsym;
	}else{
		ERROR("%s\n", "redefine function");
	}	

	// 把所有的函数存储到一个单链表中。
	if(FUNCTION_LIST == NULL){
		FUNCTION_LIST = FUNCTION_CURRENT = fsym;
	}else{
		FUNCTION_CURRENT->next = fsym;
		FUNCTION_CURRENT = fsym;
	}

	FUNCTION_CURRENT->lastv = &(FUNCTION_CURRENT->locals);
	// 检查函数体
	AstBlock block = p->block;
	int hasReturn = CheckBlock(block);	
	CURRENT->hasReturn = hasReturn;
	if(sig->resultSize > 0 && hasReturn == 0){
		// TODO
		ERROR("函数必须有返回值\n", "");
	}
}

void CheckGlobalDeclaration(AstDeclaration decls){
	CheckDeclaration(decls);
}

void CheckDeclaration(AstDeclaration decls)
{
	AstDeclaration decl = decls;
//	while(decl){
		// TODO 复制粘贴导致的可笑的错误。
		// if(decls->kind == NK_VarDeclaration){
		if(decl->kind == NK_VarDeclaration){
			AstVarDeclarator declarator = AsAstVarDeclarator(AsAstVarDeclaration(decl)->decs);	
			while(declarator){
				// 处理说明符
				CheckDeclarationSpecifiers(declarator->specs);
				VariableSymbol sym;
				VariableSymbol sym2;
				// TODO var a,b int = 2,4
				AstInitDeclarator initDec = (AstInitDeclarator)declarator->initDecs;
				// CG 处理a,b
				while(initDec){
					AstDeclarator dec = initDec->dec;
					if((sym = (VariableSymbol)LookupID(dec->id)) == NULL){
						sym = AddVariable(dec->id, declarator->specs->ty);
					}else{
						// TODO 需要优化
						ERROR("%s\n", "variable redefine-238");
					}	
					// 变量的初始值
					if(initDec->init){
						CheckInitializer((AstInitializer)initDec->init);
						if(sym->ty->categ == INTERFACE){	
							AssignInterfaceVariable(sym, ((AstInitializer)initDec->init)->idata); 
						 }else{
							sym->idata = ((AstInitializer)initDec->init)->idata;
						}
					}

					// TODO 测试查询功能，没有作用。
					sym2 = (VariableSymbol)LookupID(dec->id);

					initDec = (AstInitDeclarator)initDec->next;
				}
				declarator = declarator->next;
			}
		}else if(decls->kind == NK_ConstDeclaration){

		// }else{
		}else if(decl->kind == NK_TypeDeclaration){
			AstTypeDeclarator declarator = AsAstTypeDeclarator(AsAstTypeDeclaration(decl)->decs);	
			while(declarator){
				// 处理说明符
				CheckDeclarationSpecifiers(declarator->specs);
				VariableSymbol sym;
				VariableSymbol sym2;
				// TODO var a,b int = 2,4
				AstInitDeclarator initDec = (AstInitDeclarator)declarator->initDecs;
				// CG 处理a,b
				while(initDec){
					AstDeclarator dec = initDec->dec;
					if((sym = (VariableSymbol)LookupID(dec->id)) == NULL){
						sym = AddVariable(dec->id, declarator->specs->ty);
					}else{
						// TODO 需要优化
						ERROR("%s\n", "variable redefine-276");
					}	
					// 变量的初始值
					if(initDec->init){
						CheckInitializer((AstInitializer)initDec->init);
						if(sym->ty->categ == INTERFACE){	
							AssignInterfaceVariable(sym, ((AstInitializer)initDec->init)->idata); 
						 }else{
							sym->idata = ((AstInitializer)initDec->init)->idata;
						}
					}

					// TODO 测试查询功能，没有作用。
					sym2 = (VariableSymbol)LookupID(dec->id);

					initDec = (AstInitDeclarator)initDec->next;
				}
				declarator = declarator->next;
			}
		}else{
			// TODO 存在上面三者情况外的其他情况吗？
		}
//		decl = decl->next;
//	}
}

// TODO 必须加static吗？
// static void CheckGlobalDeclaration(AstDeclaration decl)
// void CheckGlobalDeclaration(AstDeclaration decls)
void CheckDeclaration2(AstDeclaration decls)
{
	if(decls->kind == NK_VarDeclaration){
		AstVarDeclaration decl = (AstVarDeclaration)decls;
		int i = 0;
		while(decl){
			AstVarDeclarator declarator = (AstVarDeclarator)decl->decs;	
			while(declarator){
				// 处理说明符
				CheckDeclarationSpecifiers(declarator->specs);
				VariableSymbol sym;
				VariableSymbol sym2;
				// TODO var a,b int = 2,4
				AstInitDeclarator initDec = (AstInitDeclarator)declarator->initDecs;
				// CG 处理a,b
				while(initDec){
					AstDeclarator dec = initDec->dec;
					if((sym = (VariableSymbol)LookupID(dec->id)) == NULL){
						sym = AddVariable(dec->id, declarator->specs->ty);
					}else{
						// TODO 需要优化
						ERROR("%s\n", "variable redefine-324");
					}	
					// 变量的初始值
					if(initDec->init){
						CheckInitializer((AstInitializer)initDec->init);
						if(sym->ty->categ == INTERFACE){	
							AssignInterfaceVariable(sym, ((AstInitializer)initDec->init)->idata); 
						 }else{
							sym->idata = ((AstInitializer)initDec->init)->idata;
						}
					}

					// TODO 测试查询功能，没有作用。
					sym2 = (VariableSymbol)LookupID(dec->id);

					initDec = (AstInitDeclarator)initDec->next;
				}
				declarator = declarator->next;
			}
			decl = decl->next;
			PRINTF("loop %d\n", i++);
		}	
	}else if(decls->kind == NK_ConstDeclaration){

	}else{
		printf("%s\n", "todo");
		
	}
}

Type CheckDeclarationSpecifiers(AstSpecifiers specs)
{
	Type ty = NULL;
	AstSpecifiers tySpecs = NULL;
	// TODO 为什么写出这样一段代码？没有什么特殊原因，我根据具体情况见招拆招罢了。
	// 根据typeAlias的值来区分两种情况，妥当吗？
	if(specs->typeAlias){
		TypeName *tname = LookupTypeName(specs->typeAlias);
		assert(tname != NULL);
		tySpecs = tname->type;
	}else{
		tySpecs = (AstSpecifiers)specs->tySpecs;
	}
	assert(tySpecs != NULL);
	if(tySpecs->kind == NK_StructSpecifier){
		// ty = CheckStructSpecifier((AstStructSpecifier)specs->tySpecs);
		ty = CheckStructSpecifier((AstStructSpecifier)tySpecs);
	}else if(tySpecs->kind == NK_ArrayTypeSpecifier){
		ty = CheckArraySpecifier((AstArrayTypeSpecifier)tySpecs);
	}else if(tySpecs->kind == NK_InterfaceSpecifier){
		ty = CheckInterfaceSpecifier((AstInterfaceSpecifier)tySpecs);
		AppendInterface(ty);
	}else{
		// TODO 需要补充。
		ty = T(INT);
	}

	// 接口变量会用到。
	ty->alias = tySpecs->typeAlias;	

	specs->ty = ty;

	return (Type)ty;
}

RecordType CheckStructSpecifier(AstStructSpecifier specs)
{
	RecordType rty = StartRecord();	
	AstStructDeclarator decl = specs->stDecls;

	while(decl){
		// CheckDeclarationSpecifiers(specs->tySpecs);
		// CheckDeclarationSpecifiers((AstSpecifiers)decl);
		CheckDeclarationSpecifiers((AstSpecifiers)decl->tySpecs);
		// AddField(rty, decl->id, decl->ty);	
		AddField(rty, decl->id, ((AstSpecifiers)(decl->tySpecs))->ty);	
		decl = (AstStructDeclarator)decl->next;
	}

	// specs->ty = rty;
	
	EndRecord(rty);

	return rty;
}

RecordType StartRecord()
{
	RecordType rty = (RecordType)MALLOC(sizeof(struct recordType));
	rty->kind = NK_RecordType;
	rty->categ = STRUCT;
	rty->id = NULL;
	rty->flds = NULL;
	rty->tail = &(rty->flds);

	return rty;
}

Field LookupField(char *fieldName, char *structName)
{
	Field fld = NULL;
	Field target = NULL;

	VariableSymbol sym = (VariableSymbol)LookupID(structName);
	if(sym == NULL) return target;
	RecordType rty = (RecordType)sym->ty;
	fld = rty->flds;
	char *id = NULL;
	while(fld){
// TODO 究竟是否需要这样做？从我遇到的具体情况看，不需要。或许还存在我没有想到的案例要求这样做。
// 先简化处理吧。
		// if(rty->categ == STRUCT){
//		if(fld->ty->categ == STRUCT){
//			target = LookupField(fieldName, fld->id);
//			if(target){
//				id = target->id;
//			}
//		}else{
//			id = fld->id;
////			target = fld;
//		}
		id = fld->id;
		if(strcmp(fieldName, id) == 0){
			target = fld;
			return target;
		}
		fld = fld->next;
	}

	return target;
}

Field AddField(RecordType rty, char *id, Type ty)
{
	int fldSize = sizeof(struct field);
	Field fld = (Field)MALLOC(fldSize);

	int idSize = sizeof(char) * MAX_NAME_LEN;
	fld->id = (char *)MALLOC(idSize);
	strcpy(fld->id, id);

	fld->ty = ty;
	
	*rty->tail = fld;
	rty->tail = &(fld->next);
	
	return fld;
}

void EndRecord(RecordType rty)
{
	Field fld = rty->flds;
	Field pre = NULL;
	int offset = 0;
	int size = 0;
	while(fld != NULL){
		if(pre != NULL){
			offset = pre->offset + pre->ty->size;
		}
		size += fld->ty->size;
		fld->offset = offset;
		pre = fld;
		fld = fld->next;
	}

	rty->size = size;
}

InitData CheckStructInitializer(AstCompositeLit compositeLit)
{
		AstSpecifiers literalType = compositeLit->literalType;
		AstLiteralValue literalValue = compositeLit->literalValue;
		AstKeyedElement element = literalValue->keyedElement;	
	
		InitData head = (InitData)MALLOC(sizeof(struct initData));
		InitData *init = &(head->next);
		int offset = 0;
		RecordType rty = CheckDeclarationSpecifiers((AstSpecifiers)literalType);
		Field fld = rty->flds;
		if(literalValue->hasKey){
			// {age:3,height:4}这种初始化值比较难处理。
			while(fld){
				AstKeyedElement targetElement = LookupElement(element, fld->id);
				*init = (InitData)MALLOC(sizeof(struct initData));
				if(targetElement == NULL){
					(*init)->expr = NULL;
				}else{
					(*init)->expr = targetElement->element->expr;	
				}
				(*init)->offset = offset;
				offset += fld->ty->size;

				init = &((*init)->next);

				fld = fld->next;
			}
		}else{
			while(fld){
				// 语义检查，值和数据类型是否匹配。
				// TODO 暂时不支持嵌套结构体。
				AstNode val = element->element->expr;
				if(!CanAssign(fld->ty, val)){
					// TODO 打印报错信息，搁置。
				}
				
				*init = (InitData)MALLOC(sizeof(struct initData));
				(*init)->expr = val;
				(*init)->offset = offset;
				offset += fld->ty->size;

				init = &((*init)->next);

				fld = fld->next;
			}	
		}

	InitData idata = head->next;
	idata->typeAlias = rty->alias;

	return idata;
}

InitData CheckCompositeLitInitializer(AstCompositeLit compositeLit)
{
	InitData idata;
	AstSpecifiers literalType = compositeLit->literalType;
	AstNode tySpecs = literalType->tySpecs;
	if(tySpecs->kind == NK_StructSpecifier){
		idata = CheckStructInitializer(compositeLit);
	}else if(tySpecs->kind == NK_ArrayTypeSpecifier){
		idata = CheckArrayInitializer(compositeLit);
	}else if(tySpecs->kind == NK_MapSpecifier){
		idata = CheckMapInitializer(compositeLit);
	}

	return idata;
}

void CheckInitializer(AstInitializer init)
{
	InitData idata;
	
	if(init->isCompositeLit == 1){
		// 处理struct{age int;height int}{3,4}这种初始化值。
		AstCompositeLit compositeLit = init->compositeLit;
		idata = CheckCompositeLitInitializer(compositeLit);
	}else{
		idata = (InitData)MALLOC(sizeof(struct initData));
		//memset(idata, 0, sizeof(struct initData));
		idata->offset = 0;
		idata->expr = init->expr;
	}

	init->idata = (AstNode)idata;
}

int CanAssign(Type ty, AstExpression val)
{

	return 1;
}

AstKeyedElement LookupElement(AstKeyedElement element, char *name)
{
	while(element){
		AstKey key = element->key;
		if(key->lbrace == 0){
			char *keyName = (char *)(((AstExpression)(key->expr))->val.p);
			if(strcmp(keyName, name) == 0){
				return element;
			}
		}
		element = element->next;
	}

	return NULL;
}

ArrayType CheckArraySpecifier(AstArrayTypeSpecifier specs)
{
	// TODO 数组的长度不总是可折叠的数据。这里是简化了问题。
	int len = specs->expr->val.i[0];
	// CheckDeclarationSpecifiers((AstSpecifiers)specs->tySpecs);
//	CheckDeclarationSpecifiers((AstSpecifiers)specs);
//	Type bty = ((AstSpecifiers)specs->tySpecs)->ty;
	Type bty = CheckDeclarationSpecifiers((AstSpecifiers)specs);
	ArrayType aty = ArrayOf(bty, len);

	return aty;
}

InitData CheckArrayInitializer(AstCompositeLit compositeLit)
{
	AstArrayTypeSpecifier node = (AstArrayTypeSpecifier)compositeLit->literalType;
	ArrayType aty = CheckDeclarationSpecifiers(node);
	AstLiteralValue literalValue = compositeLit->literalValue;
	AstKeyedElement element = literalValue->keyedElement;	

	InitData head = (InitData)MALLOC(sizeof(struct initData));
	if(head == -1){
		exit(-2);
	}

	InitData *init = &(head->next);
	int offset = 0;

	AstExpression arrayLength = node->expr;
	int length = -1;
	if(arrayLength){
		length = arrayLength->val.i[0];
	}

	int index = 0;
	// TODO RecordType转化成普通Type后，数据似乎全部丢失。此处的写法，有问题吗？
	int elementSize = aty->bty->size;
	while(element != NULL && length >= 0){
		offset = index * elementSize;
		*init = (InitData)MALLOC(sizeof(struct initData));
		(*init)->offset = offset;
		// TODO 这样做行吗？exlement->expr中的数据是否会被销毁？
		// TODO element->element->expr 没有考虑嵌套。
		(*init)->expr = element->element->expr;	
		init = &((*init)->next);
		// *init = &((*init)->next);
	
		element = element->next;
		length--;
		index++;
	}

	InitData idata = head->next;
	idata->typeAlias = aty->alias;

	return idata;
}

MapType CheckMapSpecifier(AstMapSpecifier specs)
{
	// TODO 缺少一个数据类型枚举值。
	MapType mty = (MapType)MALLOC(sizeof(struct mapType));
	CheckDeclarationSpecifiers((AstSpecifiers)specs->keyType);
	CheckDeclarationSpecifiers((AstSpecifiers)specs->elementType);
	mty->key = (Type)(((AstSpecifiers)(specs->keyType))->ty);
	mty->value = (Type)(((AstSpecifiers)(specs->elementType))->ty);
	
	return mty;
}

InitData CheckMapInitializer(AstCompositeLit compositeLit) 
{
	AstMapSpecifier node  = (AstMapSpecifier)compositeLit->literalType;

	// MapType mty = CheckMapSpecifier(node);
	MapType mty = CheckDeclarationSpecifiers((AstSpecifiers)node);
	int keySize = mty->size;
	int valSize = mty->size;
	int kvSize = keySize + valSize;
	AstLiteralValue literalValue = compositeLit->literalValue;

	AstKeyedElement element = literalValue->keyedElement;	
	InitData head = (InitData)MALLOC(sizeof(struct initData));
	if(head == -1){
		exit(-2);
	}

	//memset(head, 0, sizeof(struct initData));
	InitData *init = &(head->next);
	int offset = 0;
	int index = 0;

	// TODO 不支持嵌套
	while(element){
		int size = sizeof(struct keyValue);
		KeyValue kv = (KeyValue)MALLOC(size);
		//memset(kv, 0, size);
		kv->key = (AstExpression)(((AstKey)element->key)->expr); 
		kv->value = (AstExpression)(((AstElement)element->element)->expr);

		*init = (InitData)MALLOC(sizeof(struct initData));
		//memset(*init, 0, sizeof(struct initData));
		(*init)->kv = kv;
		offset = index * kvSize;
		(*init)->offset = offset;
		init = &((*init)->next);

		index++;
		element = element->next;
	}	
	
	InitData idata = head->next;
	idata->typeAlias = mty->alias;

	return idata;
}

InterfaceType CheckInterfaceSpecifier(AstInterfaceSpecifier specs)
{
	InterfaceType ity = (InterfaceType)MALLOC(sizeof(struct interfaceType));
	ity->categ = INTERFACE;
	AstMethodSpec methodTail = NULL;
	AstMethodSpec currentMethod = NULL;
	AstNode methods = specs->interfaceDecs;
	AstNode oneMethod = methods;
	while(oneMethod){
		if(oneMethod->kind == NK_MethodSpec){
			if(methodTail == NULL){
				methodTail = oneMethod;
				currentMethod = methodTail;
			}else{
				currentMethod->next = oneMethod;
				currentMethod = oneMethod;
			}	
		}
		oneMethod = oneMethod->next;
	}

	ity->methods = methodTail;

	return ity;
}

int CompareMethod(AstFunction func, AstMethodSpec method)
{
	// 比较 receiver、funcName、sig
	return 1;
}

// TODO 为一个单链表花了几分钟时间，不应该。
// 为啥纠结？这种创建单链表的方式和语法分析中常用的哪种方式似乎不同。
void AppendInterface(InterfaceType ty)
{
	if(INTERFACE_LIST == NULL){
		INTERFACE_LIST = ty;
		INTERFACE_CURRENT = ty;
	}else{
		INTERFACE_CURRENT->next = ty;
		INTERFACE_CURRENT = ty;
	}
}

void AppendMethod(AstFunction func)
{
	InterfaceType ty = INTERFACE_LIST;
	while(ty != NULL){
		AstMethodSpec method = ty->methods;	
		while(method != NULL){
			if(CompareMethod(func, method) == 1){
				// 加入当前接口类型的接口方法链表。
				// TODO 不知道什么原因，func->next不是0。这会破坏新建的单链表。所以，把它设置成0。
				// 不能这样做。在源代码的AST中，所有函数在一个单链表中。此处，只能重新创建一个结点。
				int size = sizeof(struct astFunction);
				AstFunction method = (AstFunction)MALLOC(size);
				memcpy(method, func, size);
				method->next = NULL;
				// 能使用 *method = *func吗？
				if(ty->methodDecl == NULL){
					ty->methodDecl = method;
					ty->methodDeclTail = method;
				}else{
					ty->methodDeclTail->next = method;
					ty->methodDeclTail = method;
				}
			}
			method = method->next;
		}
		ty = ty->next;
	}
}
