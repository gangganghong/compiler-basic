char *str = "Hello, World\n";

void print()
{
	asm("movl $13, %%edx	\n\t"
		"movl %0, %%ecx		\n\t"
		"movl $0, %%ebx		\n\t"
		"movl $4, %%eax		\n\t"
		"int $0x80			\n\t"
		::"r"(str): "ecx", "ebx"

	);
}

void exit()
{
	asm("movl $44, %ebx	\n\t"
		"movl $1, %eax		\n\t"
		"int $0x80			\n\t"
	);
}

void nomain()
{
	print();
	exit();
}
