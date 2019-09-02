// axiom_adt.h - Copyright (C) 2012 Willow Schlanger. All rights reserved.
// Do not include this file directly: include axiom_lang.h instead.

#ifndef l_crudasm__axiom_adt_h__included
#define l_crudasm__axiom_adt_h__included

enum
{
	AXIOM_ITEM_ARRAY_BASE_POWER = 2,
	AXIOM_ITEM_ARRAY_BASE_SIZE = 1 << AXIOM_ITEM_ARRAY_BASE_POWER
};

class AxiomItemArray :
	public AxiomItemBase
{
	size_t size;						// this cannot change
	size_t delta;						// this is 0 for the leaf level
	AxiomItem defaultItem;
	AxiomItem items[AXIOM_ITEM_ARRAY_BASE_SIZE];

	void initCopy(const AxiomItemArray *srcT)
	{
		const AxiomItemArray &src = *srcT;
		size = src.size;
		delta = src.delta;
		defaultItem = src.defaultItem;
		for(size_t i = 0; i < AXIOM_ITEM_ARRAY_BASE_SIZE; ++i)
		{
			items[i] = src.items[i];
		}
	}
	
public:
	void debugWrite(std::ostream &os, size_t level = 0)
	{
		for(size_t i = 0; i < AXIOM_ITEM_ARRAY_BASE_SIZE; ++i)
		{
			for(size_t j = 0; j < level; ++j)
				os << "    ";
			os << i << ":";
			if(items[i].isNull())
				os << " (Null)" << std::endl;
			else
			if(items[i]->getArray() == NULL)
			{
				items[i]->write(os);
				os << std::endl;
			}
			else
			{
				os << " " << std::hex << (size_t)(&*items[i]) << std::dec << std::endl;
				items[i]->getArray()->debugWrite(os, level + 1);
			}
		}
	}

	void init(size_t numItems, AxiomItem defaultSrc)
	{
		size = numItems;
		defaultItem = defaultSrc;
		for(size_t i = 0; i < AXIOM_ITEM_ARRAY_BASE_SIZE; ++i)
			items[i].clear();
		if(size <= AXIOM_ITEM_ARRAY_BASE_SIZE)
			delta = 0;
		else
		{
			delta = AXIOM_ITEM_ARRAY_BASE_SIZE;
			while(size > delta)
			{
				delta <<= AXIOM_ITEM_ARRAY_BASE_POWER;
			}
		}
	}

	virtual void write(std::ostream &os)
	{
		os << "{array ";
		os << size;
		os << "}";
	}

	AxiomItemArray()
	{
		size = 0;
		delta = 0;
	}

	virtual void clear(BaseObjectPool *pool, size_t indexT)
	{
		AxiomItemBase::clear(pool, indexT);
		setType(AXIOM_ITEM_ATOM_ARRAY);
		size = 0;
		delta = 0;
		defaultItem.clear();
		for(size_t i = 0; i < AXIOM_ITEM_ARRAY_BASE_SIZE; ++i)
			items[i].clear();
	}
	
	virtual ~AxiomItemArray()
	{
	}
	
	template <class AxiomStateT>
	AxiomItem get(size_t index, AxiomStateT *state)
	{
		AxiomItem cur;
		cur.setTo(this, false, state->getContainer());
		int pos = -1;
		AxiomItem result = internalGet(cur, index, pos);
		AxiomItemArray *r = result->getArray();
		if(pos != -1)
		{
			if(r->items[pos].isNull())
			{
				if(defaultItem.isNull())
					return state->getNil();		// oops!
				AxiomItem myIndex;
				myIndex.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_INTEGER), true, state->getContainer());
				myIndex->getInteger()->init(index, AXIOM_ITEM_ARRAY_BASE_SIZE, state->getNil());
				AxiomItem x = state->cons(myIndex, state->getNil());
				AxiomItem y = state->cons(defaultItem, x);
				y->setQuoted(false);
				return y;
			}
			return r->items[pos];
		}
		return state->getNil();
	}
	
	// Warning: this returns an AxiomItem with a NULL target if something goes
	// wrong, i.e. we're out of range. Otherwise it returns a copy of the input
	// array, modified so as to have the item at index 'requestedIndex' set to
	// 'value'. The new array is returned and does not affect the original array.
	template <class AxiomStateT>
	AxiomItem set(size_t requestedIndex, AxiomItem value, AxiomStateT *state)
	{
		if(requestedIndex >= size)
		{
			AxiomItem x;
			return x;
		}
		int pos = -1;
		AxiomItem a;
		a.setTo(this, false, state->getContainer());
		AxiomItem b = internalGet(a, requestedIndex, pos);
		if(pos == -1)
		{
			AxiomItem x;
			return x;
		}
		AxiomItem oldValue = b->getArray()->items[pos];
		if(oldValue == value)
			return a;		// nothing to do!
		
		AxiomItem result;
		result.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_ARRAY), true, state->getContainer());
		AxiomItemArray *r = result->getArray();
		r->initCopy(this);
		AxiomItem rootResult = result;
		
		if(r->delta == 0)
		{
			r->items[pos] = value;
			return rootResult;
		}
		
		// The level we copied, the root level, is NOT the last level.
		AxiomItem tmp = result;
		size_t startIndex = 0;
		for(;;)
		{
			AxiomItemArray *t = tmp->getArray();
			size_t step = t->delta >> AXIOM_ITEM_ARRAY_BASE_POWER;
			size_t imid;
			if(step == 0)
				imid = requestedIndex - startIndex;
			else
			{
				size_t imin = 0, imax = AXIOM_ITEM_ARRAY_BASE_SIZE - 1;
				bool found = false;
				while(imax >= imin)
				{
					imid = (imin + imax) / 2;
					if(requestedIndex >= (size_t)((imid + 1) * step + startIndex))
						imin = imid + 1;
					else
					if(requestedIndex < (size_t)(imid * step + startIndex))
						imax = imid - 1;
					else
					{
						found = true;
						break;
					}
				}
				if(!found)
				{
					result.clear();
					return result;		// something went wrong!
				}
			}

			if(t->delta == 0)
			{
				t->items[imid] = value;
				return rootResult;
			}
			
			// tmp->items[imid] may be NULL here.

			startIndex += step * imid;
			
			// Allocate a new structure.
			result.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_ARRAY), true, state->getContainer());
			AxiomItemArray *r = result->getArray();
			if(t->items[imid].isNull())
			{
				r->size = t->size;		// just init to something sane
				r->defaultItem = t->defaultItem;
				for(size_t i = 0; i < AXIOM_ITEM_ARRAY_BASE_SIZE; ++i)
					r->items[i].clear();	// this is redundant
				r->delta = t->delta >> AXIOM_ITEM_ARRAY_BASE_POWER;	// delta is a power of AXIOM_ITEM_ARRAY_BASE_POWER
				if(r->delta <= AXIOM_ITEM_ARRAY_BASE_SIZE)
					r->delta = 0;
			}
			else
				r->initCopy(t->items[imid]->getArray());
			
			t->items[imid] = result;
			tmp = result;
		}
		
		result.clear();
		return result;			// something went wrong
	}
	
private:	
	static AxiomItem internalGet(AxiomItem root, size_t requestedIndex, int &outPos)
	{
		AxiomItemArray *r = root->getArray();
		outPos = -1;
		if(requestedIndex >= r->size)
		{
			AxiomItem x;
			return x;					// exceeded array bounds
		}
		if(r->delta == 0)
		{
			if(requestedIndex < AXIOM_ITEM_ARRAY_BASE_SIZE)
			{
				outPos = requestedIndex;
				return root;			// items in root level are actually leaves of the tree
			}
			AxiomItem x;
			return x;					// something went wrong
		}

		AxiomItem tmp = root;
		size_t startIndex = 0;
		for(;;)
		{
			AxiomItemArray *t = tmp->getArray();
			size_t step = t->delta >> AXIOM_ITEM_ARRAY_BASE_POWER;
			size_t imin = 0, imax = AXIOM_ITEM_ARRAY_BASE_SIZE - 1;
			bool found = false;
			size_t imid;
			while(imax >= imin)
			{
				imid = (imin + imax) / 2;
				if(requestedIndex >= (size_t)((imid + 1) * step + startIndex))
					imin = imid + 1;
				else
				if(requestedIndex < (size_t)(imid * step + startIndex))
					imax = imid - 1;
				else
				{
					found = true;
					break;
				}
			}
			if(!found)
			{
				tmp.clear();
				break;
			}
			if(t->items[imid].isNull())
			{
				outPos = imid;
				break;
			}
			startIndex += step * imid;
			tmp = t->items[imid];
			if(tmp->getArray()->delta == 0)
			{
				outPos = requestedIndex - startIndex;
				return tmp;
			}
		}
		return tmp;		// tmp may be NULL on output
	}
};

#endif	// l_crudasm__axiom_adt_h__included
