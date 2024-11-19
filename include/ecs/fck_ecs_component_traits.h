#ifndef FCK_ECS_COMPONENT_TRAITS_INCLUDED
#define FCK_ECS_COMPONENT_TRAITS_INCLUDED

#include "fck_template_utility.h"

struct fck_off_tag
{
};

// Free trait
template <typename value_type>
using fck_ecs_component_free = void (*)(value_type *);
using fck_ecs_component_free_void = fck_ecs_component_free<void>;

template <typename value_type, typename = void>
struct fck_ecs_free_trait : fck_false_type
{
	template <typename target_type>
	static constexpr fck_ecs_component_free<target_type> fetch()
	{
		return nullptr;
	}
};

template <typename value_type>
struct fck_ecs_free_trait<value_type, fck_void<decltype(fck_free((value_type *)nullptr))>> : fck_true_type
{
	template <typename target_type>
	static constexpr fck_ecs_component_free<target_type> fetch()
	{
		void fck_free(target_type *);
		return fck_free;
	}
};
// !Free trait

// Serialise trait
struct fck_serialiser;

template <typename value_type>
using fck_ecs_component_serialise = void (*)(struct fck_serialiser *, value_type *, size_t);
using fck_ecs_component_serialise_void = fck_ecs_component_serialise<void>;

template <typename value_type, typename = void>
struct fck_ecs_serialise_trait : fck_false_type
{
	template <typename target_type>
	static constexpr fck_ecs_component_serialise<target_type> fetch()
	{
		return nullptr;
	}
};

template <typename value_type>
struct fck_ecs_serialise_trait<value_type,
                               fck_void<decltype(fck_serialise((struct fck_serialiser *)nullptr, (value_type *)nullptr, (size_t)0))>>
	: fck_true_type
{
	template <typename target_type>
	static constexpr fck_ecs_component_serialise<target_type> fetch()
	{
		void fck_serialise(struct fck_serialiser *, target_type *, size_t);
		return fck_serialise;
	}
};
// !Serialise Trait

template <typename value_type, typename = void>
struct fck_ecs_serialise_is_on : fck_false_type
{
	constexpr static bool value = false;
};

template <typename value_type>
struct fck_ecs_serialise_is_on<value_type, fck_void<decltype(fck_serialise((value_type *)nullptr, (fck_off_tag *)nullptr))>> : fck_true_type
{
	constexpr static bool value = true;
};

#define FCK_SERIALISE_OFF(value_type)                                                                                                      \
	void fck_serialise(struct value_type *, struct fck_off_tag *)                                                                          \
	{                                                                                                                                      \
	}

#endif // !FCK_ECS_COMPONENT_TRAITS_INCLUDED
