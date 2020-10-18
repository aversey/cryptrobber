/*
	Simple Stack Translator
	created by exegete
*/

#include <stdio.h>
#include <stdlib.h>

struct stack_elem
{
	unsigned long long value;
	struct stack_elem *next;
};

struct identifier
{
	const char *lexem;
	unsigned long long value;
	struct identifier *next;
};

struct segment
{
	char *buffer;
	unsigned long long real_size;
	unsigned long long size;
	unsigned long long pointer_address;
	unsigned long long base_address;
	unsigned long long base_offset;
	int data_size;
	int data_endianness;
	struct segment *next;
};

struct environment
{
	struct stack_elem *stack;
	struct identifier *id_list;
	struct segment *seg_list;
	unsigned long long chosen_segment;
};

struct operator
{
	void (*func)(struct environment *env);
	const char *lexem;
};

#define ERROR_STATUS_STACK_OVERFLOW 1
#define ERROR_STATUS_STACK_EMPTY 2
#define ERROR_STATUS_ID_LIST_OVERFLOW 3
#define ERROR_STATUS_SEG_LIST_OVERFLOW 4
#define ERROR_STATUS_INVALID_SEGMENT 5
#define ERROR_STATUS_WORD_BUFFER_OVERFLOW 6
#define ERROR_STATUS_LEXEM_OVERFLOW 7
#define ERROR_STATUS_UNDEFINED 8
#define ERROR_STATUS_INVALID_ACCESS 9
#define ERROR_STATUS_INVALID_WORD_SIZE 10
#define ERROR_STATUS_WORD_ENDIANNESS_INVALID 11

const char *error_msgs[] =
{
	"Stack overflow",
	"Stack is empty",
	"Identifier list overflow",
	"Segment list overflow",
	"Invalid chosen segment",
	"Word buffer overflow",
	"Lexem is too big",
	"Undefined lexem: ",
	"Invalid segment access",
	"Invalid segment data size",
	"Invalid segment data endianness"
};

void exit_error(int status, const char *lexem)
{
	if(lexem)
		fprintf(stderr, "Error: %s%s\n", error_msgs[status - 1],
			lexem);
	else
		fprintf(stderr, "Error: %s\n", error_msgs[status - 1]);
	exit(status);
}

unsigned long long create_segment(struct environment *env)
{
	struct segment **chosen_segment = &env->seg_list;
	unsigned long long segment_counter = 0;
	for(; *chosen_segment; chosen_segment = &(*chosen_segment)->next)
		segment_counter++;
	*chosen_segment = malloc(sizeof(struct segment));
	if(!*chosen_segment)
		exit_error(ERROR_STATUS_SEG_LIST_OVERFLOW, NULL);
	(*chosen_segment)->buffer = NULL;
	(*chosen_segment)->real_size = 0;
	(*chosen_segment)->size = 0;
	(*chosen_segment)->pointer_address = 0;
	(*chosen_segment)->base_address = 0;
	(*chosen_segment)->base_offset = 0;
	(*chosen_segment)->data_size = 1;
	(*chosen_segment)->data_endianness = 0;
	(*chosen_segment)->next = NULL;
	return segment_counter;
}

struct segment *get_segment(struct environment *env)
{
	struct segment *chosen_segment = env->seg_list;
	unsigned long long segment_counter = env->chosen_segment;
	while(chosen_segment && segment_counter--)
		chosen_segment = chosen_segment->next;
	if(!chosen_segment)
		exit_error(ERROR_STATUS_INVALID_SEGMENT, NULL);
	return chosen_segment;
}

void initialize_environment(struct environment *env)
{
	env->stack = NULL;
	env->id_list = NULL;
	env->seg_list = NULL;
	env->chosen_segment = 0;
}

int is_separator(int symb)
{
	return symb == ' ' || symb == '\n' || symb == '\t' || symb == '\r';
}

#define BUFFER_INITIAL_SIZE 4

int double_buffer(char **buffer, unsigned long long *buffer_size)
{
	char *tmp_pointer = *buffer;
	unsigned long long tmp_size = *buffer_size;

	if(*buffer_size == 0)
		*buffer_size = BUFFER_INITIAL_SIZE;
	else
		*buffer_size *= 2;

	*buffer = malloc(*buffer_size);
	if(!*buffer)
		return 0;

	for(; tmp_size > 0; tmp_size--)
		(*buffer)[tmp_size - 1] = tmp_pointer[tmp_size - 1];

	free(tmp_pointer);
	return 1;
}

char *read_lexem()
{
	char *buffer = NULL;
	unsigned long long buffer_size = 0;
	int symb;
	unsigned long long index;

	while(is_separator(symb = getchar()))
	{}

	if(symb == EOF)
		return NULL;

	for(index = 0;; index++)
	{
		if(index == buffer_size)
			if(!double_buffer(&buffer, &buffer_size))
				exit_error(ERROR_STATUS_LEXEM_OVERFLOW, NULL);
		if(symb == EOF || is_separator(symb))
		{
			buffer[index] = 0;
			break;
		}
		buffer[index] = symb;
		symb = getchar();
	}
	return buffer;
}

void push_stack_elem(struct stack_elem **stack, unsigned long long value)
{
	struct stack_elem *head = *stack;
	*stack = malloc(sizeof(struct stack_elem));
	if(!*stack)
		exit_error(ERROR_STATUS_STACK_OVERFLOW, NULL);
	(*stack)->value = value;
	(*stack)->next = head;
}

unsigned long long pop_stack_elem(struct stack_elem **stack)
{
	struct stack_elem *head = *stack;
	unsigned long long value;
	if(!head)
		exit_error(ERROR_STATUS_STACK_EMPTY, NULL);
	value = head->value;
	*stack = head->next;
	free(head);
	return value;
}

int is_symb_hex(char symb)
{
	if(symb >= '0' && symb <= '9')
		return symb - '0';
	else
	if(symb >= 'A' && symb <= 'F')
		return symb - 'A' + 10;
	else
	if(symb >= 'a' && symb <= 'f')
		return symb - 'a' + 10;
	else
		return -1;
}

int is_lexem_hex(struct environment *env, const char *lexem)
{
	int sign = 0;
	unsigned long long hex = 0;
	if(*lexem == '-' && *(lexem + 1))
	{
		sign = 1;
		lexem++;
	}

	for(; *lexem; lexem++)
	{
		int num = is_symb_hex(*lexem);
		if(num != -1)
			hex = (hex << 4) + num;
		else
			return 0;
	}
	hex = sign ? ~hex + 1 : hex;
	push_stack_elem(&env->stack, hex);
	return 1;
}

int compare_lexems(const char *lexem_one, const char *lexem_two)
{
	for(; *lexem_one && *lexem_two; lexem_one++, lexem_two++)
		if(*lexem_one != *lexem_two)
			return 0;
	return *lexem_one == *lexem_two;
}

void add_identifier(struct identifier **id_list, const char *lexem,
	unsigned long long value)
{
	struct identifier *head = *id_list;
	*id_list = malloc(sizeof(struct identifier));
	if(!id_list)
		exit_error(ERROR_STATUS_ID_LIST_OVERFLOW, NULL);
	(*id_list)->lexem = lexem;
	(*id_list)->value = value;
	(*id_list)->next = head;
}

int find_identifier(struct environment *env, const char *lexem)
{
	struct identifier *id_list = env->id_list;
	for(; id_list; id_list = id_list->next)
	{
		if(compare_lexems(lexem, id_list->lexem))
		{
			push_stack_elem(&env->stack, id_list->value);
			return 1;
		}
	}
	return 0;
}

void segment_compile_byte(struct environment *env, char byte)
{
	unsigned long long index = get_segment(env)->size;
	if(index == get_segment(env)->real_size)
		if(!double_buffer(&get_segment(env)->buffer,
			&get_segment(env)->real_size))
			exit_error(ERROR_STATUS_WORD_BUFFER_OVERFLOW, NULL);
	get_segment(env)->buffer[index] = byte;
	get_segment(env)->size++;
	get_segment(env)->pointer_address++;
}

void segment_compile(struct environment *env, unsigned long long value,
	int size)
{
	int dir = get_segment(env)->data_endianness ? -1 : 1;
	int index = get_segment(env)->data_endianness * (size - 1);
	for(; index < size && index >= 0; index += dir)
		segment_compile_byte(env, value >> index * 8);
}

unsigned long long convert_address(struct environment *env,
	unsigned long long address, int size)
{
	unsigned long long pointer_address =
		get_segment(env)->pointer_address;
	unsigned long long base_address = get_segment(env)->base_address;
	unsigned long long base_offset = get_segment(env)->base_offset;
	if(address < base_address || address + size > pointer_address)
		exit_error(ERROR_STATUS_INVALID_ACCESS, NULL);
	return address - base_address + base_offset;
}

unsigned long long segment_read(struct environment *env,
	unsigned long long address, int size)
{
	unsigned long long value = 0;
	int dir = get_segment(env)->data_endianness ? 1 : -1;
	int index = get_segment(env)->data_endianness ? 0 : (size - 1);
	address = convert_address(env, address, size);
	for(; index < size && index >= 0; index += dir)
		value = (value << 8) +
			(get_segment(env)->buffer[address + index] & 0xFF);
	return value;
}

void segment_write(struct environment *env, unsigned long long value,
	unsigned long long address, int size)
{
	int dir = get_segment(env)->data_endianness ? -1 : 1;
	int index = get_segment(env)->data_endianness * (size - 1);
	address = convert_address(env, address, size);
	for(; index < size && index >= 0; index += dir)
	{
		get_segment(env)->buffer[address + index] = value;
		value >>= 8;
	}
}

void translator_drop(struct environment *env)
{
	pop_stack_elem(&env->stack);
}

void translator_dup(struct environment *env)
{
	unsigned long long value = pop_stack_elem(&env->stack);
	push_stack_elem(&env->stack, value);
	push_stack_elem(&env->stack, value);
}

void translator_over(struct environment *env)
{
	unsigned long long value_one = pop_stack_elem(&env->stack);
	unsigned long long value_two = pop_stack_elem(&env->stack);
	push_stack_elem(&env->stack, value_two);
	push_stack_elem(&env->stack, value_one);
	push_stack_elem(&env->stack, value_two);
}

void translator_swap(struct environment *env)
{
	unsigned long long value_one = pop_stack_elem(&env->stack);
	unsigned long long value_two = pop_stack_elem(&env->stack);
	push_stack_elem(&env->stack, value_one);
	push_stack_elem(&env->stack, value_two);
}

void translator_add(struct environment *env)
{
	unsigned long long first = pop_stack_elem(&env->stack);
	unsigned long long second = pop_stack_elem(&env->stack);
	push_stack_elem(&env->stack, second + first);
}

void translator_sub(struct environment *env)
{
	unsigned long long first = pop_stack_elem(&env->stack);
	unsigned long long second = pop_stack_elem(&env->stack);
	push_stack_elem(&env->stack, second - first);
}

void translator_mul(struct environment *env)
{
	unsigned long long first = pop_stack_elem(&env->stack);
	unsigned long long second = pop_stack_elem(&env->stack);
	push_stack_elem(&env->stack, second * first);
}

void translator_div(struct environment *env)
{
	unsigned long long first = pop_stack_elem(&env->stack);
	unsigned long long second = pop_stack_elem(&env->stack);
	push_stack_elem(&env->stack, second / first);
}

void translator_mod(struct environment *env)
{
	unsigned long long first = pop_stack_elem(&env->stack);
	unsigned long long second = pop_stack_elem(&env->stack);
	push_stack_elem(&env->stack, second % first);
}

void translator_define_identifier(struct environment *env)
{
	unsigned long long value = pop_stack_elem(&env->stack);
	add_identifier(&env->id_list, read_lexem(), value);
}

void translator_compile_one(struct environment *env)
{
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_compile(env, value, 1);
}

void translator_compile_two(struct environment *env)
{
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_compile(env, value, 2);
}

void translator_compile_four(struct environment *env)
{
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_compile(env, value, 4);
}

void translator_compile_eight(struct environment *env)
{
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_compile(env, value, 8);
}

void translator_compile(struct environment *env)
{
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_compile(env, value, get_segment(env)->data_size);
}

void translator_reserve(struct environment *env)
{
	unsigned long long count = pop_stack_elem(&env->stack);
	while(count--)
		segment_compile(env, 0, 1);
}

void translator_read_one(struct environment *env)
{
	unsigned long long value = segment_read(env,
		pop_stack_elem(&env->stack), 1);
	push_stack_elem(&env->stack, value);
}

void translator_read_two(struct environment *env)
{
	unsigned long long value = segment_read(env,
		pop_stack_elem(&env->stack), 2);
	push_stack_elem(&env->stack, value);
}

void translator_read_four(struct environment *env)
{
	unsigned long long value = segment_read(env,
		pop_stack_elem(&env->stack), 4);
	push_stack_elem(&env->stack, value);
}

void translator_read_eight(struct environment *env)
{
	unsigned long long value = segment_read(env,
		pop_stack_elem(&env->stack), 8);
	push_stack_elem(&env->stack, value);
}

void translator_read(struct environment *env)
{
	unsigned long long value = segment_read(env,
		pop_stack_elem(&env->stack), get_segment(env)->data_size);
	push_stack_elem(&env->stack, value);
}

void translator_write_one(struct environment *env)
{
	unsigned long long address = pop_stack_elem(&env->stack);
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_write(env, value, address, 1);
}

void translator_write_two(struct environment *env)
{
	unsigned long long address = pop_stack_elem(&env->stack);
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_write(env, value, address, 2);
}

void translator_write_four(struct environment *env)
{
	unsigned long long address = pop_stack_elem(&env->stack);
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_write(env, value, address, 4);
}

void translator_write_eight(struct environment *env)
{
	unsigned long long address = pop_stack_elem(&env->stack);
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_write(env, value, address, 8);
}

void translator_write(struct environment *env)
{
	unsigned long long address = pop_stack_elem(&env->stack);
	unsigned long long value = pop_stack_elem(&env->stack);
	segment_write(env, value, address, get_segment(env)->data_size);
}

void translator_create_segment(struct environment *env)
{
	push_stack_elem(&env->stack, create_segment(env));
}

void translator_choose_segment(struct environment *env)
{
	env->chosen_segment = pop_stack_elem(&env->stack);
}

void translator_set_base(struct environment *env)
{
	get_segment(env)->base_address = pop_stack_elem(&env->stack);
	get_segment(env)->pointer_address = get_segment(env)->base_address;
	get_segment(env)->base_offset = get_segment(env)->size;
}

#define WORD_SIZE_MAX 8

void translator_set_data_size(struct environment *env)
{
	int target_size = pop_stack_elem(&env->stack);
	if(target_size <= 0 || target_size > WORD_SIZE_MAX)
		exit_error(ERROR_STATUS_INVALID_WORD_SIZE, NULL);
	get_segment(env)->data_size = target_size;
}

void translator_set_data_endianness(struct environment *env)
{
	int endianness = pop_stack_elem(&env->stack);
	if(endianness > 1)
		exit_error(ERROR_STATUS_WORD_ENDIANNESS_INVALID, NULL);
	get_segment(env)->data_endianness = endianness;
}

void translator_get_offset(struct environment *env)
{
	push_stack_elem(&env->stack, get_segment(env)->pointer_address);
}

void translator_get_base(struct environment *env)
{
	push_stack_elem(&env->stack, get_segment(env)->base_address);
}

void translator_print(struct environment *env)
{
	unsigned long long value = pop_stack_elem(&env->stack);
	fprintf(stderr, "%llx\n", value);
}

const struct operator translator_operators[] =
{
	{ &translator_drop, "?drop" },
	{ &translator_dup, "?dup" },
	{ &translator_over, "?over" },
	{ &translator_swap, "?swap" },
	{ &translator_add, "?+" },
	{ &translator_sub, "?-" },
	{ &translator_mul, "?*" },
	{ &translator_div, "?/" },
	{ &translator_mod, "?mod" },
	{ &translator_define_identifier, "??" },
	{ &translator_compile_one, "?'" },
	{ &translator_compile_one, "?1." },
	{ &translator_compile_two, "?2." },
	{ &translator_compile_four, "?4." },
	{ &translator_compile_eight, "?8." },
	{ &translator_compile, "?." },
	{ &translator_reserve, "?res" },
	{ &translator_read_one, "?1@" },
	{ &translator_read_two, "?2@" },
	{ &translator_read_four, "?4@" },
	{ &translator_read_eight, "?8@" },
	{ &translator_read, "?@" },
	{ &translator_write_one, "?1!" },
	{ &translator_write_two, "?2!" },
	{ &translator_write_four, "?4!" },
	{ &translator_write_eight, "?8!" },
	{ &translator_write, "?!" },
	{ &translator_create_segment, "?create" },
	{ &translator_choose_segment, "?choose" },
	{ &translator_set_base, "?org" },
	{ &translator_set_data_size, "?size" },
	{ &translator_set_data_endianness, "?endianness" },
	{ &translator_get_offset, "?$" },
	{ &translator_get_base, "?$$" },
	{ &translator_print, "?print" },
};

int execute_operator(struct environment *env, const char *lexem)
{
	int operators_count =
		sizeof(translator_operators) / sizeof(*translator_operators);
	int index;

	for(index = 0; index < operators_count; index++)
	{
		if(compare_lexems(lexem, translator_operators[index].lexem))
		{
			(*translator_operators[index].func)(env);
			break;
		}
	}

	return index != operators_count;
}

void print_target_buffer(struct environment *env)
{
	struct segment *seg_list;
	unsigned long long index;
	for(seg_list = env->seg_list; seg_list; seg_list = seg_list->next)
		for(index = 0; index < seg_list->size; index++)
			putchar(seg_list->buffer[index]);
}

int main()
{
	struct environment env;
	char *lexem;

	initialize_environment(&env);

	for(;; free(lexem))
	{
		lexem = read_lexem();
		if(!lexem)
			break;
		if(find_identifier(&env, lexem))
			continue;
		if(is_lexem_hex(&env, lexem))
			continue;
		if(!execute_operator(&env, lexem))
			exit_error(ERROR_STATUS_UNDEFINED, lexem);
	}

	print_target_buffer(&env);

	return 0;
}
