// gen.cpp : Source file for your target.
//

#include "gen.h"
#include <assert.h>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <unordered_map>
#include <vector>

enum source_symbol_type
{
	SST_NONE,
	// Operators
	SST_OP_ASSIGNMENT, // =

	// Arithmetic
	SST_OP_ARITH_UNARY_PLUS,      // +
	SST_OP_ARITH_UNARY_MINUS,     // -
	SST_OP_ARITH_ADD,             // +
	SST_OP_ARITH_SUB,             // -
	SST_OP_ARITH_MUL,             // *
	SST_OP_ARITH_DIV,             // /
	SST_OP_ARITH_MOD,             // %
	SST_OP_ARITH_BIT_NOT,         // ~
	SST_OP_ARITH_BIT_AND,         // &
	SST_OP_ARITH_BIT_OR,          // |
	SST_OP_ARITH_BIT_XOR,         // ^
	SST_OP_ARITH_BIT_SHIFT_LEFT,  // <<
	SST_OP_ARITH_BIT_SHIFT_RIGHT, // >>
	SST_OP_ARITH_PRE_INC,         // ++
	SST_OP_ARITH_POST_INC,        // ++
	SST_OP_ARITH_PRE_DEC,         // --
	SST_OP_ARITH_POST_DEC,        // --

	// Assignment arithmetic
	SST_OP_ARITH_ASSIGN_ADD,             // +=
	SST_OP_ARITH_ASSIGN_SUB,             // -=
	SST_OP_ARITH_ASSIGN_MUL,             // *=
	SST_OP_ARITH_ASSIGN_DIV,             // /=
	SST_OP_ARITH_ASSIGN_MOD,             // %=
	SST_OP_ARITH_ASSIGN_BIT_AND,         // &=
	SST_OP_ARITH_ASSIGN_BIT_OR,          // |=
	SST_OP_ARITH_ASSIGN_BIT_XOR,         // ^=
	SST_OP_ARITH_ASSIGN_BIT_SHIFT_LEFT,  // <<=
	SST_OP_ARITH_ASSIGN_BIT_SHIFT_RIGHT, // >>=

	// Access
	SST_OP_ACCESS_ADDR_SUBSCRIPT_L, // [
	SST_OP_ACCESS_ADDR_SUBSCRIPT_R, // ]
	SST_OP_ACCESS_MEMBER,           // .
	SST_OP_ACCESS_PTR_MEMBER,       // ->
	SST_OP_ACCESS_DEREF,            // *
	SST_OP_ACCESS_ADDR_OF,          // &

	// Comparison
	SST_OP_COMP_EQ,         // ==
	SST_OP_COMP_NOT_EQ,     // !=
	SST_OP_COMP_LESS,       // <
	SST_OP_COMP_GREATER,    // >
	SST_OP_COMP_EQ_LESS,    // =<
	SST_OP_COMP_EQ_GREATER, // =>

	// Logical
	SST_OP_LOGIC_NOT, // !
	SST_OP_LOGIC_AND, // &&
	SST_OP_LOGIC_OR,  // ||

	// Keywords - Most of them will not be implemented
	SST_KW_ALIGNAS,
	SST_KW_ALIGNOF,
	SST_KW_AUTO,
	SST_KW_BOOL,
	SST_KW_BREAK,
	SST_KW_CASE,
	SST_KW_CHAR,
	SST_KW_CONST,
	SST_KW_CONSTEXPR,
	SST_KW_CONTINUE,
	SST_KW_DEFAULT,
	SST_KW_DO,
	SST_KW_DOUBLE,
	SST_KW_ELSE,
	SST_KW_ENUM,
	SST_KW_EXTERN,
	SST_KW_FALSE,
	SST_KW_FLOAT,
	SST_KW_FOR,
	SST_KW_GOTO,
	SST_KW_IF,
	SST_KW_INLINE,
	SST_KW_INT,
	SST_KW_LONG,
	SST_KW_NULLPTR,
	SST_KW_REGISTER,
	SST_KW_RESTRICT,
	SST_KW_RETURN,
	SST_KW_SHORT,
	SST_KW_SIGNED,
	SST_KW_SIZEOF,
	SST_KW_STATIC,
	SST_KW_STATIC_ASSERT,
	SST_KW_STRUCT,
	SST_KW_SWITCH,
	SST_KW_THREAD_LOCAL,
	SST_KW_TRUE,
	SST_KW_TYPEDEF,
	SST_KW_TYPEOF,
	SST_KW_TYPEOF_UNQUAL,
	SST_KW_UNION,
	SST_KW_UNSIGNED,
	SST_KW_VOID,
	SST_KW_VOLATILE,
	SST_KW_WHILE,

	SST_COUNT,
};

inline uint64_t round_pow2(uint64_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v++;
	return v + (v == 0);
}

#if defined(__has_builtin)
#if __has_builtin(__builtin_ctzll)
int count_trailing_zeros64(uint64_t value)
{
	return __builtin_ctzll(value);
}
#else
int count_trailing_zeros64(uint64_t value)
{
	if (value == 0)
	{
		return sizeof(uint64_t) * CHAR_BIT; // 64 for uint64_t
	}

	int count = 0;
	uint64_t lowest_bit = value & (-value);

	if (lowest_bit & 0xFFFFFFFF00000000ULL)
	{
		count += 32;
		lowest_bit >>= 32;
	}
	if (lowest_bit & 0x00000000FFFF0000ULL)
	{
		count += 16;
		lowest_bit >>= 16;
	}
	if (lowest_bit & 0x000000000000FF00ULL)
	{
		count += 8;
		lowest_bit >>= 8;
	}
	if (lowest_bit & 0x00000000000000F0ULL)
	{
		count += 4;
		lowest_bit >>= 4;
	}
	if (lowest_bit & 0x000000000000000CULL)
	{
		count += 2;
		lowest_bit >>= 2;
	}
	if (lowest_bit & 0x0000000000000002ULL)
	{
		count += 1;
	}

	return count;
}
#endif
#endif

inline bool streq(char const *lhs, char const *rhs, size_t n)
{
	return memcmp(lhs, rhs, n) == 0;
}

uint64_t strhash(char const *s, size_t n)
{
	// 32 -> 2166136261
	// 64 -> 14695981039346656037
	uint64_t hash = 14695981039346656037ul;

	for (uint64_t index = 0; index < n; index++)
	{
		// 32 -> 16777619
		// 64 -> 1099511628211
		hash = hash ^ *(uint8_t *)(s + index);
		hash = hash * 1099511628211ul;
	}
	return hash;
}

struct mappie_keys
{
	uint64_t hash;
};

struct mappie
{
	mappie_keys *keys;
	size_t capacity;
	uint64_t control_bits[1];
};

mappie *mappie_alloc(size_t capacity)
{
	capacity = (size_t)round_pow2(capacity);
	const size_t control_count = (capacity + 7) / 8;
	const size_t total = offsetof(mappie, control_bits[control_count]);
	mappie *hm = (mappie *)malloc(total);
	memset(hm, 0, total);

	hm->keys = (mappie_keys *)malloc(sizeof(*hm->keys) * capacity);
	hm->capacity = capacity;
	return hm;
}

void mappie_free(mappie *hm)
{
	assert(hm);
	free(hm->keys);
	free(hm);
}

uint64_t mappie_control_mask(mappie const *hm, size_t at)
{
	uint64_t control = hm->control_bits[at / 8];
	return control;
}

void mappie_control_mask_set(mappie *hm, size_t at, uint8_t value)
{
	uint64_t *control = &hm->control_bits[at / 8];
	uint64_t offset_bytes = at % 8;
	uint64_t offset_bits = offset_bytes * 8;

	uint64_t target = uint64_t(value | 0x80ULL) << offset_bits;

	uint64_t set_mask = 0xFFULL << offset_bits;
	// Clear
	*control = *control & ~set_mask;
	// Set
	*control = *control | target;
}

uint64_t mask_invalidate_lower(uint64_t mask, size_t at)
{
	uint64_t irrelevant_bit_count = (at % 8) * 8;
	return (mask >> irrelevant_bit_count) << irrelevant_bit_count;
}

bool mappie_store(mappie *hm, uint64_t hash, uint64_t *out_slot)
{
	assert(out_slot);

	const uint64_t discovery_mask = 0x8080808080808080;

	// if control bit size is 8, we control 8 slots at once
	for (size_t index = 0; index < hm->capacity;)
	{
		const size_t at = (hash + index) % hm->capacity;
		uint64_t full_control = mappie_control_mask(hm, at);

		// 11111111 10001100 00000000 -> 10000000 10000000 00000000
		const uint64_t identfied_bits = full_control & discovery_mask;
		// 10000000 10000000 00000000 -> 11111111 11111111 01111111 <- free
		const uint64_t searchable_bits = identfied_bits | ~discovery_mask;

		const uint64_t result = mask_invalidate_lower(~searchable_bits, at);
		const int trailing_count = count_trailing_zeros64(result);

		// We mask all the content out and flip it
		// We are GUARANTEED to only find control with a 0
		// If we hit 64 count, we need to go to the next qword
		// Not hitting this branch either imples that we hit an edge (i.e., element stored in 7)
		if (trailing_count < 64)
		{
			const uint64_t offset = at % 8;
			const size_t local_at = trailing_count / 8;
			const size_t slot_at = at - offset + local_at;

			mappie_keys *key = hm->keys + slot_at;
			// This... should actually never happen
			// If the signal bit is 0, it implies that the stored hash is also 0...
			// If it is 1 (set), it implies the opposite
			//if (key->hash == 0)
			//{
				const uint64_t right_shift = (local_at * 8);
				const uint8_t hash_part = uint8_t((hash >> right_shift) & 0xFF);

				key->hash = hash;
				mappie_control_mask_set(hm, slot_at, hash_part);
				*out_slot = slot_at;
				return true;
			//}
		}

		// Elegantly moves to the next qword, lol
		index = index + (8 - ((hash + index) % 8));
	}
	return false;
}

struct mappie_load_iterator
{
	// idk if all of them are useful...
	size_t index;
	size_t slot;
	uint64_t hash;
};

mappie_load_iterator mappie_iterator_create()
{
	return {0};
}

mappie_load_iterator mappie_iterator_create(mappie *hm, uint64_t hash)
{
	return {0, ~0ull, hash};
}

bool mappie_try_load(mappie const *hm, mappie_load_iterator *iterator)
{
	const uint64_t discovery_mask = 0x8080808080808080;

	assert(hm);
	assert(iterator);

	// We continue at the slot we found before(?)
	while (iterator->index < hm->capacity)
	{
		size_t at = (iterator->hash + iterator->index) % hm->capacity;
		size_t offset = at % 8;

		const uint64_t full_control = mappie_control_mask(hm, at);

		// 01010011 11001100 00111000 -> 11010011 11001100 10111000
		const uint64_t discovery_hash = iterator->hash | discovery_mask;
		const uint64_t result = discovery_hash & full_control;

		// 11010011 11001100 00111000 -> 11111111 11111111 01111111
		const uint64_t occupied_mask = full_control | ~discovery_mask;

		const uint64_t offset_bits = offset * 8;
		//const uint64_t searchable_mask = mask_invalidate_lower(~occupied_mask, iterator->at);
		//int trailing_count = count_trailing_zeros64(searchable_mask);
		int trailing_count = count_trailing_zeros64(~(occupied_mask >> offset_bits)) + offset_bits;
		for (int bit_index = offset_bits; bit_index < trailing_count; bit_index = bit_index + 8)
		{
			const uint64_t part_mask = 0xFFULL << bit_index;
			const uint64_t part_hash = result & part_mask;
			const uint64_t part_discovery = discovery_hash & part_mask;

			if (part_hash == part_discovery) // TOTAL MATCH!! OMG!!!!!!
			{
				const size_t slot_at = at - offset + (bit_index / 8);
				mappie_keys *key = hm->keys + slot_at;
				if (key->hash == iterator->hash)
				{
					// OMG, FOUND!!
					iterator->slot = slot_at;
					return true;
				}
			}
		}
		if (trailing_count < 64)
		{
			return false;
		}

		iterator->index = iterator->index + (8 - ((iterator->hash + iterator->index) % 8));
	}

	// We only reach this one if we ACTUALLY searched through the WHOLE hashmap LOOOOL
	return false;
}


bool mappie_remove(mappie* hm, mappie_load_iterator* iterator)
{
	assert(hm);
	assert(iterator);

	// TODO: Implement
	// Invalidate control block and update it
	// Occupy a second bit? 
	// Hash... idk, remove it or so

	return false;
}


struct char_ptr_size_t_pair
{
	char const *s;
	size_t n;
};

struct mappie_string_symbol_type
{
	mappie *base;
	source_symbol_type *values;
	char_ptr_size_t_pair keys[1];
};

mappie_string_symbol_type *mappie_string_symbol_type_alloc(size_t capacity)
{
	capacity = (size_t)round_pow2(capacity);
	size_t total = offsetof(mappie_string_symbol_type, keys[capacity]);
	mappie_string_symbol_type *hm = (mappie_string_symbol_type *)malloc(total);
	memset(hm, 0, total);

	hm->base = mappie_alloc(capacity);
	hm->values = (source_symbol_type *)malloc(capacity * sizeof(*hm->values));
	return hm;
}
void mappie_string_symbol_type_free(mappie_string_symbol_type *hm)
{
	assert(hm);
	assert(hm->values);

	free(hm->values);
	mappie_free(hm->base);
	free(hm);
}

bool mappie_string_symbol_type_store(mappie_string_symbol_type *hm, char const *s, size_t n, source_symbol_type value)
{
	uint64_t hash = strhash(s, n);
	uint64_t slot;
	if (mappie_store(hm->base, hash, &slot))
	{
		char_ptr_size_t_pair key = {s, n};
		memcpy(hm->keys + slot, &key, sizeof(*hm->keys));
		memcpy(hm->values + slot, &value, sizeof(*hm->values));
		return true;
	}
	return false;
}

bool mappie_string_symbol_type_store_lazy(mappie_string_symbol_type *hm, char const *s, source_symbol_type value)
{
	const size_t n = strlen(s);
	return mappie_string_symbol_type_store(hm, s, n, value);
}

bool mappie_string_symbol_type_try_load(mappie_string_symbol_type *hm, char const *s, size_t n, source_symbol_type *value)
{
	uint64_t hash = strhash(s, n);

	mappie_load_iterator it = mappie_iterator_create(hm->base, hash);
	while (mappie_try_load(hm->base, &it))
	{
		char_ptr_size_t_pair key = hm->keys[it.slot];
		if (key.n == n && streq(key.s, s, n))
		{
			memcpy(value, hm->values + it.slot, sizeof(*hm->values));
			printf("Found in: %lu\n", it.index);
			return true;
		}
	}
	printf("Not found in: %lu\n", it.index);
	return false;
}

struct source_span
{
	size_t at;
	size_t count;
};
struct source_file
{
	source_span span;
	size_t name_length;
	char *name;
	char *source;
};
struct source_files
{
	source_file *data;
	size_t count;
};

struct structures
{
	struct structure *data;
	size_t count;
};

struct discarded_spans
{
	struct discarded_span *data;
	size_t count;
};

struct structure
{
	source_span span;
	// We DO NOT support nested struct definitions
	// structures structs;

	discarded_spans discarded;
};

struct identifier
{
	source_span span;
};

struct type_name
{
	identifier identifier;
};

struct variable_declaration
{
	type_name type_name;
	identifier identifier;
};

struct discarded_span
{
	source_span span;
};

enum node_type
{
	NONE,
	SOURCE_FILE,
	SCOPE,
};

union node {
};

source_files *source_files_alloc_and_collect(source_files *files)
{
	assert(files);

	using namespace std;
	using namespace filesystem;

	printf("Begin:\tAllocating and collecting files\n");

	memset(files, 0, sizeof(*files));

	const size_t default_capacity = 64;
	size_t capacity = 64;
	files->data = (source_file *)malloc(sizeof(*files->data) * capacity);

	for (const directory_entry &entry : recursive_directory_iterator(GEN_SOURCE_DIRECTORY_PATH))
	{
		if (entry.is_regular_file())
		{
			path target = entry.path();
			path ext = target.extension();
			if (ext == ".h" || ext == ".hpp" || ext == ".hxx")
			{
				// Reallocaate
				if (files->count >= capacity)
				{
					size_t new_capacity = capacity * 2;
					source_file *new_data = (source_file *)malloc(sizeof(*files->data) * new_capacity);
					memcpy(new_data, files->data, sizeof(*files->data) * capacity);
					capacity = new_capacity;
					free(files->data);
					files->data = new_data;
				}

				// Read file and such bullocks
				const size_t upper_bound = 1024;

				source_file file;
				file.span = {0, 0};
				file.name_length = strnlen(target.string().c_str(), upper_bound);
				file.name = (char *)malloc(file.name_length + 1);
				memcpy(file.name, target.c_str(), file.name_length + 1);

				assert(file.name);
				printf("Found:\t%.*s\n", (int)file.name_length, file.name);

				FILE *f = fopen(file.name, "r");
				assert(f);

				int seek_result = fseek(f, 0, SEEK_END);
				assert(seek_result == 0);

				long tell = ftell(f);
				assert(tell != -1L);

				file.span.count = (size_t)tell;

				file.source = (char *)malloc(file.span.count);
				assert(file.source);

				seek_result = fseek(f, 0, SEEK_SET);
				assert(seek_result == 0);

				size_t read_result = fread(file.source, sizeof(*file.source), file.span.count, f);
				assert(read_result == file.span.count);

				int result = fclose(f);
				assert(result == 0);

				memcpy(files->data + files->count, &file, sizeof(file));
				files->count = files->count + 1;
			}
		}
	}
	printf("End:\tAllocating and collecting files\n");
	return files;
};

void source_files_free(source_files *files)
{
	assert(files);

	for (size_t index = 0; index < files->count; index++)
	{
		source_file *file = files->data + index;
		free(file->name);
		free(file->source);
		memset(file, 0, sizeof(*file));
	}
	free(files->data);
}

static bool is_identifier_always_legal(char c)
{
	return isalpha(c) || c == '_';
}

static bool is_identifier_sometimes_legal(char c, size_t word_count)
{
	return is_identifier_always_legal(c) || (isdigit(c) && word_count != 0);
}

source_span *source_spans_realloc(source_span *spans, size_t capacity)
{
	if (capacity != 0)
	{
		source_span *new_spans = (source_span *)realloc(spans, capacity * sizeof(*spans));
		if (new_spans == nullptr)
		{
			free(spans);
		}
		return new_spans;
	}

	// capacity == 0
	if (spans != nullptr)
	{
		free(spans);
	}
	return nullptr;
}

static size_t file_extract_token_spans(source_span const *span, char const *source, source_span **out_spans)
{
	assert(span);
	assert(out_spans);

	printf("Begin:\tExtraction token spans\n");

	*out_spans = nullptr;
	size_t spans_capacity = 32;
	*out_spans = source_spans_realloc(*out_spans, spans_capacity);
	size_t spans_count = 0;

	size_t at = span->at;
	size_t count = span->count;

	size_t word_start = at;
	for (size_t index = at; index < count; index++)
	{
		char current = source[index];
		if (isspace(current))
		{
			size_t word_end = index;
			size_t word_count = word_end - word_start;

			if (word_count > 0)
			{
				if (spans_count >= spans_capacity)
				{
					spans_capacity = spans_capacity * 2;
					*out_spans = source_spans_realloc(*out_spans, spans_capacity);
				}
				source_span word_span = {word_start, word_count};
				source_span *current_span = (*out_spans) + spans_count;
				memcpy(current_span, &word_span, sizeof(word_span));
				spans_count = spans_count + 1;
			}
			word_start = index + 1;
		}
	}
	printf("End:\tExtraction token spans\n");
	return spans_count;
}

void file_scope_scan(source_span *span, char *source)
{
	size_t at = span->at;
	size_t count = span->count;

	source_span word_span = {at, 0};
	for (size_t index = at; index < count; index++)
	{
		char current = source[index];

		if (is_identifier_sometimes_legal(current, word_span.count))
		{
			// Keep expanding the word
			word_span.count = word_span.count + 1;
			continue;
		}

		if (current == '{')
		{
			source_span child = {index + 1, count};
			file_scope_scan(&child, source);
			// ... We have got a new scope
			// Skip ahead to until after the scope!
			index = child.at + child.count;
		}
		if (current == '}')
		{
			// Update the outer scope
			span->count = index - at;
			return;
		}
	}
}

struct source_generator
{
	source_files files;
	structures structs;
	discarded_spans discarded;
};

static source_symbol_type ssymbol(char *s)
{
	return SST_NONE;
}

static source_symbol_type dsymbol(char *s)
{
	if (streq(s, "->", 2))
	{
		return SST_OP_ACCESS_PTR_MEMBER;
	}
	return SST_NONE;
}

static source_symbol_type msymbol(char *s, size_t n)
{
	return SST_NONE;
}

static void tokenise(source_file const *file, source_span const *spans, size_t count)
{
	printf("Begin: Tokenise - %.*s\n", (int)file->name_length, file->name);

	for (size_t index = 0; index < count; index++)
	{
		source_span const *span = spans + index;
		char const *word = file->source + span->at;
		// printf("Token: %.*s\n", (int)span->count, word);
	}

	printf("End: Tokenise - %.*s\n", (int)file->name_length, file->name);
}

void go(source_generator *generator)
{
	mappie_string_symbol_type *hm = mappie_string_symbol_type_alloc(512);
	mappie_string_symbol_type_store_lazy(hm, "bool", SST_KW_BOOL);
	mappie_string_symbol_type_store_lazy(hm, "int", SST_KW_INT);
	mappie_string_symbol_type_store_lazy(hm, "long", SST_KW_LONG);
	mappie_string_symbol_type_store_lazy(hm, "float", SST_KW_FLOAT);

	assert(generator);
	memset(generator, 0, sizeof(*generator));

	source_files_alloc_and_collect(&generator->files);

	for (size_t index = 0; index < generator->files.count; index++)
	{
		source_file *file = generator->files.data + index;
		source_span *spans = nullptr;
		size_t count = file_extract_token_spans(&file->span, file->source, &spans);
		tokenise(file, spans, count);
	}
	// find scopes
	// find data types
	// find members

	source_files_free(&generator->files);
}

void make_enum_pretty(std::string &pretty, size_t offset = 0)
{
	constexpr char wildcards[] = {'/', '\\', '.', ' ', '-'};
	for (size_t index = 0; index < sizeof(wildcards); index++)
	{
		char wildcard = wildcards[index];
		size_t it = pretty.find(wildcard, offset);
		if (it != std::string::npos)
		{
			pretty = pretty.erase(it, 1);
			pretty[it] = '_';
			make_enum_pretty(pretty, it);
		}
	}
}

void test()
{
	mappie_string_symbol_type *hm = mappie_string_symbol_type_alloc(37);

	const size_t size = 512;

	char cs[size];
	for (uint16_t i = 0; i < size; i++)
	{
		char c = (char)(i % 255);
		cs[i] = c;
	}
	{
		source_symbol_type value;
		bool result = mappie_string_symbol_type_try_load(hm, "HelloThere", 10, &value);
		printf("DO NOT HAVE OVER\n");
		assert(!result);
	}
	for (uint8_t i = 0; i < 31; i++)
	{
		char *c = &cs[static_cast<ptrdiff_t>(i * 4)];
		mappie_string_symbol_type_store(hm, c, 1, (source_symbol_type)i);
		source_symbol_type value;
		bool result = mappie_string_symbol_type_try_load(hm, c, 1, &value);
		assert(result);
		assert(value == (source_symbol_type)i);
	}

	for (uint8_t i = 0; i < 31; i++)
	{
		char *c = &cs[static_cast<ptrdiff_t>(i * 4)];
		source_symbol_type value;
		bool result = mappie_string_symbol_type_try_load(hm, c, 1, &value);
		assert(result);
		assert(value == (uint32_t)i);
	}

	{
		source_symbol_type value;
		bool result = mappie_string_symbol_type_try_load(hm, "HelloThere", 10, &value);
		assert(!result);
	}

	printf("NOTHING FAILED! HASHMAP BE BUZZIN\n");

	mappie_string_symbol_type_free(hm);
}

int main(int argc, char **argv)
{
	test();
	return 0;
	using namespace std;
	using namespace filesystem;

	constexpr char const *path = GEN_OUTPUT_DIRECTORY_PATH "/gen/gen_assets.h";

	source_generator gen;
	// go(&gen);

	printf("%s at: %s \n", "Running code generator...", path);

	struct element
	{
		filesystem::path path;
		size_t index;
	};

	unordered_map<string, vector<element>> map;

	ofstream output(path);
	output << "#ifndef GEN_INCLUDED\n#define GEN_INCLUDED\n";
	output << "// GENERATED\n";
	output << "constexpr char const* GEN_FILE_PATHS[] = \n";
	output << "{\n";

	size_t index = 0;
	for (const directory_entry &entry : recursive_directory_iterator(GEN_INPUT_DIRECTORY_PATH))
	{
		if (entry.is_regular_file())
		{
			filesystem::path target = entry.path(); // relative(entry.path(), GEN_INPUT_DIRECTORY_PATH);
			printf("Found: %s\n", target.c_str());
			output << '\t' << target << ",\n";

			string ext = target.extension().string();
			ext = ext.erase(0, 1);
			transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

			map[ext].push_back({target, index});
			index = index + 1;
		}
	}
	output << "};\n\n";

	for (auto &[ext, files] : map)
	{
		string copy_ext = ext;
		transform(copy_ext.begin(), copy_ext.end(), copy_ext.begin(), [](unsigned char c) { return std::toupper(c); });
		output << "#define GEN_DEFINED_" << copy_ext << '\n';
		output << "enum class gen_" << ext;
		output << '\n';
		output << "{\n";
		for (const auto &element : files)
		{
			filesystem::path copy = element.path;
			while (copy.has_extension())
			{
				copy = copy.stem();
			}
			std::string target = copy.string();

			make_enum_pretty(target);
			output << '\t' << target << " = " << element.index << ",\n";
		}
		output << "};\n\n";

		output << "constexpr gen_" << ext << " gen_" << ext << "_all" << "[] = {\n";
		for (const auto &element : files)
		{
			filesystem::path copy = element.path;
			while (copy.has_extension())
			{
				copy = copy.stem();
			}
			std::string target = copy.string();

			make_enum_pretty(target);
			output << '\t' << "gen_" << ext << "::" << target << ",\n";
		}
		output << "};\n\n";
	}

	for (auto &[ext, files] : map)
	{
		output << "inline char const* gen_get_path(gen_" << ext;
		output << " value)";
		output << '\n';
		output << "{\n";
		output << "\treturn GEN_FILE_PATHS[(int) value];\n";
		// for (const auto& element : files)
		//{
		//	filesystem::path copy = element.path;
		//	while (copy.has_extension()) {
		//		copy = copy.stem();
		//	}
		//	std::string target = copy.string();

		//	target[0] = toupper(target[0]);
		//	make_enum_pretty(target);
		//	output << '\t' << target << " = " << element.index << ",\n";
		//}
		output << "};\n\n";
	}
	output << "#endif // !GEN_INCLUDED";

	return 0;
}
