#ifndef DECLCHK_H
#define DECLCHK_H

// #include "grammer.h"
#include "ast.h"
#include "lex.h"
// #include "grammer.h"
#include "stmt.h"
#include "type2.h"
#include "expr.h"
// #include "grammer.h"


void EndRecord(RecordType rty);
Field AddField(RecordType rty, char *id, Type ty);
RecordType StartRecord();
RecordType CheckStructSpecifier(AstStructSpecifier specs);
void CheckDeclarationSpecifiers(AstSpecifiers specs);

void CheckTranslationUnit(AstTranslationUnit transUnit);
void CheckGlobalDeclaration(AstDeclaration decl);

#endif
