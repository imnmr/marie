#include "prelude.c"

#define MEMORY_SIZE   4096
#define OPCODES_COUNT 13

#define X_OPCODES \
	X_OPCODE(OPCODE_LOAD,     "LOAD",     0x1, true)  \
	X_OPCODE(OPCODE_STORE,    "STORE",    0x2, true)  \
	X_OPCODE(OPCODE_ADD,      "ADD",      0x3, true)  \
	X_OPCODE(OPCODE_SUBT,     "SUBT",     0x4, true)  \
	X_OPCODE(OPCODE_INPUT,    "INPUT",    0x5, false) \
	X_OPCODE(OPCODE_OUTPUT,   "OUTPUT",   0x6, false) \
	X_OPCODE(OPCODE_HALT,     "HALT",     0x7, false) \
	X_OPCODE(OPCODE_SKIPCOND, "SKIPCOND", 0x8, true)  \
	X_OPCODE(OPCODE_JUMP,     "JUMP",     0x9, true)  \
	X_OPCODE(OPCODE_JNS,      "JNS",      0x0, true)  \
	X_OPCODE(OPCODE_CLEAR,    "CLEAR",    0xA, false) \
	X_OPCODE(OPCODE_ADDI,     "ADDI",     0xB, true)  \
	X_OPCODE(OPCODE_JUMPI,    "JUMPI",    0xC, true)  \

typedef enum Opcode {
	OPCODE_INVALID = -1,
#define X_OPCODE(e, _1, value, _2) e = value,
	X_OPCODES
#undef X_OPCODE
} Opcode;

char const* opcode_names[] = {
#define X_OPCODE(e, name, _1, _2) [e] = name,
	X_OPCODES
#undef X_OPCODE
};

bool opcode_unary[] = {
#define X_OPCODE(e, _1, _2, unary) [e] = unary,
	X_OPCODES
#undef X_OPCODE
};

// TODO: Use a lookup table.
Opcode opcode_from_string(String s) {
	if (s.len < 3 || 8 < s.len)
		return OPCODE_INVALID;

	if (false) /* noop */;
#define X_OPCODE(e, name, _1, _2) else if (string_equal(s, S(name))) return e;
	X_OPCODES
#undef X_OPCODE

	return OPCODE_INVALID;
}

typedef enum SkipCondOperand {
	SKIPCOND_LT = 0x000,
	SKIPCOND_EQ = 0x400,
	SKIPCOND_GT = 0x800,
} SkipCondOperand;

#define X_DIRECTIVES \
	X_DIRECTIVE(DIRECTIVE_ORG, "ORG") \
	X_DIRECTIVE(DIRECTIVE_DEC, "DEC") \
	X_DIRECTIVE(DIRECTIVE_HEX, "HEX")

typedef enum Directive {
	DIRECTIVE_INVALID,
#define X_DIRECTIVE(e, _) e,
	X_DIRECTIVES
#undef X_DIRECTIVE
} Directive;

char const* directive_names[] = {
#define X_DIRECTIVE(e, name) [e] = name,
	X_DIRECTIVES
#undef X_DIRECTIVE
};

Directive directive_from_string(String s) {
	if (s.len != 3)
		return DIRECTIVE_INVALID;

	if (false) /* no-op */;
#define X_DIRECTIVE(e, name) else if (string_equal(s, S(#name))) return e;
	X_DIRECTIVES
#undef X_DIRECTIVES

	return DIRECTIVE_INVALID;
}

#define X_TOKENS \
	X_TOKEN(EOF)       \
	X_TOKEN(INVALID)   \
	X_TOKEN(OPCODE)    \
	X_TOKEN(DIRECTIVE) \
	X_TOKEN(IDENT)     \
	X_TOKEN(NUMBER)    \
	X_TOKEN(COMMA)     \

typedef enum TokenKind {
#define X_TOKEN(name) TOKEN_##name,
	X_TOKENS
#undef X_TOKEN
} TokenKind;

char const* token_names[] = {
#define X_TOKEN(name) [TOKEN_##name] = #name,
	X_TOKENS
#undef X_TOKEN
};

typedef struct Token {
	TokenKind kind;
	isize     off;
	isize     len;
	isize     line;
	isize     line_off;
} Token;

typedef struct Tokenizer {
	String src;
	isize  cursor;
	isize  line;
	isize  line_off;

	isize  tokens_len;
	isize  tokens_cap;
	Token* tokens; 
} Tokenizer;

void tokenizer_init(Tokenizer *tz, String src) {
	tz->src = src;
	tz->cursor = 0;
	tz->line = 1;
	tz->line_off = 1;
	tz->tokens_len = 0;
	tz->tokens_cap = 8;
	tz->tokens = malloc(tz->tokens_cap * sizeof(*tz->tokens));
}

void tokenizer_free(Tokenizer* tz) {
	free(tz->tokens);
}

void tokenizer_append(Tokenizer* tz, TokenKind kind, isize off, isize len) {
	tz->tokens_len += 1;

	isize new_cap = tz->tokens_cap;
	while (new_cap < tz->tokens_len)
		new_cap *= 2;

	if (new_cap != tz->tokens_cap) {
		tz->tokens_cap = new_cap;
		tz->tokens = realloc(tz->tokens, tz->tokens_cap * sizeof(*tz->tokens));
		assert(tz->tokens != NULL);
	}

	Token t;
	t.kind = kind;
	t.off = off;
	t.len = len;
	t.line = tz->line;
	t.line_off = tz->line_off;

	tz->tokens[tz->tokens_len - 1] = t;
}

bool tokenizer_eof(Tokenizer const* tz) {
	return tz->cursor >= tz->src.len;
}

u8 tokenizer_curr(Tokenizer const* tz) {
	if (tokenizer_eof(tz))
		return 0;
	return tz->src.data[tz->cursor];
}

void tokenizer_consume(Tokenizer* tz) {
	tz->cursor += 1;
}

void tokenizer_advance(Tokenizer* tz) {
	u8 c = tokenizer_curr(tz);
	while (is_space(c) || c == '/') {
		if (c == '\n') {
			tz->line += 1;
			tz->line_off = tz->cursor + 1;
		} else if (c == '/') {
			while (c != '\n' && !tokenizer_eof(tz)) {
				tokenizer_consume(tz);
				c = tokenizer_curr(tz);
			}
			continue;
		}

		tokenizer_consume(tz);
		c = tokenizer_curr(tz);
	}
}

bool tokenizer_next(Tokenizer* tz) {
	tokenizer_advance(tz);

	if (tokenizer_eof(tz))
		return false;

	u8 c = tokenizer_curr(tz);
	isize start = tz->cursor;

	if (c == ',') {
		tokenizer_consume(tz);
		tokenizer_append(tz, TOKEN_COMMA, start, 1);
		return true;
	}

	while (is_alnum(c)) {
		tokenizer_consume(tz);
		c = tokenizer_curr(tz);
	}

	if (tz->cursor == start) {
		tokenizer_consume(tz);
		tokenizer_append(tz, TOKEN_INVALID, start, 1);
		return true;
	}

	String s = string_to_upper(SS(tz->src, start, tz->cursor));

	Opcode opcode = opcode_from_string(s);
	if (opcode != OPCODE_INVALID) {
		tokenizer_append(tz, TOKEN_OPCODE, start, s.len);
		string_free(&s);
		return true;
	}

	Directive dire = directive_from_string(s);
	if (dire != DIRECTIVE_INVALID) {
		tokenizer_append(tz, TOKEN_DIRECTIVE, start, s.len);
		string_free(&s);
		return true;
	}

	if (s.len > 0 && is_digit(s.data[0])) {
		isize i = 1;
		while (i < s.len && is_hex_digit(s.data[i]))
			i += 1;
		if (i == s.len) {
			tokenizer_append(tz, TOKEN_NUMBER, start, s.len);
			string_free(&s);
			return true;
		}
		tokenizer_append(tz, TOKEN_INVALID, start, s.len);
		string_free(&s);
		return false;
	}

	tokenizer_append(tz, TOKEN_IDENT, start, s.len);
	string_free(&s);

	return true;
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

	Tokenizer tz;
	tokenizer_init(&tz, input);

	while (tokenizer_next(&tz))
		/* no-op */;

	printf("Tokens: %td\n", tz.tokens_len);
	for (isize i = 0; i < tz.tokens_len; ++i) {
		Token t = tz.tokens[i];
		printf("[%td]: %s '%.*s'\n", i, token_names[t.kind],
		       (int)t.len, &tz.src.data[t.off]);
	}

	tokenizer_free(&tz);
	string_free(&input);

	return 0;
}
