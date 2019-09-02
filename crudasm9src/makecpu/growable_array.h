// growable_array.h - Copyright (C) 2012 Willow Schlanger. All rights reserved.

#ifndef l_ncc_growable_array_h__included
#define l_ncc_growable_array_h__included

#include <stddef.h>		/* for NULL and size_t */

namespace CrudasmUtilities
{

namespace NccInternal
{

template <class ValueType>
class append_array
{
	ValueType *values[64];
	size_t alloc_size;
	size_t num_levels;
	size_t init_size;
	
	// Some compilers mask shifts to the low 5 bits, even in 64bit mode.
	static size_t shl(size_t x, size_t y)
	{
		if(y < 32)
			return x << y;
		x <<= 16;
		x <<= 16;
		y -= 32;
		return x << y;
	}

public:
	append_array(size_t init_sizeT)
	{
		for(size_t i = 0; i < 64; ++i)
			values[i] = 0;
		init_size = init_sizeT;
		alloc_size = shl(2, init_size);
		num_levels = 1;
		values[0] = new ValueType [alloc_size];
	}
	
	~append_array()
	{
		for(size_t i = 0; i < 64; ++i)
			if(values[i] != NULL)
				delete [] values[i];
	}
	
	size_t getAllocSize() const
	{
		return alloc_size;
	}
	
	void grow()
	{
		size_t grow_size = shl(2, init_size + num_levels);
		values[num_levels] = new ValueType [grow_size];
		alloc_size += grow_size;
		++num_levels;
	}

private:

	size_t getStartOffset(size_t level)
	{
		if(level == 0)
			return 0;
		// suppose init_size is 2.
		// level == 0 --> returns 0
		// level == 1 --> returns 8 = 1000
		// level == 2 --> returns 8 + 16 = 11000
		// level == 3 --> returns 8 + 16 + 32 = 111000
		return shl((shl(1, level) - 1), 1 + init_size);
	}

public:

	ValueType *seek(size_t index, size_t *contig_size, size_t *start_index = NULL)
	{
		size_t min = 0;
		size_t max = 63;
		
        size_t mid;
        size_t y, y2;
        do
        {
            mid = min + ((max - min) >> 1);
            y = getStartOffset(mid);
            y2 = getStartOffset(mid + 1);
            if(index >= y2)
                min = mid + 1;
            else
            if(index < y)
                max = mid - 1;
            else
            {
            	if(contig_size != NULL)
            		*contig_size = y2 - index;
            	if(start_index != NULL)
            		*start_index = y;
                return &values[mid][index - y];
            }
        }   while(min <= max);
		
		return NULL;
	}
};

template <class ValueType>
class growable_array_iter
{
	append_array<ValueType> *container;
	ValueType *current;
	size_t index;
	size_t begin_index;
	size_t end_index;

public:
	growable_array_iter()
	{
		container = NULL;
		index = 0;
		current = NULL;
		begin_index = 0;
		end_index = 0;
	}
	
	growable_array_iter &operator+=(size_t count)
	{
		index += count;
		if(current != NULL)
		{
			if(index >= end_index)
				current = NULL;
			else
				current += count;
		}
		return *this;
	}
	
	growable_array_iter &operator-=(size_t count)
	{
		index -= count;
		if(current != NULL)
		{
			if(index < begin_index)
				current = NULL;
			else
				current -= count;
		}
		return *this;
	}
	
	growable_array_iter operator+(size_t src)
	{
		growable_array_iter result = *this;
		result += src;
		return result;
	}
	
	growable_array_iter operator-(size_t src)
	{
		growable_array_iter result = *this;
		result -= src;
		return result;
	}
	
	size_t operator-(const growable_array_iter &rhs) const
	{
		return index - rhs.index;
	}

	growable_array_iter(append_array<ValueType> *containerT, size_t indexT)
	{
		container = containerT;
		index = indexT;
		current = NULL;
		begin_index = 0;
		end_index = 0;
	}
	
	bool operator==(const growable_array_iter<ValueType> &src) const
	{
		return container == src.container && index == src.index;
	}
	
	bool operator!=(const growable_array_iter<ValueType> &src) const
	{
		return container != src.container || index != src.index;
	}
	
	growable_array_iter &operator++()
	{
		++index;
		if(current != NULL)
		{
			if(index >= end_index)
				current = NULL;
			else
				++current;
		}
		return *this;
	}
	
	growable_array_iter &operator--()
	{
		--index;
		if(current != NULL)
		{
			if(index < begin_index)
				current = NULL;
			else
				--current;
		}
		return *this;
	}
	
	ValueType &operator*()
	{
		if(current == NULL)
		{
			size_t contig_size = 0;
			current = container->seek(index, &contig_size, &begin_index);
			end_index = contig_size + index;
		}
		
		return *current;
	}
};

template <class ValueType>
class growable_array
{
	append_array<ValueType> container;
	growable_array_iter<ValueType> end_iter;
	size_t content_length;

public:
	growable_array(size_t init_sizeT = 7 /* 2 << 7 == 256 objects in first level */) :
		container(init_sizeT)
	{
		content_length = 0;
		end_iter = growable_array_iter<ValueType>(&container, content_length);
	}
	
	append_array<ValueType> *getContainer()
	{
		return &container;
	}

	growable_array_iter<ValueType> begin()
	{
		return growable_array_iter<ValueType>(&container, 0);
	}

	growable_array_iter<ValueType> end()
	{
		return end_iter;
	}
	
	void clear()
	{
		content_length = 0;
		end_iter = growable_array_iter<ValueType>(&container, content_length);
	}
	
	size_t size()
	{
		return content_length;
	}
	
	ValueType &push_back(ValueType value)
	{
		if(content_length == container.getAllocSize())
			container.grow();
		ValueType &result = *end_iter = value;
		++end_iter;
		++content_length;
		return result;
	}
	
	ValueType &push_back()
	{
		if(content_length == container.getAllocSize())
			container.grow();
		ValueType &result = *end_iter;
		++end_iter;
		++content_length;
		return result;
	}
	
	ValueType &back()
	{
		// assume content_length isn't 0 !
		growable_array_iter<ValueType> iter = end_iter;
		return *--iter;
	}
	
	ValueType &front()
	{
		// assume content_length isn't 0 !
		return *begin();
	}
	
	void pop_back()
	{
		if(content_length != 0)
		{
			--content_length;
			--end_iter;
		}
	}
	
	void truncate(size_t new_size)
	{
		if(content_length > new_size)
		{
			end_iter -= (content_length - new_size);
			content_length = new_size;
		}
	}
};

}	// namespace NccInternal

using NccInternal::growable_array;
using NccInternal::growable_array_iter;
using NccInternal::append_array;

}   // namespace CrudasmUtilities

#endif  // l_ncc_growable_array_h__included
