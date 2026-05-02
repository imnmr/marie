//
// General utilities
//

// TODO: Remove this
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define typeof(T) __typeof__(T)
#define cast(T, expr) (T)(expr)

#define TODO(s) assert(s && false);

#define SS(s, from, to) (typeof(s)){(to) - (from), &((s).data[from])}
#define SZ(s, from)     SS((s), (from), (s).len)

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef ptrdiff_t isize;
typedef size_t    usize;
typedef uintptr_t uintptr;

typedef float  f32;
typedef double f64;

typedef struct String {
	isize len;
	u8*   data;
} String;

#define S(s)     (String){sizeof(s) - 1, (u8*)(s)}
#define SL(s)    (String){strlen(s), (u8*)(s)}
#define SN(s, n) (String){(n), (u8*)(s)}
#define SF(s)    ((int)(s).len), ((s).data)

bool is_space    (u8 c);
bool is_alpha    (u8 c);
bool is_digit    (u8 c);
bool is_alnum    (u8 c);
bool is_hex_digit(u8 c);

String string_make      (isize n);
String string_clone     (String s);
void   string_free      (String* s);
void   string_resize    (String* s, isize n);
void   string_copy      (String* dst, String src);
bool   string_equal     (String s, String t);
bool   string_has_prefix(String s, String prefix);
bool   string_has_suffix(String s, String suffix);
isize  string_index     (String s, String t);
isize  string_index_byte(String s, u8 b);
bool   string_cut       (String s, String t, String* left, String* match, String* right);
String string_trim_space(String s);
String string_to_upper  (String s);

typedef struct StringBuilder {
	isize len;
	isize cap;
	u8*   data;
} StringBuilder;

String strb_to_string    (StringBuilder* sb);
char*  strb_to_cstring   (StringBuilder* sb);
void   strb_write_byte   (StringBuilder* sb, u8 c);
void   strb_write        (StringBuilder* sb, String s);
void   strb_write_cstring(StringBuilder* sb, char const* s);

u64 strconv_parse_uint(String s, isize base, isize* n);
i64 strconv_parse_int (String s, isize base, isize* n);

void printfln (char const* format, ...);
void eprintf  (char const* format, ...);
void eprintfln(char const* format, ...);

#define DYNAMIC_ARRAY(T, name) \
	typedef struct name {  \
		isize len;     \
		isize cap;     \
		T*    data;    \
	} name

#define da_init_cap(a, _cap) \
	do {                                                     \
		(a)->len = 0;                                    \
		(a)->cap = (_cap);                               \
		(a)->data = malloc((_cap) * sizeof(*(a)->data)); \
	} while (0)

#define da_free(a) free((a)->data)

#define da_append(a, item) \
	do {                                                               \
		isize new_cap = (a)->cap;                                  \
		while (new_cap < (a)->len + 1)                             \
			new_cap *= 2;                                      \
		if (new_cap != (a)->cap) {                                 \
			(a)->cap = new_cap;                                \
			(a)->data = realloc(                               \
				(a)->data, (a)->cap * sizeof(*(a)->data)); \
			assert((a)->data != NULL);                         \
		}                                                          \
		(a)->data[(a)->len] = (item);                              \
		(a)->len += 1;                                             \
	} while (0)

bool is_space(u8 c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool is_alpha(u8 c) {
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

bool is_digit(u8 c) {
	return '0' <= c && c <= '9';
}

bool is_alnum(u8 c) {
	return is_alpha(c) || is_digit(c);
}

bool is_hex_digit(u8 c) {
	return is_digit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

String string_make(isize n) {
	String dst = SN(malloc(n * sizeof(*dst.data)), n);
	return dst;
}

String string_clone(String s) {
	String dst = string_make(s.len);
	string_copy(&dst, s);
	return dst;
}

void string_free(String* s) {
	free(s->data);
}

void string_resize(String* s, isize n) {
	s->len = n;
	s->data = realloc(s->data, n * sizeof(*s->data));
	assert(s->data != NULL);
}

void string_copy(String* dst, String src) {
	memmove(dst->data, src.data, src.len);
}

bool string_equal(String s, String t) {
	if (s.len != t.len)
		return false;
	if (s.len == 0)
		return true;
	return memcmp(s.data, t.data, s.len) == 0;
}

bool string_has_prefix(String s, String prefix) {
	isize off = s.len - prefix.len;
	return off >= 0 && string_equal(SS(s, 0, prefix.len), prefix);
}

bool string_has_suffix(String s, String suffix) {
	isize off = s.len - suffix.len;
	return off >= 0 && string_equal(SZ(s, off), suffix);
}

u32 hash_rabin_karp(isize n, u8 const* data, u32* pow) {
	u32 hash = 0;
	for (isize i = 0; i < n; ++i)
		hash = hash * 16777619 + data[i];

	*pow = 1;
	u32 sqr = 16777619;
	for (isize i = n - 1; i > 0; i >>= 1) {
		if ((i & 1) != 0)
			*pow *= sqr;
		sqr *= sqr;
	}

	return hash;
}

isize string_index(String s, String t) {
	isize n = s.len;
	isize m = t.len;

	if (m == 0)
		return 0;
	if (m == 1)
		return string_index_byte(s, t.data[0]);
	if (m == n)
		return string_equal(s, t) ? 0 : -1;
	if (m > n)
		return -1;

	u32 pow;
	u32 target_hash = hash_rabin_karp(m, t.data, &pow);
	u32 window_hash = hash_rabin_karp(m, s.data, &pow);

	for (isize i = 0; i <= n - m; ++i) {
		if (target_hash == window_hash && memcmp(&s.data[i], t.data, m) == 0)
			return i;
		if (i < n - m)
			window_hash = (window_hash - (s.data[i] * pow)) * 16777619 + s.data[i + m];
	}

	return -1;
}

isize string_index_byte(String s, u8 c) {
	for (isize i = 0; i < s.len; ++i) {
		if (s.data[i] == c)
			return i;
	}
	return -1;
}

bool string_cut(String s, String t, String* left, String* match, String* right) {
	isize n = string_index(s, t);
	if (n == -1)
		return false;
	if (left != NULL)
		*left = SS(s, 0, n);
	if (match != NULL)
		*match = SS(s, n, t.len);
	if (right != NULL)
		*right = SZ(s, n + t.len);
	return true;
}

String string_trim_space(String s) {
	isize from = 0;
	while (from < s.len && is_space(s.data[from]))
		from += 1;

	isize to = s.len - 1;
	while (to >= 0 && is_space(s.data[to]))
		to -= 1;

	if (from > to)
		from = 0;

	return SS(s, from, to + 1);
}

String string_to_upper(String s) {
	String t = string_clone(s);
	for (isize i = 0; i < t.len; ++i) {
		if ('a' <= t.data[i] && t.data[i] <= 'z')
			t.data[i] -= 'a' - 'A';
	}
	return t;
}

String strb_to_string(StringBuilder* sb) {
	return SN(sb->data, sb->len);
}

char* strb_to_cstring(StringBuilder* sb) {
	strb_write_byte(sb, '\0');
	return cast(char*, sb->data);
}

void strb__try_grow(StringBuilder* sb, isize new_len) {
	if (sb->cap < new_len) {
		isize new_cap = sb->cap == 0 ? 64 : sb->cap;
		while (new_cap < new_len)
			new_cap *= 2;
		if (new_cap != sb->cap) {
			sb->cap = new_cap;
			sb->data = realloc(sb->data, sb->cap);
			assert(sb->data != NULL);
		}
	}
}

void strb_write_byte(StringBuilder* sb, u8 c) {
	strb__try_grow(sb, sb->len + 1);
	sb->data[sb->len] = c;
	sb->len += 1;
}

void strb_write(StringBuilder* sb, String s) {
	strb__try_grow(sb, sb->len + s.len);
	memmove(&sb->data[sb->len], s.data, s.len);
	sb->len += s.len;
}

void strb_write_cstring(StringBuilder* sb, char const* s) {
	isize len = strlen(s);
	strb__try_grow(sb, sb->len + len);
	memmove(&sb->data[sb->len], s, len);
	sb->len += len;
}

u64 strconv_parse_uint(String s, isize base, isize* n) {
	u64 r = 0;
	for (isize i = 0; i < s.len; ++i) {
		u8 c = s.data[i];
		if ('a' <= c && c <= 'z')
			c = c - 'a' + 10;
		else if ('A' <= c && c <= 'Z')
			c = c - 'A' + 10;
		else
			c -= '0';

		if (c < 0 || base - 1 < c) {
			if (n != NULL)
				*n = i;
			break;
		}

		r = r * base + c;
	}
	return r;
}

i64 strconv_parse_int(String s, isize base, isize* n) {
	i64 r = 0;
	i64 i = 0;

	if (s.data[0] == '-')
		i = 1;

	r = strconv_parse_uint(SZ(s, i), base, n);
	return i == 0 ? r : -r;
}

void printfln(char const* format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	printf("\n");
	va_end(args);
}

void eprintf(char const* format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

void eprintfln(char const* format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

//
// MARIE processor
//

#define MARIE_MEMORY_SIZE 4096
#define MARIE_MAX_ADDR    0xFFF
#define MARIE_MIN_INT     (-32768)
#define MARIE_MAX_INT     32767

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
	X_OPCODE(OPCODE_JUMPI,    "JUMPI",    0xC, true)

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

bool opcode_binary[] = {
#define X_OPCODE(e, _1, _2, binary) [e] = binary,
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
#define X_DIRECTIVE(e, name) else if (string_equal(s, S(name))) return e;
	X_DIRECTIVES
#undef X_DIRECTIVES

	return DIRECTIVE_INVALID;
}

// typedef enum InstructionCycle {
// 	INST_FETCH,
// 	INST_DECODE,
// 	INST_EXECUTE,
// } InstructionCycle;

typedef struct Processor {
	Opcode           opcode;
	// InstructionCycle cycle;

	u16 pc;  // program counter
	u16 ir;  // instruction register
	i16 ac;  // accumulator
	i16 mbr; // memory buffer register
	i16 mar; // memory address register
	i16 in;  // input register
	i16 out; // output register

	i16* memory;
} Processor;

void processor_init(Processor* p, i16* memory) {
	p->opcode = OPCODE_INVALID;
	// p->cycle  = INST_FETCH;
	p->pc     = 0;
	p->ir     = 0;
	p->ac     = 0;
	p->mbr    = 0;
	p->mar    = 0;
	p->in     = 0;
	p->out    = 0;
	p->memory = memory;
}

// TODO: Split one step to several microsteps, e.g., fetch, decode, and execute.
bool processor_step(Processor* p) {
	p->mar = p->pc;
	p->mbr = p->memory[p->mar];
	p->ir = p->mbr;
	
	p->opcode = p->ir >> 12;

	// printf("%04X -> [opcode: %s, operand: %03X]\n", p->ir, opcode_names[p->opcode], p->ir & 0xFFF);

	if (opcode_binary[p->opcode]) {
		u16 operand = p->ir & 0xFFF;
		if (p->opcode == OPCODE_LOAD) {
			p->mar = operand;
			p->mbr = p->memory[p->mar];
			p->ac = p->mbr;
		} else if (p->opcode == OPCODE_STORE) {
			p->mar = operand;
			p->mbr = p->ac;
			p->memory[p->mar] = p->mbr;
		} else if (p->opcode == OPCODE_ADD) {
			p->mar = operand;
			p->mbr = p->memory[p->mar];
			p->ac += p->mbr;
		} else if (p->opcode == OPCODE_SUBT) {
			p->mar = operand;
			p->mbr = p->memory[p->mar];
			p->ac -= p->mbr;
		} else if (p->opcode == OPCODE_SKIPCOND) {
			SkipCondOperand skipcond = cast(SkipCondOperand, operand);
			if (skipcond == SKIPCOND_LT) {
				if (p->ac < 0)
					p->pc += 1;
			} else if (skipcond == SKIPCOND_EQ) {
				if (p->ac == 0)
					p->pc += 1;
			} else if (skipcond == SKIPCOND_GT) {
				if (p->ac > 0)
					p->pc += 1;
			}
		} else if (p->opcode == OPCODE_JUMP) {
			p->pc = operand;
		} else if (p->opcode == OPCODE_JNS) {
			p->mar = operand;
			p->mbr = p->pc;
			p->memory[p->mar] = p->mbr;
			p->pc = operand + 1;
		} else if (p->opcode == OPCODE_ADDI) {
			p->mar = operand;
			p->mbr = p->memory[p->mar];
			p->mar = p->mbr;
			p->mbr = p->memory[p->mar];
			p->ac += p->mbr;
		} else if (p->opcode == OPCODE_JUMPI) {
			p->mar = operand;
			p->mbr = p->memory[p->mar];
			p->pc = p->mbr;
		}
	} else {
		// TODO: Allow different input formats, e.g., octal, decimal,
		//       hexadecimal.
		// TODO: Remove usage of getline() to read user input.
		if (p->opcode == OPCODE_INPUT) {
			u8* line_raw = NULL;
			isize line_cap = 0;

			u16 in;
			for (;;) {
				eprintf("input: ");

				isize n_read = getline(&line_raw, &line_cap, stdin);
				assert(n_read != -1);

				String line = SN(line_raw, n_read);
				if (line.data[line.len - 1] == '\n')
					line.len -= 1;

				isize in_idx = -1;
				in = strconv_parse_int(line, 10, &in_idx);
				if (in_idx != -1)
					eprintfln("error: invalid input, expected decimal number");
				else
					break;
			}

			free(line_raw);

			p->in = in;
			p->ac = p->in;
		} else if (p->opcode == OPCODE_OUTPUT) {
			p->out = p->ac;
			printfln("%d", p->out);
		} else if (p->opcode == OPCODE_HALT) {
			return false;
		} else if (p->opcode == OPCODE_CLEAR) {
			p->ac = 0;
		}
	}

	bool should_increment_pc = p->opcode != OPCODE_JUMP
		&& p->opcode != OPCODE_JUMPI && p->opcode != OPCODE_JNS;
	if (should_increment_pc)
		p->pc += 1;

	return true;
}

//
// MARIE assembler
//

#define X_TOKENS \
	X_TOKEN(TOKEN_EOF,       "EOF")        \
	X_TOKEN(TOKEN_INVALID,   "INVALID")    \
	X_TOKEN(TOKEN_OPCODE,    "OPCODE")     \
	X_TOKEN(TOKEN_DIRECTIVE, "DIRECTIVE")  \
	X_TOKEN(TOKEN_IDENT,     "IDENTIFIER") \
	X_TOKEN(TOKEN_NUMBER,    "NUMBER")     \
	X_TOKEN(TOKEN_COMMA,     "COMMA")      \
	X_TOKEN(TOKEN_NEWLINE,   "NEWLINE")    \
	X_TOKEN(TOKEN_COMMENT,   "COMMENT")

typedef enum TokenKind {
#define X_TOKEN(e, _) e,
	X_TOKENS
#undef X_TOKEN
} TokenKind;

char const* token_names[] = {
#define X_TOKEN(e, name) [e] = name,
	X_TOKENS
#undef X_TOKEN
};

typedef struct Token {
	isize     off;
	isize     len;
	isize     line;
	isize     line_off;
	TokenKind kind;
	u32       data;
} Token;

DYNAMIC_ARRAY(Token, TokenList);

typedef struct Tokenizer {
	String    src;
	isize     cursor;
	isize     line;
	isize     line_off;
	TokenList tokens;
} Tokenizer;

void tokenizer_init(Tokenizer* tz, String src) {
	tz->src = src;
	tz->cursor = 0;
	tz->line = 1;
	tz->line_off = 0;

	da_init_cap(&tz->tokens, 64);
}

void tokenizer_free(Tokenizer* tz) {
	free(tz->tokens.data);
}

void tokenizer_append_data(Tokenizer* tz, TokenKind kind, isize off, isize len, u32 data) {
	Token t = {
		.off      = off,
		.len      = len,
		.line     = tz->line,
		.line_off = tz->line_off,
		.kind     = kind,
		.data     = data,
	};

	da_append(&tz->tokens, t);
}

void tokenizer_append(Tokenizer* tz, TokenKind kind, isize off, isize len) {
	tokenizer_append_data(tz, kind, off, len, 0);
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
			tokenizer_append(tz, TOKEN_NEWLINE, tz->cursor, 1);
			tz->line += 1;
			tz->line_off = tz->cursor + 1;
		} else if (c == '/') {
			isize start = tz->cursor;
			while (c != '\n' && !tokenizer_eof(tz)) {
				tokenizer_consume(tz);
				c = tokenizer_curr(tz);
			}
			tokenizer_append(tz, TOKEN_COMMENT, start, tz->cursor - start + 1);
			continue;
		}

		tokenizer_consume(tz);
		c = tokenizer_curr(tz);
	}
}

bool is_valid_ident(u8 c) {
	return !is_space(c) && c != ',' && c != '/';
}

bool tokenizer_next(Tokenizer* tz) {
	tokenizer_advance(tz);

	if (tokenizer_eof(tz)) {
		tokenizer_append(tz, TOKEN_EOF, tz->src.len, 0);
		return false;
	}

	u8 c = tokenizer_curr(tz);
	isize start = tz->cursor;

	if (c == ',') {
		tokenizer_consume(tz);
		tokenizer_append(tz, TOKEN_COMMA, start, 1);
		return true;
	}

	while (is_valid_ident(c)) {
		tokenizer_consume(tz);
		c = tokenizer_curr(tz);
	}

	if (tz->cursor == start) {
		tokenizer_consume(tz);
		tokenizer_append(tz, TOKEN_INVALID, start, 1);
		return true;
	}

	String s = SS(tz->src, start, tz->cursor);
	String s_upper = string_to_upper(s);

	Opcode opcode = opcode_from_string(s_upper);
	if (opcode != OPCODE_INVALID) {
		tokenizer_append_data(tz, TOKEN_OPCODE, start, s.len, opcode);
		string_free(&s_upper);
		return true;
	}

	Directive dire = directive_from_string(s_upper);
	if (dire != DIRECTIVE_INVALID) {
		tokenizer_append_data(tz, TOKEN_DIRECTIVE, start, s.len, dire);
		string_free(&s_upper);
		return true;
	}

	string_free(&s_upper);

	if (s.len > 0 && (s.data[0] == '-' || is_digit(s.data[0]))) {
		isize i = 1;
		while (i < s.len && is_hex_digit(s.data[i]))
			i += 1;

		if (i == s.len) {
			tokenizer_append(tz, TOKEN_NUMBER, start, s.len);
			return true;
		}

		tokenizer_append(tz, TOKEN_INVALID, start, s.len);
		return false;
	}

	tokenizer_append(tz, TOKEN_IDENT, start, s.len);

	return true;
}

typedef struct LabelMapEntry {
	String label;
	i16    addr;  // memory address of the label
	Token  decl;  // where this label was first declared in the source
} LabelMapEntry;

typedef struct Parser {
	String    src;
	String    src_name;
	TokenList tokens;

	isize error_count;

	isize cursor;
	u16   start_addr;
	u16   curr_addr;

	isize          map_len;
	isize          map_cap;
	u32*           map_hashes;
	LabelMapEntry* map_entries;

	i16* memory;
} Parser;

void parser_init(Parser* p, String src, String src_name, TokenList tokens, i16* memory) {
	p->src = src;
	p->src_name = src_name;
	p->tokens = tokens;

	p->error_count = 0;

	p->cursor = 0;
	p->start_addr = 0;
	p->curr_addr = 0;

	p->map_len = 0;
	p->map_cap = 32;

	isize map_size = sizeof(*p->map_hashes) + sizeof(*p->map_entries);
	u8* map_mem = calloc(p->map_cap, map_size);

	p->map_hashes = cast(u32*, map_mem);
	p->map_entries = cast(LabelMapEntry*, &p->map_hashes[p->map_cap]);

	p->memory = memory;
}

void parser_free(Parser* p) {
	free(p->map_hashes);
}

u32 hash_fnv1a32(isize n, u8 const* data) {
	u32 hash = 2166136261;
	for (isize i = 0; i < n; ++i)
		hash = (hash ^ data[i]) * 16777619;
	if (hash == 0)
		hash = 1;
	return hash;
}

u32 parser_map_hash(String label) {
	return hash_fnv1a32(label.len, label.data);
}

LabelMapEntry const* parser_map_get(Parser* p, String label) {
	u32 hash = parser_map_hash(label);
	isize n = hash % p->map_cap;

	isize i = n;
	isize c = 0;
	while (c < p->map_cap && p->map_hashes[i] != 0) {
		if (string_equal(p->map_entries[i].label, label))
			return &p->map_entries[i];

		i = (i + 1) % p->map_cap;
		c += 1;
	}

	return NULL;
}

void parser_map_put(Parser* p, String label, i16 addr, Token decl) {
	if (cast(f32, p->map_len + 1) / p->map_cap > 0.75) {
		isize new_cap = p->map_cap;
		while (cast(f32, p->map_len + 1) / new_cap > 0.75)
			new_cap <<= 1;

		isize map_size = sizeof(*p->map_hashes) + sizeof(*p->map_entries);
		u8* new_mem = calloc(new_cap, map_size);

		u32* new_hashes = cast(u32*, new_mem);
		LabelMapEntry* new_entries = cast(LabelMapEntry*, &new_hashes[new_cap]);

		for (isize i = 0; i < p->map_cap; ++i) {
			if (p->map_hashes[i] == 0)
				continue;

			isize j = p->map_hashes[i] % new_cap;
			while (new_hashes[j] != 0)
				j = (j + 1) % new_cap;

			new_hashes[j] = p->map_hashes[i];
			new_entries[j] = p->map_entries[i];
		}

		free(p->map_hashes);

		p->map_cap = new_cap;
		p->map_hashes = new_hashes;
		p->map_entries = new_entries;
	}

	u32 hash = parser_map_hash(label);
	isize n = hash % p->map_cap;

	isize i = n;
	while (p->map_hashes[i] != 0)
		i = (i + 1) % p->map_cap;

	p->map_hashes[i] = hash;
	p->map_entries[i].label = label;
	p->map_entries[i].addr = addr;
	p->map_entries[i].decl = decl;
	p->map_len += 1;
}

bool parser_eof(Parser const* p) {
	if (p->cursor >= p->tokens.len)
		return true;
	return p->tokens.data[p->cursor].kind == TOKEN_EOF;
}

Token parser_curr(Parser const* p) {
	return p->tokens.data[p->cursor];
}

Token parser_peek(Parser const* p) {
	if (parser_eof(p))
		return parser_curr(p);
	return p->tokens.data[p->cursor + 1];
}

void parser_consume(Parser* p) {
	p->cursor += 1;
}

void parser_skip_statement(Parser* p) {
	Token t = parser_curr(p);
	while (t.kind != TOKEN_NEWLINE) {
		parser_consume(p);
		t = parser_curr(p);
	}
}

void parser_skip(Parser* p) {
	Token t = parser_curr(p);
	while (t.kind == TOKEN_NEWLINE || t.kind == TOKEN_COMMENT) {
		parser_consume(p);
		t = parser_curr(p);
	}
}

bool parser_msg(Parser* p, Token t, bool error, char const* fmt, ...) {
	isize d_off = t.off - t.line_off;
	eprintf("%.*s:%td:%td: ", SF(p->src_name), t.line, d_off + 1);

	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);

	eprintf("\n");

	String cursor = string_make(d_off + t.len);
	for (isize i = 0; i < d_off; ++i)
		cursor.data[i] = p->src.data[t.line_off + i] == '\t' ? '\t' : ' ';
	for (isize i = d_off; i < d_off + t.len; ++i)
		cursor.data[i] = '~';
	cursor.data[d_off] = '^';

	String line = SZ(p->src, t.line_off);
	isize end = string_index_byte(line, '\n');
	if (end == -1)
		end = line.len;

	eprintfln(" %5td | %.*s", t.line, SF(SS(line, 0, end)));
	eprintfln("       | %.*s", SF(cursor));

	string_free(&cursor);

	if (error) {
		p->error_count += 1;
		parser_skip_statement(p);
	}

	return true;
}

bool parser_scan_directive(Parser* p) {
	Token t = parser_curr(p);
	Directive dire = t.data;
	if (dire == DIRECTIVE_ORG && p->curr_addr != p->start_addr)
		return !parser_msg(p, t, true, "error: ORG can only be at the first line");

	parser_consume(p); // directive

	Token u = parser_curr(p);
	if (u.kind != TOKEN_NUMBER)
		return !parser_msg(p, u, true, "error: expected a number after directive");

	isize base = 16;
	isize min = 0;
	isize max = DIRECTIVE_HEX ? 0xFFFF : 0xFFF;

	if (dire == DIRECTIVE_DEC) {
		base = 10;
		min = MARIE_MIN_INT;
		max = MARIE_MAX_INT;
	}

	i64 value = strconv_parse_int(SS(p->src, u.off, u.off + u.len), base, NULL);
	if (value < min || max < value) {
		if (dire == DIRECTIVE_DEC)
			return !parser_msg(p, u, true, "error: value must be within %td and %td", min, max);
		else
			return !parser_msg(p, u, true, "error: value must be within %tX and %tX", min, max);
	}

	if (dire == DIRECTIVE_ORG) {
		p->start_addr = value;
		p->curr_addr = p->start_addr - 1;
	}

	parser_consume(p); // addr

	return true;
}

bool parser_scan_opcode(Parser* p) {
	Token t = parser_curr(p);
	Opcode opcode = t.data;

	parser_consume(p); // opcode

	Token u = parser_curr(p);
	if (u.kind == TOKEN_COMMA)
		return parser_msg(p, t, true, "error: '%.*s' is a reserved opcode and cannot be used as a label",
				  SF(SS(p->src, t.off, t.off + t.len)));

	if (opcode_binary[opcode]) {
		if (u.kind == TOKEN_IDENT) {
			// no-op
		} else if (u.kind == TOKEN_NUMBER) {
			u64 value = strconv_parse_uint(SS(p->src, u.off, u.off + u.len), 16, NULL);
			if (opcode == OPCODE_SKIPCOND && value != SKIPCOND_LT && value != SKIPCOND_EQ && value != SKIPCOND_GT)
				return !parser_msg(p, u, true, "error: only 000, 400, and 800 are valid operands for SKIPCOND");
			else if (value >= MARIE_MEMORY_SIZE)
				return !parser_msg(p, u, true, "error: address out of memory bounds");
		} else if (u.kind == TOKEN_OPCODE) {
			return parser_msg(p, u, true, "error: opcode '%.*s' cannot be used as operand",
					  SF(SS(p->src, u.off, u.off + u.len)));
		} else {
			return parser_msg(p, u, true, "error: expected number or label");
		}

		parser_consume(p); // operand
	}

	return true;
}

bool parser_scan_next(Parser* p) {
	parser_skip(p);
	if (parser_eof(p))
		return false;

	Token t = parser_curr(p);

	if (t.kind == TOKEN_DIRECTIVE) {
		if (!parser_scan_directive(p))
			return true;
	} else if (t.kind == TOKEN_OPCODE) {
		if (!parser_scan_opcode(p))
			return true;
	} else if (t.kind == TOKEN_IDENT) {
		String label = SS(p->src, t.off, t.off + t.len);
		Token u = parser_peek(p);
		if (u.kind == TOKEN_COMMA) {
			LabelMapEntry const* e = parser_map_get(p, label);
			if (e != NULL) {
				parser_msg(p, t, true, "error: redeclaration of label '%.*s'", SF(label));
				parser_msg(p, e->decl, false, "note: label '%.*s' was previously declared here", SF(label));
				return true;
			}

			parser_consume(p); // ident
			parser_consume(p); // comma

			Token v = parser_curr(p);
			if (v.kind == TOKEN_DIRECTIVE) {
				if (!parser_scan_directive(p))
					return true;

				parser_map_put(p, label, p->curr_addr, t);
			} else if (v.kind == TOKEN_OPCODE) {
				if (!parser_scan_opcode(p))
					return true;

				parser_map_put(p, label, p->curr_addr, t);
			} else {
				return parser_msg(p, v, true, "error: expected opcode or directive");
			}
		} else {
			return parser_msg(p, t, true, "error: expected a comma after label '%.*s'", SF(label));
		}
	} else {
		Token u = parser_peek(p);
		if (u.kind == TOKEN_COMMA)
			return parser_msg(p, t, true, "error: label must not begin with a number");
		return parser_msg(p, t, true, "error: expected opcode, directive, or label");
	}

	t = parser_curr(p);
	if (t.kind == TOKEN_COMMENT)
		parser_consume(p);

	t = parser_curr(p);
	if (t.kind == TOKEN_NEWLINE) {
		p->curr_addr += 1;
		return true;
	}

	return parser_msg(p, t, true, "error: expected newline before new statement");
}

bool parser_decode_addr(Parser* p, Token t, i16* out) {
	if (t.kind == TOKEN_NUMBER) {
		*out = strconv_parse_int(SS(p->src, t.off, t.off + t.len), 16, NULL);
		return true;
	} else if (t.kind == TOKEN_IDENT) {
		LabelMapEntry const* e = parser_map_get(p, SS(p->src, t.off, t.off + t.len));
		if (e == NULL)
			return false;
		*out = e->addr;
		return true;
	}
	return false;
}

bool parser_run_next(Parser* p) {
	parser_skip(p);
	if (parser_eof(p))
		return false;

	Token t = parser_curr(p);
	parser_consume(p);

	if (t.kind == TOKEN_DIRECTIVE) { // either DEC or HEX
		Directive dire = t.data;
		if (dire == DIRECTIVE_ORG) {
			p->curr_addr -= 1;
		} else {
			Token u = parser_curr(p);
			i16 value = strconv_parse_int(
				SS(p->src, u.off, u.off + u.len),
				dire == DIRECTIVE_DEC ? 10 : 16, NULL);
			p->memory[p->curr_addr] = value;
		}

		parser_consume(p); // operand
	} else if (t.kind == TOKEN_OPCODE) {
		Opcode opcode = t.data;
		i16 operand = 0;

		if (opcode_binary[opcode]) {
			Token u = parser_curr(p);
			if (!parser_decode_addr(p, u, &operand))
				return parser_msg(p, u, true, "error: referenced label '%.*s' is not defined",
						  SS(p->src, u.off, u.off + u.len));
			parser_consume(p); // operand
		}

		p->memory[p->curr_addr] = (opcode << 12) | operand;
	} else { // t.kind == TOKEN_IDENT
		parser_consume(p); // comma

		Token u = parser_curr(p);
		parser_consume(p); // operation

		if (u.kind == TOKEN_DIRECTIVE) {
			Directive dire = u.data;
			if (dire == DIRECTIVE_ORG) {
				p->curr_addr -= 1;
			} else {
				Token w = parser_curr(p);
				i16 value = strconv_parse_int(
					SS(p->src, w.off, w.off + w.len),
					dire == DIRECTIVE_DEC ? 10 : 16, NULL);
				p->memory[p->curr_addr] = value;
			}

			parser_consume(p); // operand
		} else { // u.kind == TOKEN_OPERAND
			Opcode opcode = u.data;
			i16 operand = 0;

			if (opcode_binary[opcode]) {
				Token w = parser_curr(p);
				if (!parser_decode_addr(p, w, &operand))
					return parser_msg(p, w, true, "error: referenced label '%.*s' is not defined",
							  SS(p->src, w.off, w.off + w.len));
				parser_consume(p); // operand
			}

			p->memory[p->curr_addr] = (opcode << 12) | operand;
		}
	}

	p->curr_addr += 1;

	return true;
}

isize parser_run(Parser* p) {
	while (parser_scan_next(p))
		/* no-op */;

	if (p->error_count == 0) {
		p->cursor = 0;
		p->curr_addr = p->start_addr;
		while (parser_run_next(p))
			/* no-op */;
	}

	return p->error_count;
}

//
// CLI
//

DYNAMIC_ARRAY(char*, CStringList);

typedef enum Command {
	CMD_INVALID,
	CMD_HELP,
	CMD_BUILD,
	CMD_RUN,
} Command;

Command command_from_cstring(char const* s) {
	if (strcmp(s, "help") == 0)
		return CMD_HELP;
	else if (strcmp(s, "build") == 0)
		return CMD_BUILD;
	else if (strcmp(s, "run") == 0)
		return CMD_RUN;
	return CMD_INVALID;
}

typedef struct App {
	isize  argc;
	char** argv;

	Command     cmd;
	CStringList args;

	bool    use_stdin;
	bool    use_stdout;
	char*   out_name;
	Command cmd_help;
} App;

bool build_file(String in_name, FILE* in_file, FILE* out_file) {
	StringBuilder sb = {0};
	u8 buf_data[2048] = {0};
	String buf = SN(buf_data, 2048);

	for (;;) {
		isize n_read = fread(buf.data, sizeof(*buf.data), buf.len, in_file);
		if (n_read == 0)
			break;
		strb_write(&sb, SS(buf, 0, n_read));
	}

	if (ferror(in_file)) {
		eprintfln("error: failed to read input: %s", strerror(errno));
		return false;
	}

	String input = strb_to_string(&sb);

	Tokenizer tz;
	tokenizer_init(&tz, input);

	while (tokenizer_next(&tz))
		/* no-op */;

	i16 memory[MARIE_MEMORY_SIZE] = {0};

	Parser p;
	parser_init(&p, input, in_name, tz.tokens, memory);
	isize errors = parser_run(&p);

	parser_free(&p);
	tokenizer_free(&tz);
	string_free(&input);

	if (errors == 0) {
		fwrite(&p.start_addr, sizeof(p.start_addr), 1, out_file);

		isize n_written = fwrite(memory, sizeof(*memory), MARIE_MEMORY_SIZE, out_file);
		if (n_written != MARIE_MEMORY_SIZE) {
			eprintf("error: failed to write memory: %s (expected: %d, wrote: %td)\n",
				strerror(errno), MARIE_MEMORY_SIZE, n_written);
			return false;
		}

		fwrite(&p.map_len, sizeof(p.map_len), 1, out_file);
		for (isize i = 0; i < p.map_cap; ++i) {
			if (p.map_hashes[i] == 0)
				continue;
			LabelMapEntry e = p.map_entries[i];
			fwrite(&e.label.len, sizeof(e.label.len), 1, out_file);
			fwrite(e.label.data, sizeof(*e.label.data), e.label.len, out_file);
			fwrite(&e.addr, sizeof(e.addr), 1, out_file);
		}
	}

	return errors == 0;
}

bool run_file(String in_name, FILE* in_file) {
	i16 memory[MARIE_MEMORY_SIZE] = {0};

	Processor p;
	processor_init(&p, memory);

	fread(&p.pc, sizeof(p.pc), 1, in_file);
	
	isize n_read = fread(memory, sizeof(*memory), MARIE_MEMORY_SIZE, in_file);
	if (n_read != MARIE_MEMORY_SIZE) {
		eprintf("error: failed to read file: %s (expected: %d, read: %td)\n",
			strerror(errno), MARIE_MEMORY_SIZE, n_read);
		return false;
	}

	while (processor_step(&p))
		/* no-op */;

	return true;
}

bool parse_args(App* app) {
	for (isize i = 2; i < app->argc; ++i) {
		String arg = SL(app->argv[i]);
		if (string_has_prefix(arg, S("-")) && arg.len > 1) {
			String flag = arg;
			String value;
			bool has_value = string_cut(arg, S(":"), &flag, NULL, &value);

			if (string_equal(flag, S("-help"))) {
				app->cmd_help = app->cmd;
				app->cmd = CMD_HELP;
				break;
			}

			if (app->cmd == CMD_BUILD) {
				if (string_equal(flag, S("-stdin"))) {
					app->use_stdin = true;
				} else if (string_equal(flag, S("-stdout"))) {
					app->use_stdout = true;
				} else if (string_equal(flag, S("-out"))) {
					if (has_value && value.len > 0) {
						app->out_name = cast(char*, value.data);
					} else {
						eprintfln("error: expected file name in flag '-out'");
						return false;
					}
				} else {
					eprintfln("error: unknown flag '%.*s'", SF(flag));
					return false;
				}
			}
		} else {
			da_append(&app->args, app->argv[i]);
		}
	}

	return true;
}

char const* command_help[] = {
	[CMD_INVALID] = NULL,
	[CMD_HELP] =
		"Usage:\n"
		"\t%s <command> [arguments]\n"
		"\nCommands:\n"
		"\tbuild    Compile a MARIE assembly file\n"
		"\trun      Run a MARIE executable file\n",
	[CMD_BUILD] = 
		"Usage:\n"
		"\t%s build [file] [flags]\n\n"
		"\tCompile the given MARIE assembly file into a MARIE executable file.\n"
		"\nExamples:\n"
		"\tmarie build foo.mas                 Build foo.mas, output to foo.mex.\n"
		"\tmarie build foo.mas -out:bar.mex    Set bar.mex as the output file.\n"
		"\tmarie build -stdin -stdout          Use stdin as input and stdout as output.\n"
		"\nFlags:\n"
		"\t-out:<path>\n"
		"\t\tSet the path of the output file\n"
		"\t-stdin\n"
		"\t\tRead source from standard input\n"
		"\t-stdout\n"
		"\t\tWrite result to standard output\n",
	[CMD_RUN] =
		"Usage:\n"
		"\t%s run [arguments]\n\n"
		"\tRun the given MARIE executable file.\n"
		"\nExamples:\n"
		"\tmarie run foo.mex    Run foo.mex.\n",
};

bool handle_command(App* app) {
	bool help = app->cmd == CMD_HELP;
	bool build = app->cmd == CMD_BUILD;
	bool run = app->cmd == CMD_RUN;

	if (help) {
		if (app->cmd_help == CMD_INVALID && app->args.len >= 1)
			app->cmd_help = command_from_cstring(app->args.data[0]);
		if (app->cmd_help == CMD_INVALID) {
			eprintf(command_help[CMD_HELP], app->argv[0]);
			return false;
		}
		eprintf(command_help[app->cmd_help], app->argv[0]);
	} else if (build || run) {
		FILE* in_file = stdin;
		char* in_name = "<stdin>";

		if (!app->use_stdin) {
			if (app->args.len < 1) {
				eprintfln("error: expected input file as the first argument");
				return false;
			}

			in_name = app->args.data[0];
			in_file = fopen(in_name, "rb");
			if (in_file == NULL) {
				eprintfln("error: failed to open file %s: %s",
					in_name, strerror(errno));
				return false;
			}
		}

		String in_name_s = SL(in_name);

		if (build) {
			FILE* out_file = stdout;
			if (app->use_stdout && app->out_name != NULL) {
				eprintfln("error: the flags -stdout and -out cannot be used at the same time");
				return false;
			} else if (!app->use_stdout) {
				bool no_out_name = app->out_name == NULL;
				if (no_out_name) {
					StringBuilder sb = {0};
					isize to = in_name_s.len;
					if (string_has_suffix(in_name_s, S(".mas")))
						to -= 4;
					strb_write(&sb, SS(in_name_s, 0, to));
					strb_write_cstring(&sb, ".mex");
					app->out_name = strb_to_cstring(&sb);
				}

				out_file = fopen(app->out_name, "wb");
				if (out_file == NULL) {
					eprintfln("error: failed to open file %s: %s",
						  app->out_name, strerror(errno));
					if (no_out_name)
						free(app->out_name);
					return false;
				}

				if (no_out_name)
					free(app->out_name);
			}

			bool build_ok = build_file(in_name_s, in_file, out_file);
			if (in_file != stdin)
				fclose(in_file);
			if (out_file != stdout)
				fclose(out_file);
			if (!build_ok)
				return false;
		} else {
			bool run_ok = run_file(in_name_s, in_file);
			if (in_file != stdin)
				fclose(in_file);
			if (!run_ok)
				return false;
		}
	}

	return true;
}

int main(int argc, char** argv) {
	App app;
	app.argc = argc;
	app.argv = argv;

	app.cmd = CMD_INVALID;
	if (argc > 1)
		app.cmd = command_from_cstring(argv[1]);

	if (app.cmd == CMD_INVALID) {
		eprintf(command_help[CMD_HELP], argv[0]);
		return 1;
	}

	da_init_cap(&app.args, 8);

	app.use_stdin  = false;
	app.use_stdout = false;
	app.out_name   = NULL;
	app.cmd_help   = CMD_INVALID;

	bool ok = parse_args(&app);
	if (ok)
		ok = handle_command(&app);

	da_free(&app.args);

	return ok ? 0 : 1;
}
