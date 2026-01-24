//
// General utilities
//

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define cast(T, expr) (T)(expr)

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

#define S(s)  (String){sizeof(s) - 1, (u8*)(s)}
#define SL(s) (String){strlen(s), (u8*)(s)}
#define SF(s) ((int)(s).len), ((s).data)

#define SS(s, from, to) (String){(to) - (from), &((s).data[from])}
#define SZ(s, from)     SS((s), (from), (s).len)

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
void   string_copy_n    (String* dst, String src, isize n);
bool   string_equal     (String a, String b);
bool   string_equal_n   (String a, String b, isize n);
isize  string_index_byte(String s, u8 b);
String string_trim_space(String s);
String string_to_upper  (String s);

u64 strconv_from_uint(String s, isize base, isize* n);
i64 strconv_from_int(String s, isize base, isize* n);

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
	String dst = {n, malloc(n * sizeof(*dst.data))};
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

void string_copy_n(String* dst, String src, isize n) {
	memmove(dst->data, src.data, n);
}

bool string_equal(String a, String b) {
	if (a.len != b.len)
		return false;
	isize i = 0;
	while (i < a.len && a.data[i] == b.data[i])
		i += 1;
	return i == a.len;
}

bool string_equal_n(String a, String b, isize n) {
	if (a.len < n || b.len < n)
		return false;
	isize i = 0;
	while (i < n && a.data[i] == b.data[i])
		i += 1;
	return i == n;
}

isize string_index_byte(String s, u8 c) {
	for (isize i = 0; i < s.len; ++i) {
		if (s.data[i] == c)
			return i;
	}
	return -1;
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

u64 strconv_from_uint(String s, isize base, isize* n) {
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

i64 strconv_from_int(String s, isize base, isize* n) {
	i64 r = 0;
	i64 i = 0;

	if (s.data[0] == '-')
		i = 1;

	r = strconv_from_uint(SZ(s, i), base, n);
	return i == 0 ? r : -r;
}

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

//
// MARIE
//

#define MEMORY_SIZE 4096

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

#define X_TOKENS \
	X_TOKEN(TOKEN_EOF,       "EOF")        \
	X_TOKEN(TOKEN_INVALID,   "INVALID")    \
	X_TOKEN(TOKEN_OPCODE,    "OPCODE")     \
	X_TOKEN(TOKEN_DIRECTIVE, "DIRECTIVE")  \
	X_TOKEN(TOKEN_IDENT,     "IDENTIFIER") \
	X_TOKEN(TOKEN_NUMBER,    "NUMBER")     \
	X_TOKEN(TOKEN_COMMA,     "COMMA")      \
	X_TOKEN(TOKEN_NEWLINE,   "NEWLINE")    \
	X_TOKEN(TOKEN_COMMENT,   "COMMENT")    \

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

typedef struct ParserMapEntry {
	String label;
	i16    addr;  // memory address of the label
	Token  decl;  // where this label was first declared in the source
} ParserMapEntry;

typedef struct Parser {
	String    src;
	String    src_name;
	TokenList tokens;

	bool has_errors;

	isize cursor;
	i16   start_addr;
	i16   curr_addr;

	isize           map_len;
	isize           map_cap;
	u32*            map_hashes;
	ParserMapEntry* map_entries;

	i16* memory;
} Parser;

void parser_init(Parser* p, String src, String src_name, TokenList tokens, i16* memory) {
	p->src = src;
	p->src_name = src_name;
	p->tokens = tokens;

	p->has_errors = false;

	p->cursor = 0;
	p->start_addr = 0;
	p->curr_addr = 0;

	p->map_len = 0;
	p->map_cap = 32;

	isize map_size = sizeof(*p->map_hashes) + sizeof(*p->map_entries);
	u8* map_mem = calloc(p->map_cap, map_size);

	p->map_hashes = cast(u32*, map_mem);
	p->map_entries = cast(ParserMapEntry*, &p->map_hashes[p->map_cap]);

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

ParserMapEntry const* parser_map_get(Parser* p, String label) {
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
		ParserMapEntry* new_entries = cast(ParserMapEntry*, &new_hashes[new_cap]);

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
	fprintf(stderr, "%.*s:%td:%td: ", SF(p->src_name), t.line, d_off + 1);

	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);

	fprintf(stderr, "\n");

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

	fprintf(stderr, " %5td | %.*s\n", t.line, SF(SS(line, 0, end)));
	fprintf(stderr, "       | %.*s\n", SF(cursor));

	string_free(&cursor);

	if (error) {
		p->has_errors = true;
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
		min = -32768;
		max = 32767;
	}

	i64 value = strconv_from_int(SS(p->src, u.off, u.off + u.len), base, NULL);
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

	if (opcode_binary[opcode]) {
		Token u = parser_curr(p);
		if (u.kind == TOKEN_IDENT) {
			// no-op
		} else if (u.kind == TOKEN_NUMBER) {
			u64 value = strconv_from_uint(SS(p->src, u.off, u.off + u.len), 16, NULL);
			if (opcode == OPCODE_SKIPCOND && value != SKIPCOND_LT && value != SKIPCOND_EQ && value != SKIPCOND_GT)
				return !parser_msg(p, u, true, "error: only 000, 400, and 800 are valid operands for SKIPCOND");
			else if (value >= MEMORY_SIZE)
				return !parser_msg(p, u, true, "error: address out of memory bounds");
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
		Token u = parser_peek(p);
		if (u.kind == TOKEN_COMMA) {
			String label = SS(p->src, t.off, t.off + t.len);
			ParserMapEntry const* e = parser_map_get(p, label);
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
			return parser_msg(p, u, true, "error: expected a comma after label");
		}
	} else {
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
		*out = strconv_from_int(SS(p->src, t.off, t.off + t.len), 16, NULL);
		return true;
	} else if (t.kind == TOKEN_IDENT) {
		ParserMapEntry const* e = parser_map_get(p, SS(p->src, t.off, t.off + t.len));
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
			i16 value = strconv_from_int(
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
				i16 value = strconv_from_int(
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

void parser_run(Parser* p) {
	while (parser_scan_next(p))
		/* no-op */;

	if (!p->has_errors) {
		p->cursor = 0;
		p->curr_addr = p->start_addr;
		while (parser_run_next(p))
			/* no-op */;
	}
}

int main(int argc, char** argv) {
	String input = {0};
	String input_name = S("<stdin>");

	{
		FILE* file = stdin;
		if (argc > 1) {
			char const* file_name = argv[1];
			file = fopen(file_name, "rb");
			if (file == NULL) {
				fprintf(stderr, "error: failed to open file %s: %s\n",
					file_name, strerror(errno));
			}
			input_name = SL(file_name);
		}

		fseek(file, 0, SEEK_END);
		input.len = ftell(file);
		rewind(file);

		input.data = malloc(input.len * sizeof(*input.data));
		isize nread = fread(input.data, sizeof(*input.data), input.len, file);
		if (nread != input.len) {
			fprintf(stderr, "error: failed to read input (expected: %td, read: %td)\n",
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

	i16 memory[MEMORY_SIZE] = {0};

	Parser p;
	parser_init(&p, input, input_name, tz.tokens, memory);
	parser_run(&p);

	for (isize i = p.start_addr; i < p.start_addr + 20; ++i) {
		Opcode opcode = (p.memory[i] >> 12) & 0xF;
		printf("[%03tX] -> %04X (opcode: %s)\n", i, p.memory[i] & 0xFFFF, opcode_names[opcode]);
	}

	parser_free(&p);
	tokenizer_free(&tz);
	string_free(&input);

	return 0;
}
