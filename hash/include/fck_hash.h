
// TODO: The first case should trigger a compile time error
#define FCK_STATIC_HASH_NO_HASH_EXISTS(str) sizeof("" str "")
#define FCK_STATIC_HASH_HASH_EXISTS(str, h) (sizeof("" str "") ? (h) : (h))
#define FCK_STATIC_HASH_CHOOSE(X, SELECT, ...) SELECT

#define FCK_STATIC_HASH(...) STATIC_HASH_CHOOSE(__VA_ARGS__, STATIC_HASH_NO_HASH_EXISTS(__VA_ARGS__), STATIC_HASH_HASH_EXISTS(__VA_ARGS__))

typedef unsigned long long fck_hash_int;

static fck_hash_int fck_hash(char *str, char *end)
{
	unsigned long long hash = 5381;

	while (str < end && *str)
	{
		char c = *str++;
		hash = ((hash << 5) + hash) + (unsigned char)c;
	}

	return hash;
}

// example:
// unsigned long long id = STATIC_HASH("Teeest", 0x00000652D32D23AF)