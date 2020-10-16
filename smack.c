/*
	Simple Macro Generator
	created by exegete
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct macro
{
	const char *lexem;
	unsigned long long param_count;
	const char *text;
	struct macro *next;
};

struct macro_parameter
{
	char *lexem;
	struct macro_parameter *next;
};

struct input_stream
{
	FILE *fd;
	const struct macro *macro;
	unsigned long long text_offset;
	struct macro_parameter *param_list;
	struct input_stream *next;
};

struct module
{
	const char *file_name;
	struct module *next;
};

#define ERROR_STATUS_ARG 1
#define ERROR_STATUS_OPEN 2
#define ERROR_STATUS_CHDIR 3
#define ERROR_STATUS_MALLOC 4
#define ERROR_STATUS_EMPTY_STREAM_LIST 5
#define ERROR_STATUS_INVALID_STREAM 6
#define ERROR_STATUS_INVALID_MACRO 7
#define ERROR_STATUS_INVALID_PARAMETER 8
#define ERROR_STATUS_INVALID_CALL 9
#define ERROR_STATUS_INVALID_INCLUDE 10
#define ERROR_STATUS_INVALID_SHIELD 11

const char *error_msgs[] =
{
	"Please, specify input file",
	"Unable to open file: ",
	"Unable to change working directory: ",
	"Unable to allocate memory",
	"Input stream list is empty",
	"Input stream is invalid",
	"Macro definition is invalid",
	"Macro parameter is invalid",
	"Macro call is invalid",
	"Invalid include/module argument",
	"Invalid '#' argument"
};

void exit_error(int status, const char *msg)
{
	if(msg)
		fprintf(stderr, "Error: %s%s\n", error_msgs[status - 1], msg);
	else
		fprintf(stderr, "Error: %s\n", error_msgs[status - 1]);
	exit(status);
}

void add_input_stream(struct input_stream **stream_list,
	FILE *fd, const struct macro *macro)
{
	struct input_stream *head = *stream_list;
	*stream_list = malloc(sizeof(struct input_stream));
	if(!*stream_list)
		exit_error(ERROR_STATUS_MALLOC, NULL);
	(*stream_list)->fd = fd;
	(*stream_list)->macro = macro;
	(*stream_list)->text_offset = 0;
	(*stream_list)->param_list = NULL;
	(*stream_list)->next = head;
}

int delete_input_stream(struct input_stream **stream_list)
{
	struct input_stream *head = *stream_list;
	if(!head)
		exit_error(ERROR_STATUS_EMPTY_STREAM_LIST, NULL);
	*stream_list = (*stream_list)->next;
	if(head->fd)
		fclose(head->fd);
	while(head->param_list)
	{
		struct macro_parameter *param_head = head->param_list;
		head->param_list = param_head->next;
		free(param_head->lexem);
		free(param_head);
	}
	free(head);
	return *stream_list != NULL;
}

void add_input_stream_parameter(struct input_stream *stream_list, char *lexem)
{
	struct macro_parameter **param = &stream_list->param_list;
	while(*param)
		param = &(*param)->next;
	*param = malloc(sizeof(struct macro_parameter));
	if(!*param)
		exit_error(ERROR_STATUS_MALLOC, NULL);
	(*param)->lexem = lexem;
	(*param)->next = NULL;
}

const char *get_input_stream_parameter(struct input_stream *stream_list,
	unsigned long long chosen)
{
	struct macro_parameter *param = stream_list->param_list;
	if(!chosen)
		return stream_list->macro->lexem;
	chosen--;
	for(; param && chosen > 0; chosen--)
		param = param->next;
	if(!param)
		exit_error(ERROR_STATUS_INVALID_CALL, NULL);
	return param->lexem;
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

int read_symb_from_text(struct input_stream *stream_list)
{
	char symb;
	if(!stream_list->macro)
		exit_error(ERROR_STATUS_INVALID_STREAM, NULL);
	if(!stream_list->macro->text)
		exit_error(ERROR_STATUS_INVALID_STREAM, NULL);
	symb = stream_list->macro->text[stream_list->text_offset];
	if(!symb)
		return EOF;
	stream_list->text_offset++;
	return symb;
}

#define COMMENT_SYMB ';'

int read_real_symb(struct input_stream *stream_list)
{
	int symb;

	if(!stream_list)
		exit_error(ERROR_STATUS_EMPTY_STREAM_LIST, NULL);

	if(stream_list->fd)
		symb = fgetc(stream_list->fd);
	else
		symb = read_symb_from_text(stream_list);

	if(symb == COMMENT_SYMB)
	{
		for(; symb != '\n' && symb != EOF;
			symb = fgetc(stream_list->fd))
		{}
	}
	return symb;
}

#define SHIELD_SYMB '\\'

int read_symb(struct input_stream *stream_list, int *shield)
{
	int symb = read_real_symb(stream_list);
	if(shield)
	{
		*shield = 0;
		if(symb == SHIELD_SYMB)
		{
			symb = read_real_symb(stream_list);
			*shield = 1;
		}
	}
	return symb;
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

int is_lexem_hex(const char *lexem, unsigned long long *dest)
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
	if(dest)
		*dest = hex;
	return 1;
}

int compare_lexems(const char *lexem_one, const char *lexem_two)
{
	for(; *lexem_one && *lexem_two; lexem_one++, lexem_two++)
		if(*lexem_one != *lexem_two)
			return 0;
	return *lexem_one == *lexem_two;
}

char *copy_lexem(const char *lexem)
{
	char *buffer = NULL;
	unsigned long long buffer_size = 0;
	unsigned long long index;

	for(index = 0;; index++)
	{
		if(index == buffer_size)
			if(!double_buffer(&buffer, &buffer_size))
				exit_error(ERROR_STATUS_MALLOC, NULL);
		buffer[index] = lexem[index];
		if(!lexem[index])
			break;
	}
	return buffer;
}

#define EVAL_SYMB '%'

unsigned long long get_eval_end(const char *lexem)
{
	unsigned long long counter = 0;

	for(; *lexem && *lexem != EVAL_SYMB; lexem++, counter++)
	{}
	if(!*lexem)
		exit_error(ERROR_STATUS_INVALID_CALL, NULL);
	return counter;
}

char *param_eval(struct input_stream *stream_list, char *lexem)
{
	char *input_lexem = lexem;
	char *buffer = NULL;
	unsigned long long buffer_size = 0;
	const char *param = NULL;
	unsigned long long index = 0;

	for(;;)
	{
		char symb = param && *param ? *param : *lexem++;
		param = !param || !*param ? NULL : param + 1;
		if(symb == EVAL_SYMB)
		{
			unsigned long long param_offset = get_eval_end(lexem);
			unsigned long long param_num;
			*(lexem + param_offset) = 0;
			if(!is_lexem_hex(lexem, &param_num))
				exit_error(ERROR_STATUS_INVALID_CALL, NULL);
			lexem += param_offset + 1;
			param = get_input_stream_parameter(stream_list,
				param_num);
			continue;
		}
		if(index == buffer_size)
			if(!double_buffer(&buffer, &buffer_size))
				exit_error(ERROR_STATUS_MALLOC, NULL);
		buffer[index] = symb;
		if(!symb)
			break;
		index++;
	}
	free(input_lexem);
	return buffer;
}

int is_separator(int symb)
{
	return symb == ' ' || symb == '\n' || symb == '\t' || symb == '\r';
}

char *read_lexem(struct input_stream *stream_list)
{
	char *buffer = NULL;
	unsigned long long buffer_size = 0;
	int symb;
	int shield;
	unsigned long long index;

	for(;;)
	{
		symb = read_symb(stream_list, &shield);
		if(is_separator(symb) && !shield)
			continue;
		if(symb == EOF)
			return NULL;
		break;
	}

	for(index = 0;; index++)
	{
		if(index == buffer_size)
			if(!double_buffer(&buffer, &buffer_size))
				exit_error(ERROR_STATUS_MALLOC, NULL);
		if(symb == EOF || (is_separator(symb) && !shield))
		{
			buffer[index] = 0;
			break;
		}
		buffer[index] = symb;
		symb = read_symb(stream_list, &shield);
	}
	buffer = param_eval(stream_list, buffer);
	return buffer;
}

#define MACRO_END_SYMB ']'

const char *read_macro_text(struct input_stream *stream_list)
{
	char *buffer = NULL;
	unsigned long long buffer_size = 0;
	unsigned long long index;

	for(index = 0;; index++)
	{
		int shield;
		int symb = read_symb(stream_list, &shield);
		if(index == buffer_size)
			if(!double_buffer(&buffer, &buffer_size))
				exit_error(ERROR_STATUS_MALLOC, NULL);
		if(symb == EOF || (symb == MACRO_END_SYMB && !shield))
		{
			buffer[index] = 0;
			break;
		}
		buffer[index] = symb;
	}
	return buffer;
}

void create_macro(struct input_stream *stream_list, struct macro **macro_list)
{
	char *param_count_lexem;
	struct macro *head = *macro_list;
	*macro_list = malloc(sizeof(struct macro));
	if(!*macro_list)
		exit_error(ERROR_STATUS_MALLOC, NULL);
	(*macro_list)->lexem = read_lexem(stream_list);
	if(!(*macro_list)->lexem)
		exit_error(ERROR_STATUS_INVALID_MACRO, NULL);	
	param_count_lexem = read_lexem(stream_list);
	if(!param_count_lexem || !is_lexem_hex(param_count_lexem,
		&(*macro_list)->param_count))
		exit_error(ERROR_STATUS_INVALID_MACRO, NULL);
	free(param_count_lexem);
	(*macro_list)->text = read_macro_text(stream_list);
	(*macro_list)->next = head;
}

int find_module(struct module *module_list, const char *file_name)
{
	for(; module_list; module_list = module_list->next)
		if(compare_lexems(module_list->file_name, file_name))
			return 1;
	return 0;
}

void add_module(struct module **module_list, const char *file_name)
{
	struct module *head = *module_list;
	*module_list = malloc(sizeof(struct module));
	if(!*module_list)
		exit_error(ERROR_STATUS_MALLOC, NULL);
	(*module_list)->file_name = file_name;
	(*module_list)->next = head;
}

void include_file(struct input_stream **stream_list,
	struct module **module_list)
{
	char *file_name = read_lexem(*stream_list);
	FILE *fd;
	if(!file_name)
		exit_error(ERROR_STATUS_INVALID_INCLUDE, NULL);
	if(module_list && find_module(*module_list, file_name))
	{
		free(file_name);
		return;
	}
	fd = fopen(file_name, "r");
	if(!fd)
		exit_error(ERROR_STATUS_OPEN, file_name);
	if(module_list)
		add_module(module_list, file_name);
	else
		free(file_name);
	add_input_stream(stream_list, fd, NULL);
}

void push_parameter(struct input_stream *input_stream,
	struct input_stream *actual_stream)
{
	char *lexem = read_lexem(input_stream);
	if(!lexem)
		exit_error(ERROR_STATUS_INVALID_PARAMETER, NULL);
	add_input_stream_parameter(actual_stream, lexem);
}

int find_macro(struct input_stream **stream_list, struct macro *macro_list,
	const char *lexem)
{
	for(; macro_list; macro_list = macro_list->next)
	{
		if(compare_lexems(macro_list->lexem, lexem))
		{
			struct input_stream *old_stream = *stream_list;
			unsigned long long param_count =
				macro_list->param_count;
			add_input_stream(stream_list, NULL, macro_list);
			for(; param_count > 0; param_count--)
				push_parameter(old_stream, *stream_list);
			return 1;
		}
	}
	return 0;
}

#define DIRECTORY_SEPARATOR_SYMB '/'

unsigned long long find_last_dir_separator_pos(const char *file_name)
{
	const char *orig_file_name = file_name;
	unsigned long long pos = 0;
	for(; *file_name; file_name++)
		if(*file_name == DIRECTORY_SEPARATOR_SYMB)
			pos = file_name - orig_file_name;
	return pos;
}

void initialize_stream(struct input_stream **stream_list,
	const char *file_name)
{
	char *dir_path = copy_lexem(file_name);
	unsigned long long dir_sep_pos = find_last_dir_separator_pos(dir_path);
	FILE *fd = fopen(file_name, "r");
	if(!fd)
		exit_error(ERROR_STATUS_OPEN, file_name);
	add_input_stream(stream_list, fd, NULL);
	if(dir_sep_pos)
	{
		dir_path[dir_sep_pos] = 0;
		if(chdir(dir_path) == -1)
			exit_error(ERROR_STATUS_CHDIR, NULL);
	}
	free(dir_path);
}

void print_lexem(const char *lexem)
{
	printf("%s ", lexem);
}

#define MACRO_SHIELD_LEXEM "#"
#define MACRO_START_LEXEM "["
#define MACRO_INCLUDE_LEXEM "include"
#define MACRO_MODULE_LEXEM "module"
#define MACRO_LIT_LEXEM "literal"

int main(int argc, char **argv)
{
	struct input_stream *stream_list = NULL;
	struct macro *macro_list = NULL;
	struct module *module_list = NULL;
	char *lexem;

	if(argc <= 1)
		exit_error(ERROR_STATUS_ARG, NULL);
	else
		initialize_stream(&stream_list, argv[1]);

	for(;; free(lexem))
	{
		lexem = read_lexem(stream_list);
		if(!lexem)
		{
			if(delete_input_stream(&stream_list))
				continue;
			else
				break;
		}
		if(compare_lexems(lexem, MACRO_SHIELD_LEXEM))
		{
			free(lexem);
			lexem = read_lexem(stream_list);
			if(!lexem)
				exit_error(ERROR_STATUS_INVALID_SHIELD, NULL);
			print_lexem(lexem);
			continue;
		}
		if(compare_lexems(lexem, MACRO_START_LEXEM))
		{
			create_macro(stream_list, &macro_list);
			continue;
		}
		if(compare_lexems(lexem, MACRO_INCLUDE_LEXEM))
		{
			include_file(&stream_list, NULL);
			continue;
		}
		if(compare_lexems(lexem, MACRO_MODULE_LEXEM))
		{
			include_file(&stream_list, &module_list);
			continue;
		}
		if(is_lexem_hex(lexem, NULL))
		{
			if(find_macro(&stream_list, macro_list,
				MACRO_LIT_LEXEM))
			{
				add_input_stream_parameter(stream_list,
					copy_lexem(lexem));
				continue;
			}
		}
		if(find_macro(&stream_list, macro_list, lexem))
			continue;
		print_lexem(lexem);
	}

	return 0;
}
