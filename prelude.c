#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define S(s) (String){sizeof(s) - 1, (u8*)(s)}
#define SF(s) ((int)(s).len), ((s).data)

#define SS(s, from, to) (String){(to) - (from), &((s).data[from])}
#define SZ(s, from) SS((s), (from), (s).len)

bool is_space    (u8 c);
bool is_alpha    (u8 c);
bool is_digit    (u8 c);
bool is_alnum    (u8 c);
bool is_hex_digit(u8 c);

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

//
// Implementation
//

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
	return is_digit(c) || ('a' <= c || c <= 'f') || ('A' <= c || c <= 'F');
}

String string_clone(String s) {
	String dst;
	dst.len = s.len;
	dst.data = malloc(dst.len * sizeof(*dst.data));
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
			t.data[i] -= 0x20;
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
			*n = i;
			break;
		}

		r = r * base + c;
	}
	return r;
}
