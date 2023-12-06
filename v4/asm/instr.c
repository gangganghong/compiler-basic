#include "instr.h"

OFFSET_TYPE GetOffsetType(int offset)
{
	if(-128 <= offset && offset <= 127){
		return EIGHT;
	}

	if(-65536 <= offset && offset <= 65535){
		return SIXTEEN;
	}

	return THIRTY_TWO;
}

OprandType GetOprandType()
{
	StartPeekToken();	

	unsigned char leftParenthesis = 0;
	unsigned char rightParenthesis = 0;
	unsigned char commaCount = 0;
	
	int token, preToken;
	preToken = -1;

	while(1){
		token = GetNextToken();
		// 在所有的token中都没有换行符。原因是什么？我不知道。一时半会儿我理解不了去年写的GetNextToken。
//		if(token == TYPE_TOKEN_NEWLINE){
//			break;
//		}
//		我认为SIB只能有逗号、寄存器、括号、数字组成。
//		我修改了这个函数。我认为操作数的构成要素只能是逗号、寄存器、括号、标识符、立即数、数字。
//		TODO token == TYPE_TOKEN_INDENT 可能有问题。我有没有把指令从标识符中分离出来？
		if(!(token == TYPE_TOKEN_REGISTER || token == TYPE_TOKEN_INT \
			|| token == TYPE_TOKEN_OPEN_PARENTHESES	\
			|| token == TYPE_TOKEN_COMMA			\
			|| token == TYPE_TOKEN_INT			\
			|| token == TYPE_TOKEN_IMMEDIATE			\
			|| token == TYPE_TOKEN_INDENT			\
			|| token == TYPE_TOKEN_CLOSE_PARENTHESES)){

			break;
		}

		if(token == TYPE_TOKEN_INDENT){
			// 如果是指令，需要断开。
			// TODO 最好的方法是在GetNextToken识别token是不是指令，把指令和普通的indent区分开。
			// 我不愿意现在花精力做这件事，因此像这样打补丁。
			char *name = GetCurrentTokenLexeme();
			InstructionSet instrCode = FindInstrCode(name);
			if(instrCode != I_INSTR_INVALID){
				break;
			}
		}

		if(token == TYPE_TOKEN_OPEN_PARENTHESES){
			leftParenthesis = 1;
		}
		
		if(token == TYPE_TOKEN_CLOSE_PARENTHESES){
			rightParenthesis = 1;
			break;
		}

		if(token == TYPE_TOKEN_COMMA){
			commaCount++;
		}

		preToken = token;
	}

	OprandType type;
	if(leftParenthesis & rightParenthesis){
		type = REG_BASE_MEM;	
		if(commaCount == 2){
			type = T_SIB;
		}
	}else{
		// 合法的操作数，如果不包含小括号，必定只有一个标识符。
		// 这个if..else if语句群可以改成多个if语句。
		if(preToken == TYPE_TOKEN_INT){
			type = IMM_BASE_MEM;
		}else if(preToken == TYPE_TOKEN_IMMEDIATE){
			type = IMM;
		}else if(preToken == TYPE_TOKEN_REGISTER){
			type = REG;
		}	
	}

	EndPeekToken();

	if(preToken == -1){
		printf("there is a invalid oprand in line %d in file %s\n", __LINE__, __FILE__);
		exit(-1);
	}

	return type;
}

SIB ParseSIB()
{
	StartPeekToken();
	int token = GetNextToken();
	int offset = -1;
	char *name;
	if(token == TYPE_TOKEN_INT){
		name = GetCurrentTokenLexeme();
		offset = StrToNumber(name);
	}
	EndPeekToken();

	// offset不是-1，表示第一个token是-4(%ebp)中的-4。应该跳过它。
	if(offset != -1){
		GetNextToken();
	}
	// 要处理小括号。
	// 一个SIB实例，(%ebx,%eax,2)
	SIB sib = (SIB)MALLOC(sizeof(struct sib));
	GetNextToken();		// 获取(
	GetNextToken();		// 获取%ebx
	char *baseRegName = GetCurrentTokenLexeme();
	GetNextToken();		// 获取,
	GetNextToken();		// 获取%eax
	char *indexRegName = GetCurrentTokenLexeme();
	GetNextToken();		// 获取,
	GetNextToken();		// 获取2
	char *scaleName = GetCurrentTokenLexeme();
	GetNextToken();		// 获取)

	sib->base = FindRegIndex(++baseRegName)->index;
	sib->index = FindRegIndex(++indexRegName)->index;
	sib->scale = StrToNumber(scaleName);
	sib->offset = offset;
	
	return sib;
}

Oprand ParseOprand()
{
	OprandType type = GetOprandType();

	Oprand opr = (Oprand)MALLOC(sizeof(struct oprand));	

	// IMM, T_SIB, REG_BASE_MEM, IMM_BASE_MEM, REG
	if(type == IMM){
		int token = GetNextToken();
		char *name = GetCurrentTokenLexeme();
		name++;
		opr->value.immediate = StrToNumber(name);
		opr->type = IMM;
	}else if(type == T_SIB){
		SIB sib = ParseSIB();
		opr->value.sib = *sib;
	}else if(type == REG_BASE_MEM){
		StartPeekToken();
		int token = GetNextToken();
		int offset = -1;
		char *name;
		if(token == TYPE_TOKEN_INT){
			name = GetCurrentTokenLexeme();
			offset = StrToNumber(name);
		}
		EndPeekToken();
		
		// offset不是-1，表示第一个token是-4(%ebp)中的-4。应该跳过它。
		if(offset != -1){
			GetNextToken();
		}

		// 跳过(。
		GetNextToken();
		// 只能是寄存器，否则就不是合法的汇编代码。本项目只负责处理正确的汇编代码。
		int currentToken = GetNextToken();
		if(currentToken != TYPE_TOKEN_REGISTER){
			printf("%s is not a register in line %d\n", GetCurrentTokenLexeme(), __LINE__);
			exit(__LINE__);
		}
		// 计算寄存器的编号。
		// 前面获取的name是寄存器，%eax。
		name = GetCurrentTokenLexeme();
		name++;
		// TODO 这里应该判断regInfo是不是NULL，我懒得马上写。
		RegInfo regInfo = FindRegIndex(name);
		opr->value.regBaseMem.reg = regInfo->index;
		opr->value.regBaseMem.offset = offset;
		
		opr->type = REG_BASE_MEM;

	}else if(type == IMM_BASE_MEM){
		int token = GetNextToken();
		char *name = GetCurrentTokenLexeme();
		opr->value.immBaseMem = StrToNumber(name);
		opr->type = IMM_BASE_MEM;
	}else if(type == REG){
		// 计算寄存器的编号。
		// 前面获取的name是寄存器，%eax。
		int token = GetNextToken();
		char *name = GetCurrentTokenLexeme();
		name++;
		opr->value.reg = FindRegIndex(name);
		opr->type = REG;
	} 

	return opr;
}

Instruction GenerateSimpleInstr(int prefix, Opcode opcode, ModRM modRM,\
	 SIB sib, int offset, int immediate)
{
	int size = sizeof(struct instruction);
	Instruction instr = (Instruction)MALLOC(size);
	instr->prefix = prefix;
	instr->opcode = opcode;
	instr->modRM = modRM;
	instr->sib = sib;
	instr->offset = offset;
	instr->immediate = immediate;

	return instr;
}

Instruction ParseFchsInstr(InstructionSet instrCode)
{
	Opcode opcode = {0xD9, 0xE0};

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, NULL, NULL, 0, 0);
}

Instruction ParseFldzInstr(InstructionSet instrCode)
{
	Opcode opcode = {0xD9, 0xEE};

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, NULL, NULL, 0, 0);
}

Instruction ParseFucomppInstr(InstructionSet instrCode)
{
	Opcode opcode = {0xDA, 0xE9};

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, NULL, NULL, 0, 0);
}

Instruction ParseFld1Instr(InstructionSet instrCode)
{
	Opcode opcode = {0xD9, 0xE8};

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, NULL, NULL, 0, 0);
}

Instruction ParseFaddsInstr(InstructionSet instrCode)
{
	// d8 05 45 23 01 00    	fadds  0x12345
	
	Opcode opcode = {0xD8, -1};

	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 0;
	// vim没有高亮0b101，但这种写法是正确的。
	// 为什么是101？有空时写上注释。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

Instruction ParseFaddlInstr(InstructionSet instrCode)
{
	// dc 05 45 23 01 00    	fadds  0x12345
	
	Opcode opcode = {0xDC, -1};

	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 0;
	// vim没有高亮0b101，但这种写法是正确的。
	// 为什么是101？有空时写上注释。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

Instruction ParseFsubsInstr(InstructionSet instrCode)
{
	// d8 25 45 23 01 00    	fsubs  0x12345

	Opcode opcode = {0xD8, -1};

	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 4;
	// vim没有高亮0b101，但这种写法是正确的。
	// 为什么是101？有空时写上注释。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

Instruction ParseFsublInstr(InstructionSet instrCode)
{
	// dc 25 45 23 01 00    	fsubl  0x12345
	
	Opcode opcode = {0xDC, -1};

	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 4;
	// vim没有高亮0b101，但这种写法是正确的。
	// 为什么是101？有空时写上注释。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

Instruction ParseFdivsInstr(InstructionSet instrCode)
{
	Opcode opcode = {0xD8, -1};

	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 6;
	// vim没有高亮0b101，但这种写法是正确的。
	// 为什么是101？有空时写上注释。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

Instruction ParseFdivlInstr(InstructionSet instrCode)
{
	Opcode opcode = {0xDC, -1};

	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 6;
	// vim没有高亮0b101，但这种写法是正确的。
	// 为什么是101？有空时写上注释。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

Instruction ParseFnstswInstr(InstructionSet instrCode)
{
	Opcode opcode = {0xDF, 0xE0};

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, NULL, NULL, 0, 0);
}

Instruction ParseFldsInstr(InstructionSet instrCode)
{
	// 0:	d9 05 45 23 01 00    	flds   0x12345
	
	Opcode opcode = {0xD9, -1};
	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 0;
	// vim没有高亮0b101，但这种写法是正确的。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

// todo 这个函数和ParseFldsInstr太相似了。找机会优化它。
Instruction ParseFldlInstr(InstructionSet instrCode)
{
	// dd 05 45 23 01 00    	fldl   0x12345
	
	Opcode opcode = {0xDD, -1};
	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 0;
	// vim没有高亮0b101，但这种写法是正确的。
	// 为什么是101？有空时写上注释。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

Instruction ParseFildlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFildqInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFstpsInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFstplInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFldcwInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFistplInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFistpllInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFstsInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFstlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFstpInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseFmulsInstr(InstructionSet instrCode)
{
	Opcode opcode = {0xD8, -1};

	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 6;
	// vim没有高亮0b101，但这种写法是正确的。
	// 为什么是101？有空时写上注释。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

Instruction ParseFmullInstr(InstructionSet instrCode)
{
	Opcode opcode = {0xDC, -1};

	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->mod = 0;
	modRM->regOrOpcode = 1;
	// vim没有高亮0b101，但这种写法是正确的。
	// 为什么是101？有空时写上注释。
	modRM->rm = 0b101;

	// 只有一个操作数需要处理。
	int token = GetNextToken();
	// 这个操作数是一个内存地址，应该存储到偏移字段。
	// 这个操作数是不是合法的字符串数值，不是本函数甚至本程序的职责，不理会。
	char *name = GetCurrentTokenLexeme();
	int offset = StrToNumber(name);

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, modRM, NULL, offset, 0);
}

Instruction ParseCdqInstr(InstructionSet instrCode)
{
	Opcode opcode = {0x99, -1};

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, NULL, NULL, 0, 0);
}

Instruction ParseRetInstr(InstructionSet instrCode)
{
	GetNextToken();
	// unsigned char opcode = 0xC3;
	Opcode opcode = {0xC3, -1};

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, NULL, NULL, 0, 0);
}

Instruction ParseMullInstr(InstructionSet instrCode)
{
	int prefix = 0;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	opcode.primaryOpcode = 0xF7;

	modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->regOrOpcode = 4;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;

		if(regSize == 32){
			modRM->rm = reg->index;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		if(type == T_SIB){
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){
			struct memoryAddress regBaseMem = opr->value.regBaseMem;
			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseIdivlInstr(InstructionSet instrCode)
{
	int prefix = 0;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	opcode.primaryOpcode = 0xF7;

	modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->regOrOpcode = 7;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;

		if(regSize == 32){
			modRM->rm = reg->index;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		if(type == T_SIB){
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){
			struct memoryAddress regBaseMem = opr->value.regBaseMem;
			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseDivlInstr(InstructionSet instrCode)
{
	int prefix = 0;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	opcode.primaryOpcode = 0xF7;

	modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->regOrOpcode = 6;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;

		if(regSize == 32){
			modRM->rm = reg->index;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		if(type == T_SIB){
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){
			struct memoryAddress regBaseMem = opr->value.regBaseMem;
			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseNeglInstr(InstructionSet instrCode)
{
	int prefix = 0;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	opcode.primaryOpcode = 0xF7;

	modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->regOrOpcode = 3;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;

		if(regSize == 32){
			modRM->rm = reg->index;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		if(type == T_SIB){
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){
			struct memoryAddress regBaseMem = opr->value.regBaseMem;
			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseNotlInstr(InstructionSet instrCode)
{
	int prefix = 0;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	opcode.primaryOpcode = 0xF7;

	modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->regOrOpcode = 2;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;

		if(regSize == 32){
			modRM->rm = reg->index;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		if(type == T_SIB){
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){
			struct memoryAddress regBaseMem = opr->value.regBaseMem;
			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseJeInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJnpInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJneInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJpInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJgInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJaInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJbInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJgeInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJaeInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJleInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJbeInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseJmpInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParsePushlInstr(InstructionSet instrCode)
{
	int prefix = -1;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		opcode.primaryOpcode = 0x50 + reg->index;
	}else{
		opcode.primaryOpcode = 0xFF;

		modRM = (ModRM)MALLOC(sizeof(struct modRM));
		modRM->regOrOpcode = 6;

		if(type == T_SIB){
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){
			struct memoryAddress regBaseMem = opr->value.regBaseMem;	
			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	int size = sizeof(struct instruction);
	Instruction instr = (Instruction)MALLOC(size);
	instr->prefix = prefix;
	instr->opcode = opcode;
	instr->modRM = modRM;
	instr->sib = sib;
	instr->offset = offset;
	instr->immediate = immediate;

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParsePoplInstr(InstructionSet instrCode)
{
	int prefix = 0;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	opcode.primaryOpcode = 0x58;

	// 只有一个操作数。按照指令模板，这个操作数只能是寄存器。
	// TODO 如果操作数不是寄存器怎么办？在本项目，一律不处理这种意外。
	// TODO 以后有时间，可以增加对意外的处理。
	Oprand opr = ParseOprand();
	RegInfo reg = opr->value.reg;

	modRM = (ModRM)MALLOC(sizeof(struct modRM));
	modRM->regOrOpcode = reg->index;

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseIncbInstr(InstructionSet instrCode)
{
	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	Opcode opcode = {-1, -1};
	int prefix = -1;
	int offset = 0;
	SIB sib = NULL;
	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));

/**
 * 一些机器码。
0:  fe c0                   inc    %al
2:   66 43                   inc    %bx
4:   43                      inc    %ebx
fe c3                	inc    %bl
 *
 * c0--->11000000--->11-000-000
 * c3--->11000011--->11-000-011
 */

	opcode.primaryOpcode = 0xFE;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;
		if(regSize == 8){
			modRM->mod = 3;
			modRM->rm = reg->index;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		// 内存地址很麻烦。
		// 内存地址的类型：45, 4(%ebp)，-4(%eax, %ebx, 2), my_str。
		// 不知道怎么处理标识符。
		// IMM, T_SIB, REG_BASE_MEM, IMM_BASE_MEM, REG
		if(type == T_SIB){
			// 机器码：fe 84 99 00 02 00 00 	incb   0x200(%ecx,%ebx,4)
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){
			struct memoryAddress regBaseMem = opr->value.regBaseMem;	

			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
			
		}else if(type == IMM_BASE_MEM){
			// TODO 搁置。
			// 机器码：fe 05 08 00 00 00    	incb   0x8
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, 0);
}

Instruction ParseIncwInstr(InstructionSet instrCode)
{
	int prefix = -1;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;
	// 16位指令的前缀是0x66。
	prefix = 0x66;
	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;

		if(regSize == 16){
			opcode.primaryOpcode = 0x40 + reg->index;
			modRM = NULL;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		modRM = (ModRM)MALLOC(sizeof(struct modRM));
		// 内存地址很麻烦。
		// 内存地址的类型：45, 4(%ebp)，-4(%eax, %ebx, 2), my_str。
		// 不知道怎么处理标识符。
		// IMM, T_SIB, REG_BASE_MEM, IMM_BASE_MEM, REG
		if(type == T_SIB){
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){
			// 66 ff 43 08          	incw   0x8(%ebx)
			// 43--->01-000-011-->mod=01,reg/opcode=000,rm=011
			// ff 83 fe ff 00 00    	incl   0xfffe(%ebx)
			
			opcode.primaryOpcode = 0xFF;
			struct memoryAddress regBaseMem = opr->value.regBaseMem;

			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg; 
		}else if(type == IMM_BASE_MEM){
			// TODO 搁置。
			// 机器码：66 ff 05 00 02 00 00 	incw   0x200
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseInclInstr(InstructionSet instrCode)
{
	// 和INCW的处理机制的差异只有一点：没有前缀。
	int prefix = -1;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;

		if(regSize == 32){
			opcode.primaryOpcode = 0x40 + reg->index;
			modRM = NULL;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		modRM = (ModRM)MALLOC(sizeof(struct modRM));
		// 内存地址很麻烦。
		// 内存地址的类型：45, 4(%ebp)，-4(%eax, %ebx, 2), my_str。
		// 不知道怎么处理标识符。
		// IMM, T_SIB, REG_BASE_MEM, IMM_BASE_MEM, REG
		if(type == T_SIB){
			// 机器码：66 ff 84 99 00 02 00 00	incw   0x200(%ecx,%ebx,4)
			// 84--->10-000-100-->mod=10,reg/opcode=000,rm=100
			opcode.primaryOpcode = 0xFF;

			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){
			// 机器码：ff 43 08             	incl   0x8(%ebx)
			// 43-->01-000-011
			// 机器码：ff 83 48 23 01 00    	incl   0x12348(%ebx)
			// 83-->10-000-011
			
			opcode.primaryOpcode = 0xFF;
			struct memoryAddress regBaseMem = opr->value.regBaseMem;	

			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			// TODO 搁置。
			// 机器码：66 ff 05 00 02 00 00 	incw   0x200
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseDecbInstr(InstructionSet instrCode)
{
	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	Opcode opcode = {-1, -1};
	int prefix = -1;
	int offset = 0;
	SIB sib = NULL;
	ModRM modRM = (ModRM)MALLOC(sizeof(struct modRM));;

	opcode.primaryOpcode = 0xFE;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;
		if(regSize == 8){
			modRM->mod = 3;
			modRM->regOrOpcode = 1;
			modRM->rm = reg->index;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		// 内存地址很麻烦。
		// 内存地址的类型：45, 4(%ebp)，-4(%eax, %ebx, 2), my_str。
		// 不知道怎么处理标识符。
		// IMM, T_SIB, REG_BASE_MEM, IMM_BASE_MEM, REG
		if(type == T_SIB){
			modRM->regOrOpcode = 1;
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100; 
		}else if(type == REG_BASE_MEM){
			struct memoryAddress regBaseMem = opr->value.regBaseMem;

			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->regOrOpcode = 1;
			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			// TODO 搁置。
			// 机器码：fe 05 08 00 00 00    	incb   0x8
			modRM->mod = 0b00;
			modRM->regOrOpcode = 1;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, 0);
}

Instruction ParseDecwInstr(InstructionSet instrCode)
{
	int prefix = -1;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	// 16位指令的前缀是0x66。
	prefix = 0x66;
	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;

		if(regSize == 16){
			opcode.primaryOpcode = 0x48 + reg->index;
			modRM = NULL;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		modRM = (ModRM)MALLOC(sizeof(struct modRM));
		modRM->regOrOpcode = 1;
		// 内存地址很麻烦。
		// 内存地址的类型：45, 4(%ebp)，-4(%eax, %ebx, 2), my_str。
		// 不知道怎么处理标识符。
		// IMM, T_SIB, REG_BASE_MEM, IMM_BASE_MEM, REG
		if(type == T_SIB){
			opcode.primaryOpcode = 0xFF;
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){			
			opcode.primaryOpcode = 0xFF;
			struct memoryAddress regBaseMem = opr->value.regBaseMem;

			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseDeclInstr(InstructionSet instrCode)
{
	// 和DECW的处理机制的差异只有一点：没有前缀。
	int prefix = -1;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	Oprand opr = ParseOprand();
	OprandType type = opr->type;

	if(type == REG){
		RegInfo reg = opr->value.reg;
		char regSize = reg->size;

		if(regSize == 32){
			opcode.primaryOpcode = 0x48 + reg->index;
			modRM = NULL;
		}else{
			// TODO 不会出现这种情况。
		}
	}else{
		modRM = (ModRM)MALLOC(sizeof(struct modRM));
		modRM->regOrOpcode = 1;
		// 内存地址很麻烦。
		// 内存地址的类型：45, 4(%ebp)，-4(%eax, %ebx, 2), my_str。
		// 不知道怎么处理标识符。
		// IMM, T_SIB, REG_BASE_MEM, IMM_BASE_MEM, REG
		if(type == T_SIB){
			opcode.primaryOpcode = 0xFF;
			sib = &(opr->value.sib);
			offset = sib->offset;
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}
			modRM->rm = 0b100;
		}else if(type == REG_BASE_MEM){			
			opcode.primaryOpcode = 0xFF;
			struct memoryAddress regBaseMem = opr->value.regBaseMem;

			offset = regBaseMem.offset; 
			if(offset == 0){
				modRM->mod = 0b00;
			}else{
				OFFSET_TYPE offsetType = GetOffsetType(offset);
				if(offsetType == EIGHT){
					modRM->mod = 0b01;
				}else{
					modRM->mod = 0b10;
				}
			}

			modRM->rm = regBaseMem.reg;
		}else if(type == IMM_BASE_MEM){
			modRM->mod = 0b00;
			modRM->rm = 0b101;
			offset = opr->value.immBaseMem;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseRepInstr(InstructionSet instrCode)
{
	// 由于模板限制，这条指令是 rep movsb。
	Opcode opcode = {0xF3, 0xA4};

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, NULL, NULL, 0, 0);
}

Instruction ParseCallInstr(InstructionSet instrCode)
{

	Opcode opcode = {0xE8, -1};

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(0, opcode, NULL, NULL, 0xFFFFFFFC, 0);
}

Instruction ParseOrlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseXorlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseAndlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseShllInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseSHLDInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseSarlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseShrlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseAddlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseSublInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseImullInstr(InstructionSet instrCode)
{
	int prefix = -1;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	// 这一次，先读取两个操作数。
	Oprand opr1 = ParseOprand();
	Oprand opr2 = NULL;
	char ch = GetLookAheadChar();
	if(ch == ','){
		// 跳过逗号
		GetNextToken();
		opr2 = ParseOprand();
	}

	modRM = (ModRM)MALLOC(sizeof(struct modRM));

	// 只有一个操作数。
	if(opr2 == NULL){
		OprandType type = opr1->type;

		opcode.primaryOpcode = 0xF7;

		modRM->regOrOpcode = 5;

		if(type == REG){
			RegInfo reg = opr2->value.reg;
			char regSize = reg->size;

			if(regSize == 32){
				modRM->mod = 0b11;
				modRM->rm = reg->index;
			}else{
			// TODO 不会出现这种情况。
			}
		}else{
			// 内存地址很麻烦。
			// 内存地址的类型：45, 4(%ebp)，-4(%eax, %ebx, 2), my_str。
			// 不知道怎么处理标识符。
			// IMM, T_SIB, REG_BASE_MEM, IMM_BASE_MEM, REG
			if(type == T_SIB){
				sib = &(opr2->value.sib);
				offset = sib->offset;
				if(offset == 0){
					modRM->mod = 0b00;
				}else{
					OFFSET_TYPE offsetType = GetOffsetType(offset);
					if(offsetType == EIGHT){
						modRM->mod = 0b01;
					}else{
						modRM->mod = 0b10;
					}
				}
				modRM->rm = 0b100;
			}else if(type == REG_BASE_MEM){			
				opcode.primaryOpcode = 0xFF;
				struct memoryAddress regBaseMem = opr2->value.regBaseMem;

				offset = regBaseMem.offset; 
				if(offset == 0){
					modRM->mod = 0b00;
				}else{
					OFFSET_TYPE offsetType = GetOffsetType(offset);
					if(offsetType == EIGHT){
						modRM->mod = 0b01;
					}else{
						modRM->mod = 0b10;
					}
				}

				modRM->rm = regBaseMem.reg;
			}else if(type == IMM_BASE_MEM){
				modRM->mod = 0b00;
				modRM->rm = 0b101;
				offset = opr2->value.immBaseMem;
			}
		}
	}else{
		opcode.primaryOpcode = 0x0F;
		opcode.secondaryOpcode = 0xAF;

		// 有两个操作数。src = opr1, dst = opr2。
		OprandType type1 = opr1->type;
		OprandType type2 = opr2->type;

		// TODO 我不确定 type1 == type1 == REG 是否等价于 type1 == REG && type2== REG。
		if(type1 == type2 == REG){
			// 机器码：0f af d8             	imul   %eax,%ebx
			// ebx存储在reg/opcode中。依据是我琢磨出来的那条规则。
			// 什么规则？指令的哪个操作数必须是寄存器，那个操作数就存储在reg/opcode中。
			// d8-->11-011-000--->mod=0b11,reg/opcode=0b011,rm=0b000。

			// 机器码：	0f af cb             	imul   %ebx,%ecx
			// cb-->11-001-011-->mod=0b11,reg/opcode=0b001,rm=0b011。
			modRM->mod = 0b11;
			RegInfo reg1 = opr1->value.reg;
			RegInfo reg2 = opr2->value.reg;
			modRM->regOrOpcode = reg2->index;
			modRM->rm = reg1->index;
		}else{
			// TODO 是否需要判断 src == IMM && dst == reg？
			// 不判断。本项目对这种意外情况一律不处理。

			// 操作数组合`src = IMM, dst = reg`，需要转换。
			// 机器码：69 db 00 04 00 00    	imul   $0x400,%ebx,%ebx。
			// db-->11-011-011--->mod=0b11,reg=3,rm=0b011-->rm是寄存器的编号。
			// 第二个操作数是dst。
			modRM->mod = 0b11;
			RegInfo reg2 = opr2->value.reg;
			modRM->regOrOpcode = reg2->index;
			modRM->rm = reg2->index;
			// 第一个操作数是立即数。
			immediate = opr1->value.immediate;
		}
	}

	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}

Instruction ParseMovlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseMovbInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseMovwInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseCmplInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseTestInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseMovsblInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseMovswlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseMovzblInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseMovzwlInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseMovzxInstr(InstructionSet instrCode)
{


return NULL;
}

Instruction ParseLealInstr(InstructionSet instrCode)
{
	int prefix = -1;
	Opcode opcode = {-1, -1};
	ModRM modRM = NULL;
	SIB sib = NULL;
	int offset = 0;
	int immediate = 0;

	// Leal的两条模板是：leal %0, %%edi 和 leal %0, %1。

	modRM = (ModRM)MALLOC(sizeof(struct modRM));

	// 处理第一个操作数。src。根据src确定rm和mod的值。
	Oprand opr1 = ParseOprand();
	// 这个操作数是内存地址。内存地址的种类：-4(%ebp),SIB,0x1234。
	OprandType type1 = opr1->type;

	opcode.primaryOpcode = 0x8D;

	if(type1 == T_SIB){
		sib = &(opr1->value.sib);
		offset = sib->offset;
		if(offset == 0){
			modRM->mod = 0b00;
		}else{
			OFFSET_TYPE offsetType = GetOffsetType(offset);
			if(offsetType == EIGHT){
				modRM->mod = 0b01;
			}else{
				modRM->mod = 0b10;
			}
		}
		modRM->rm = 0b100;

	}else if(type1 == REG_BASE_MEM){			
		struct memoryAddress regBaseMem = opr1->value.regBaseMem;	

		offset = regBaseMem.offset; 
		if(offset == 0){
			modRM->mod = 0b00;
		}else{
			OFFSET_TYPE offsetType = GetOffsetType(offset);
			if(offsetType == EIGHT){
				modRM->mod = 0b01;
			}else{
				modRM->mod = 0b10;
			}
		}
		
		modRM->rm = regBaseMem.reg;
	}else if(type1 == IMM_BASE_MEM){
		modRM->mod = 0b00;
		modRM->rm = 0b101;
		offset = opr1->value.immBaseMem;
	}

	// 跳过操作数中间的逗号。
	GetNextToken();
	// 处理第二个操作数。dest。根据dst确定reg/opcode的值。
	// dst只能是寄存器。在本项目中不处理dst不是寄存器的情况。
	Oprand opr2 = ParseOprand();
	RegInfo reg = opr2->value.reg;
	modRM->regOrOpcode = reg->index;


	//GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate)
	return GenerateSimpleInstr(prefix, opcode, modRM, sib, offset, immediate);
}
