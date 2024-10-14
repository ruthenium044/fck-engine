#ifndef FCK_ITERATOR_INCLUDED
#define FCK_ITERATOR_INCLUDED

template <typename value_type>
struct fck_iterator
{
	value_type *value;
};

template <typename value_type>
fck_iterator<value_type> &operator++(fck_iterator<value_type> &item)
{
	++item.value;
	return item;
}

template <typename value_type>
value_type *operator*(fck_iterator<value_type> item)
{
	return item.value;
}

template <typename value_type>
bool operator!=(fck_iterator<value_type> lhs, fck_iterator<value_type> rhs)
{
	return lhs.value != rhs.value;
}

#endif // FCK_ITERATOR_INCLUDED