#include "prelude.c"

#define MEMORY_SIZE   4096
#define OPCODES_COUNT 13

#define X_OPCODES \
	X_OPCODE(LOAD, 0x1, true)     \
	X_OPCODE(STORE, 0x2, true)    \
	X_OPCODE(ADD, 0x3, true)      \
	X_OPCODE(SUBT, 0x4, true)     \
	X_OPCODE(INPUT, 0x5, false)   \
	X_OPCODE(OUTPUT, 0x6, false)  \
	X_OPCODE(HALT, 0x7, false)    \
	X_OPCODE(SKIPCOND, 0x8, true) \
	X_OPCODE(JUMP, 0x9, true)     \
	X_OPCODE(JNS, 0x0, true)      \
	X_OPCODE(CLEAR, 0xA, false)   \
	X_OPCODE(ADDI, 0xB, true)     \
	X_OPCODE(JUMPI, 0xC, true)    \

typedef enum Opcode {
#define X_OPCODE(name, value, _) OPCODE_##name = value,
	X_OPCODES
#undef X_OPCODE
} Opcode;

char const* opcode_names[] = {
#define X_OPCODE(name, _1, _2) [OPCODE_##name] = #name,
	X_OPCODES
#undef X_OPCODE
};

bool opcode_unary[] = {
#define X_OPCODE(name, _1, unary) [OPCODE_##name] = unary,
	X_OPCODES
#undef X_OPCODE
};

// TODO: Use a lookup table.
Opcode opcode_from_string(String s) {
	if (s.len < 3 || 8 < s.len)
		return -1;

	if      (string_equal(s, S("LOAD")))     return 0x1;
	else if (string_equal(s, S("STORE")))    return 0x2;
	else if (string_equal(s, S("ADD")))      return 0x3;
	else if (string_equal(s, S("SUBT")))     return 0x4;
	else if (string_equal(s, S("INPUT")))    return 0x5;
	else if (string_equal(s, S("OUTPUT")))   return 0x6;
	else if (string_equal(s, S("HALT")))     return 0x7;
	else if (string_equal(s, S("SKIPCOND"))) return 0x8;
	else if (string_equal(s, S("JUMP")))     return 0x9;
	else if (string_equal(s, S("JNS")))      return 0x0;
	else if (string_equal(s, S("CLEAR")))    return 0xA;
	else if (string_equal(s, S("ADDI")))     return 0xB;
	else if (string_equal(s, S("JUMPI")))    return 0xC;

	return -1;
}

typedef enum SkipCondOperand {
	SKIPCOND_LT = 0x000,
	SKIPCOND_EQ = 0x400,
	SKIPCOND_GT = 0x800,
} SkipCond_Operand;

typedef struct Instruction {
	Opcode opcode;
	u16    operand;
} Instruction;

typedef struct Parser {
	String src;
	isize  cursor;
	isize  line;

	u16 program_addr;

	isize   sym_len;
	isize   sym_cap;
	u32*    sym_hashes;
	String* sym_names;
	u16*    sym_addrs;

	isize        inst_len;
	isize        inst_cap;
	Instruction* inst;
} Parser;

void parser_init(Parser* pr, String src) {
	pr->src = src;
	pr->cursor = 0;
	pr->line = 1;

	// TODO: Handle ORG directive.
	pr->program_addr = 0;

	pr->sym_len = 0;
	pr->sym_cap = 8;
	// TODO: Implement symbol maps.
	// pr->sym_hashes;
	// pr->sym_names;
	// pr->sym_addrs;

	pr->inst_len = 0;
	pr->inst_cap = 8;
	pr->inst = malloc(pr->inst_cap * sizeof(*pr->inst));
}

void parser_free(Parser* pr) {
	free(pr->inst);
}

void parser_append_inst(Parser* pr, Instruction inst) {
	if (pr->inst_len < pr->inst_cap) {
		isize new_cap = pr->inst_cap;
		while (new_cap < pr->inst_len)
			new_cap *= 2;
		pr->inst_cap = new_cap;
		pr->inst = realloc(pr->inst, pr->inst_cap * sizeof(*pr->inst));
		assert(pr->inst != NULL);
	}

	pr->inst[pr->inst_len] = inst;
	pr->inst_len += 1;
}

u64 parse_uint_base16(String s, isize* n) {
	u64 r = 0;
	for (isize i = 0; i < s.len; ++i) {
		u8 c = s.data[i];
		if ('a' <= c && c <= 'f')
			c -= 32;
		if ('A' <= c && c <= 'F')
			c -= 7;
		c -= '0';

		if (c < 0 || 15 < c) {
			*n = i;
			break;
		}

		r = (r * 16) + c;
	}
	return r;
}

int main(int argc, char** argv) {
	String input = {0};
	{
		FILE* file = stdin;
		if (argc > 1) {
			char const* file_name = argv[1];
			file = fopen(file_name, "rb");
			if (file == NULL) {
				fprintf(stderr, "error: failed to open file %s: %s\n",
					file_name, strerror(errno));
			}
		}

		fseek(file, 0, SEEK_END);
		input.len = ftell(file);
		rewind(file);

		input.data = malloc(input.len * sizeof(*input.data));
		isize nread = fread(input.data, sizeof(*input.data), input.len, file);
		if (nread != input.len) {
			fprintf(stderr, "error: failed to read input (expected: %td, read: %td)",
				input.len, nread);
			return 1;
		}

		if (file != stdin)
			fclose(file);
	}

	Parser parser;
	parser_init(&parser, input);

	while (parser.cursor < parser.src.len) {
		String line = SZ(parser.src, parser.cursor);
		isize delim = string_index_byte(line, '\n');
		if (delim >= 0)
			line = SS(line, 0, delim);

		isize cdelim = string_index_byte(line, '/');
		if (cdelim >= 0)
			line = SS(line, 0, cdelim);

		line = string_trim_space(line);

		isize ldelim = string_index_byte(line, ',');
		if (ldelim == 0) {
			// TODO: Label name must preceed ','.
		} else if (ldelim >= 0) {
			// TODO: Handle labels.

			line = SZ(line, ldelim + 1);
			line = string_trim_space(line);
		}

		isize end = 0;
		while (is_alpha(line.data[end]))
			end += 1;

		// TODO: Don't do this.
		String opcode_arg = string_to_upper(SS(line, 0, end));
		Opcode opcode = opcode_from_string(opcode_arg);
		string_free(&opcode_arg);

		if (opcode == -1) {
			// TODO: Invalid opcode, try directive.
		} else {
			Instruction inst;
			inst.opcode = opcode;

			if (opcode_unary[opcode]) {
				line = SZ(line, end);
				line = string_trim_space(line);

				isize n = -1;
				u64 val = parse_uint_base16(line, &n);
				if (n >= 0) {
					// TODO: Invalid number, try label.
				} else if (val >= MEMORY_SIZE) {
					// TODO: Address out of bounds.
				} else {
					inst.operand = (u16)val;
				}
			}

			parser_append_inst(&parser, inst);
		}

		parser.line += 1;
		parser.cursor += delim + 1;
	}

	for (isize i = 0; i < parser.inst_len; ++i) {
		Instruction inst = parser.inst[i];
		if (opcode_unary[inst.opcode])
			printf("%s %03X\n", opcode_names[inst.opcode], inst.operand);
		else
			printf("%s\n", opcode_names[inst.opcode]);
	}

	string_free(&input);

	return 0;
}
