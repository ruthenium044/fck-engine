#ifndef FCK_TRAIT_UTILITY_INCLUDED
#define FCK_TRAIT_UTILITY_INCLUDED

#define FCK_SERIALISE_DEACTIVATE(type) void inline fck_serialise(struct fck_serialiser *serialiser, type *value, size_t count){};

#endif // !FCK_TRAIT_UTILITY_INCLUDED
