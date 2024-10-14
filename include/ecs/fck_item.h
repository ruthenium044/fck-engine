#ifndef FCK_ITEM_INCLUDED
#define FCK_ITEM_INCLUDED

#include "ecs/fck_iterator.h"

template <typename index_type, typename value_type>
struct fck_item
{
	index_type *index;
	value_type *value;
};

template <typename index_type, typename value_type>
struct fck_iterator<fck_item<index_type, value_type>>
{
	index_type *index;
	value_type *value;
};

template <typename index_type, typename value_type>
fck_iterator<fck_item<index_type, value_type>> &operator++(fck_iterator<fck_item<index_type, value_type>> &item)
{
	++item.index;
	++item.value;
	return item;
}

template <typename index_type, typename value_type>
fck_item<index_type, value_type> operator*(fck_iterator<fck_item<index_type, value_type>> item)
{
	fck_item<index_type, value_type> value;
	value.index = item.index;
	value.value = item.value;
	return value;
}

#endif // FCK_ITEM_INCLUDED