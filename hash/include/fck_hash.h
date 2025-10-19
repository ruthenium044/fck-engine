
#ifndef FCK_HASH_H_INCLUDED
#define FCK_HASH_H_INCLUDED

// TODO: The first case should trigger a compile time error
#define FCK_STATIC_HASH_NO_HASH_EXISTS(str) sizeof("" str "") / 0
#define FCK_STATIC_HASH_HASH_EXISTS(str, h) (sizeof("" str "") ? (h) : (h))
#define FCK_STATIC_HASH_CHOOSE(X, SELECT, ...) SELECT

// TODO: Fix this - it expands wrong cause I am dumb :-(
#define FCK_STATIC_HASH(...)                                                                                                               \
	FCK_STATIC_HASH_CHOOSE(__VA_ARGS__, FCK_STATIC_HASH_NO_HASH_EXISTS(__VA_ARGS__), FCK_STATIC_HASH_HASH_EXISTS(__VA_ARGS__))

// I do not know if I still approve hash_int... hash_type, maybe?
typedef unsigned long long fck_hash_int;

static fck_hash_int fck_hash(const char *str, int length)
{
	unsigned long long hash = 5381;

	for (int index = 0; index < length; index++)
	{
		char c = str[index];
		if (c == 0)
		{
			break;
		}
		hash = ((hash << 5) + hash) + (unsigned char)c;
	}
	return hash;
}

#endif // !FCK_HASH_H_INCLUDED
       // example:
       // unsigned long long id = STATIC_HASH("Teeest", 0x00000652D32D23AF)