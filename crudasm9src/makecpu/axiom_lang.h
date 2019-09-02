// axiom_lang.h - Copyright (C) 2012 Willow Schlanger. All rights reserved.

#ifndef l_crudasm__axiom_lang_h__included
#define l_crudasm__axiom_lang_h__included

#define AXIOM_LANGUAGE_DEBUG_LEVEL 1	/* change to 0 for release builds */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <stddef.h>
#include "growable_array.h"
#include <map>
#include <string>
#include <list>
#include <set>
#include <stdio.h>
#include <cstring>

#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
#include <iostream>
#endif

namespace AxiomLanguage
{

typedef unsigned long long U8;
typedef unsigned int U4;
typedef unsigned short U2;
typedef unsigned char U1;

typedef signed long long S8;
typedef signed int S4;
typedef signed short S2;
typedef signed char S1;

enum
{
	AXIOM_ITEM_LIST,
	AXIOM_ITEM_ATOM_IDENTIFIER,
	AXIOM_ITEM_ATOM_SCOPE,
	AXIOM_ITEM_ATOM_STRING,
	AXIOM_ITEM_ATOM_INTEGER,
	AXIOM_ITEM_ATOM_FUN_DEF,			// function definition (also created by lambda)
	AXIOM_ITEM_ATOM_FUN_ARG,
	AXIOM_ITEM_ATOM_FUN_REF,			// contains a ref. to a scope, a ptr. to a symbol name atom, and a target ptr cache
	AXIOM_ITEM_ATOM_FUN_REF_INTERNAL,	// used by recursive (or mutually recursive) functions defined in a module
	AXIOM_ITEM_ATOM_ARRAY,
	AXIOM_ITEM_ATOM_CPU_ARCH,

	AXIOM_ITEM__COUNT
};

// The reason we have keyword 'codes' is because then we can use a switch statement which is faster
// than a series of if statements to compare pointers.
enum
{
	AXIOM_KEYWORD__USER,
	AXIOM_KEYWORD_NIL,
	AXIOM_KEYWORD_LIST,
	AXIOM_KEYWORD_VOID,
	AXIOM_KEYWORD_ASGN,
	AXIOM_KEYWORD_LAMBDA,
	AXIOM_KEYWORD_NAMESPACE,
	AXIOM_KEYWORD_MODULE,
	AXIOM_KEYWORD_SELF,				// 'self' keyword is not supposed to do anything useful
	AXIOM_KEYWORD_FUNSELF,			// just generates an error message if you try to use it (to prevent recursion outside of a module)
	AXIOM_KEYWORD_FUN,
	AXIOM_KEYWORD_RETURN,
	AXIOM_KEYWORD_PUBLIC,
	AXIOM_KEYWORD_AXIOM,
	AXIOM_KEYWORD_USING,
	AXIOM_KEYWORD_INCLUDE,
	AXIOM_KEYWORD_LOADCPU,
	AXIOM_KEYWORD_DEBUG,			// prints a message at lexical parse time
	AXIOM_KEYWORD_PLUS,				// unimplemented
	AXIOM_KEYWORD_MINUS,			// unimplemented

	AXIOM_KEYWORD__COUNT
};

enum
{
	AXIOM_SCOPE_TYPE_GENERAL,
	AXIOM_SCOPE_TYPE_NAMESPACE,		// may have a name
	AXIOM_SCOPE_TYPE_MODULE			// may have a name
};

#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
extern size_t numObjectsGlobal;
#endif

class BaseObjectPool
{
public:
	virtual ~BaseObjectPool() { }
	virtual class BaseItem *get(size_t index) = 0;
	virtual void remove(class BaseItem *item) = 0;
	virtual class BaseItem *alloc() = 0;
};

class BaseItem
{
protected:
	BaseItem *forwarder;
public:
	BaseItem()
	{
		forwarder = NULL;
	}
	virtual ~BaseItem()
	{
	}
	BaseItem *getForwarder()
	{
		return forwarder;
	}
	void setForwarder(BaseItem *item)
	{
		if(forwarder != item)
			forwarder = item;
	}
	virtual U8 getIndex() = 0;
	virtual void setIndex(U8 index) = 0;
	virtual void clear(BaseObjectPool *pool, size_t index) = 0;	// also sets index to 'index' and 'forwarder' to NULL
	virtual size_t getRefCount() = 0;	// OK to return 0 if object is not reference counted
	virtual void addRef() = 0;
	virtual bool removeRef() = 0;		// returns true if we should remove the object
	virtual U4 getType() const = 0;
};

template <class T>
class ObjectPool :
	public BaseObjectPool
{
	CrudasmUtilities::growable_array<T> items;
	BaseItem *available;
public:
	virtual BaseItem *get(size_t index)
	{
		return &(*(items.begin() + index));
	}
	ObjectPool()
	{
		available = NULL;
	}
	virtual ~ObjectPool()
	{
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
		size_t n = items.size();
		for(size_t i = 0; i < n; ++i)
		{
			BaseItem *item = get(i);
			if(item->getRefCount() != 0)
			{
				std::cout << "NOTE: Object of type " << item->getType() << " had a reference count of " << item->getRefCount() << std::endl;
			}
		}
#endif
	}
	virtual BaseItem *alloc()
	{
		if(available != NULL)
		{
			BaseItem *x = available;
			available = available->getForwarder();
			x->clear(this, (size_t)x->getIndex());
			((T *)(x))->initRefs();
			return (T *)(x);
		}
		size_t index = items.size();
		T &x = items.push_back();
		x.clear(this, (size_t)index);
		((T *)(&x))->initRefs();
		return &x;
	}
	virtual void remove(BaseItem *item)
	{
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
		if(item->getType() == AXIOM_ITEM__COUNT)
		{
			std::cout << "WARNING! Attempt to remove invalid item!" << std::endl;
		}
		if(item->getForwarder() != NULL)
		{
			std::cout << "WARNING: removing an item with non-NULL eval!" << std::endl;
			item->setForwarder(NULL);
		}
		if(item->getRefCount() != 0)
		{
			std::cout << "WARNING: removing an item with non-zero reference count!" << std::endl;
		}
#endif
		item->clear(this, (size_t)item->getIndex());
		item->setForwarder(available);
		available = item;
	}
};

class AxiomItemBase :
	public BaseItem
{
	U8 type : 6;
	U8 quoted : 1;
	U8 argfun : 1;		// are we a function of any arguments (direct or indirect?)
	U8 index : 56;
	size_t refCount;
protected:
	void setType(U4 typeT)
	{
		type = typeT;
	}
public:
	void setQuoted(bool q)
	{
		if(q)
			quoted = 1;
		else
			quoted = 0;
	}
	void setArgFun(bool q)
	{
		if(q)
			argfun = 1;
		else
			argfun = 0;
	}
	bool isQuoted() const
	{
		return quoted != 0;
	}
	bool isArgFun() const
	{
		return argfun != 0;
	}
	virtual void write(std::ostream &os) = 0;
	bool isList()
	{
		return getType() == AXIOM_ITEM_LIST;
	}
	class AxiomItemString *getString()
	{
		if(getType() == AXIOM_ITEM_ATOM_STRING)
			return (class AxiomItemString *)(this);
		return NULL;
	}
	class AxiomItemList *getList()
	{
		if(type == AXIOM_ITEM_LIST)
			return (class AxiomItemList *)(this);
		return NULL;
	}
	class AxiomItemCpuArch *getCpuArch()
	{
		if(type == AXIOM_ITEM_ATOM_CPU_ARCH)
			return (class AxiomItemCpuArch *)(this);
		return NULL;
	}
	class AxiomItemFunDef *getFunDef()
	{
		if(type == AXIOM_ITEM_ATOM_FUN_DEF)
			return (class AxiomItemFunDef *)(this);
		return NULL;
	}
	class AxiomItemFunRef *getFunRef()
	{
		if(type == AXIOM_ITEM_ATOM_FUN_REF)
			return (class AxiomItemFunRef *)(this);
		return NULL;
	}
	class AxiomItemFunRefInternal *getFunRefInternal()
	{
		if(type == AXIOM_ITEM_ATOM_FUN_REF_INTERNAL)
			return (class AxiomItemFunRefInternal *)(this);
		return NULL;
	}
	class AxiomItemFunArg *getFunArg()
	{
		if(type == AXIOM_ITEM_ATOM_FUN_ARG)
			return (class AxiomItemFunArg *)(this);
		return NULL;
	}
	class AxiomItemInteger *getInteger()
	{
		if(type == AXIOM_ITEM_ATOM_INTEGER)
			return (class AxiomItemInteger *)(this);
		return NULL;
	}
	class AxiomItemIdentifier *getIdentifier()
	{
		if(getType() == AXIOM_ITEM_ATOM_IDENTIFIER)
			return (class AxiomItemIdentifier *)(this);
		return NULL;
	}
	class AxiomItemArray *getArray()
	{
		if(getType() == AXIOM_ITEM_ATOM_ARRAY)
			return (AxiomItemArray *)(this);
		return NULL;
	}
	class AxiomItemScope *getScope()
	{
		if(getType() == AXIOM_ITEM_ATOM_SCOPE)
			return (class AxiomItemScope *)(this);
		return NULL;
	}
	AxiomItemBase()
	{
		refCount = 1;
		index = 0;
		quoted = 1;
		type = AXIOM_ITEM__COUNT;
		argfun = 0;
	}
	virtual U4 getType() const
	{
		return type;
	}
	virtual ~AxiomItemBase()
	{
		if(type == AXIOM_ITEM__COUNT)
			return;
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
		if(refCount != 0)
		{
			std::cout << "Item of type " << type << " left active -- ref. count was " << refCount << "!" << std::endl;
		}
#endif
		type = AXIOM_ITEM__COUNT;
	}
	virtual U8 getIndex()
	{
		return index;
	}
	virtual void setIndex(U8 indexT)
	{
		index = indexT;
	}
	virtual void clear(BaseObjectPool *, size_t indexT)
	{
		setForwarder(NULL);		// this is expected of us!
		type = AXIOM_ITEM__COUNT;
		quoted = 1;
		index = indexT;
		refCount = 0;
		argfun = 0;
	}
	void initRefs()
	{
		if(refCount == 0)
			refCount = 1;
	}
	virtual size_t getRefCount()
	{
		return refCount;
	}
	virtual void addRef()
	{
		if(refCount != 0)
			++refCount;
	}
	virtual bool removeRef()
	{
		if(refCount == 0)
			return false;
		--refCount;
		if(refCount == 0)
		{
			return true;
		}
		return false;
	}
};

class AxiomItemContainerBase
{
public:
	virtual void remove(class AxiomItemBase *item) = 0;
};

class AxiomItem
{
protected:
	AxiomItemContainerBase *container;
	AxiomItemBase *target;
	AxiomItemBase *getTarget()
	{
		AxiomItemBase *tmp = target;
		if(tmp != NULL)
		{
#if 0
			while(tmp->getForwarder() != NULL)
				tmp = (AxiomItemBase *)tmp->getForwarder();
#else
			// If we do something like this: (asgn c (list 'x 'x)) (asgn a (list c c)) (asgn b (list a a)) b
			// When 'b' is evaluated we don't want to have to traverse the forwarding addresses each time 'a'
			// is encountered.
			if(target->getForwarder() != NULL)
			{
				AxiomItemBase *newTarget = target;
				while(newTarget->getForwarder() != NULL)
				{
					// 'newTarget' has a forwarding address.
					tmp = (AxiomItemBase *)newTarget->getForwarder();
					newTarget = tmp;
				}

				setTo(newTarget, false, container);
			}

			return target;
#endif
		}
		return tmp;
	}
public:
	bool isNull()
	{
		return getTarget() == NULL;
	}
	AxiomItemContainerBase *getContainer()
	{
		return container;
	}
	AxiomItem()
	{
		container = NULL;
		target = NULL;
	}
	AxiomItemBase *operator->()
	{
		return getTarget();
	}
	AxiomItemBase &operator*()
	{
		return *getTarget();
	}
	AxiomItem(const AxiomItem &src)
	{
		container = src.container;
		target = NULL;
		setTo(src.target, false, src.container);
	}
	void operator=(const AxiomItem &src)
	{
		if(&src == this)
			return;
		setTo(src.target, false, src.container);
	}
	bool operator==(const AxiomItem &src) const
	{
		return target == src.target;
	}
	bool operator!=(const AxiomItem &src) const
	{
		return target != src.target;
	}
	bool operator<(const AxiomItem &src) const
	{
		return target < src.target;
	}
	void setTo(AxiomItemBase *src, bool decreaseRef, AxiomItemContainerBase *conT)
	{
		if(conT != NULL)
			container = conT;
		doSet(src);
		if(src != NULL && decreaseRef)
			src->removeRef();	// this should not return false
	}
	~AxiomItem()
	{
		removeTarget();
	}
	void clear()
	{
		removeTarget();
	}
private:
	void removeTarget()
	{
		if(target != NULL && container != NULL)
		{
			if(target->removeRef())
			{
				if(target->getForwarder() != NULL)
				{
					AxiomItemBase *prev = target;
					std::list<AxiomItemBase *> lst;
					for(AxiomItemBase *tmp = (AxiomItemBase *)target->getForwarder(); tmp != NULL;)
					{
						AxiomItemBase *next = (AxiomItemBase *)tmp->getForwarder();
						if(!tmp->removeRef())
						{
							prev->setForwarder(NULL);
							break;
						}
						lst.push_back(tmp);
						//container->remove(tmp);
						prev->setForwarder(NULL);
						prev = tmp;
						tmp = next;
					}
					for(std::list<AxiomItemBase *>::iterator i = lst.begin(); i != lst.end(); ++i)
					{
						container->remove(*i);
					}
				}
				container->remove(target);
			}
		}
		target = NULL;
	}
	void doSet(AxiomItemBase *src)
	{
		if(src == target || container == NULL)
		{
			if(src == NULL)
				removeTarget();
			return;
		}
		if(src != NULL)
			src->addRef();
		removeTarget();
		target = src;
	}
};

// Individual classes using AxiomItemBase as a base class go here.

class AxiomItemIdentifier :
	public AxiomItemBase
{
	const std::string *name;
	U4 code;		// AXIOM_KEYWORD__USER means user identifier
public:
	void write(std::ostream &os)
	{
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 2
		if(isArgFun())
			os << "@";
#endif
		os << *name;
	}
	std::string getName() const
	{
		return *name;
	}
	U4 getCode() const
	{
		return code;
	}
	AxiomItemIdentifier()
	{
		name = NULL;
		code = 0;
	}
	virtual ~AxiomItemIdentifier()
	{
	}
	virtual void clear(BaseObjectPool *pool, size_t indexT)
	{
		AxiomItemBase::clear(pool, indexT);
		setType(AXIOM_ITEM_ATOM_IDENTIFIER);
		name = NULL;
		code = AXIOM_KEYWORD__USER;
	}
	void init(const std::string *nameT, U4 codeT)
	{
		name = nameT;
		code = codeT;
	}
};

class AxiomItemScope :
	public AxiomItemBase
{
	AxiomItemBase *parent;			// parent AxiomItemScope. NULL for root scope.
	AxiomItem other_item;			// other AxiomItemScope we're using (prev namespace)
	// The following item maps a quoted identifier atom (key) to an item object.
	std::map<AxiomItemBase *, AxiomItem> symbols;
	// scopeType: AXIOM_SCOPE_TYPE_GENERAL, AXIOM_SCOPE_TYPE_NAMESPACE, or AXIOM_SCOPE_TYPE_MODULE
	char scopeType;
	AxiomItem scopeName;
public:
	AxiomItemBase *getParent()
	{
		return parent;
	}
	std::map<AxiomItemBase *, AxiomItem> &getDirectSymbols()
	{
		return symbols;
	}
	AxiomItemScope()
	{
		scopeType = AXIOM_SCOPE_TYPE_GENERAL;
	}
	virtual ~AxiomItemScope()
	{
	}
	virtual void clear(BaseObjectPool *poolT, size_t indexT)
	{
		AxiomItemBase::clear(poolT, indexT);
		setType(AXIOM_ITEM_ATOM_SCOPE);
		scopeType = AXIOM_SCOPE_TYPE_GENERAL;
		parent = NULL;
		other_item.clear();
		symbols.clear();
		scopeName.clear();
	}
	void init(char type = AXIOM_SCOPE_TYPE_GENERAL)
	{
		parent = NULL;
		other_item.clear();
		symbols.clear();
		scopeName.clear();
		scopeType = type;
	}
	void setScopeType(char type)
	{
		scopeType = type;
	}
	char getScopeType() const
	{
		return scopeType;
	}
	void setParent(AxiomItem parentT)
	{
		parent = &*parentT;
	}
	void setName(AxiomItem nameT)
	{
		scopeName = nameT;
	}
	// NOTE--you can have only one 'other' ! This is designed for continuation namespaces.
	void addOther(AxiomItem item)
	{
		other_item = item;
	}
	void receiveSymbolsFromScope(AxiomItem srcScope)
	{
		if(srcScope->getScope() != NULL)
		{
			AxiomItemBase *cur = &*srcScope;
			std::list<AxiomItemBase *> all;
			while(cur != NULL)
			{
				all.push_front(cur);
				cur = &*cur->getScope()->other_item;
			}
			for(std::list<AxiomItemBase *>::iterator j = all.begin(); j != all.end(); ++j)
			{
				for(std::map<AxiomItemBase *, AxiomItem>::iterator i = (*j)->getScope()->symbols.begin();
					i != (*j)->getScope()->symbols.end();
					++i
				)
				{
					if(i->second->getIdentifier() != NULL)
					{
						if(i->second->getIdentifier()->getCode() == AXIOM_KEYWORD_SELF)
							continue;
					}
					add(i->first, i->second);
				}
			}
		}
	}
	void add(AxiomItemBase *identifier, AxiomItem target)
	{
		symbols[identifier] = target;
	}
	void write(std::ostream &os)
	{
		os << "{";
		if(scopeType == AXIOM_SCOPE_TYPE_NAMESPACE)
		{
			os << "namespace ";
			if(scopeName.isNull())
				os << "{anonymous}";
			else
				scopeName->write(os);
		}
		else
		if(scopeType == AXIOM_SCOPE_TYPE_MODULE)
		{
			os << "module ";
			if(scopeName.isNull())
				os << "{anonymous}";
			else
				scopeName->write(os);
		}
		else
		{
			if(parent == NULL)
				os << "root scope";
			else
				os << "anonymous scope";
		}
		std::set<AxiomItemBase *> syms;
		if(!getSymbolsDirect(syms))
		{
			os << " {error}}" << std::endl;
			return;
		}
		if(!other_item.isNull())
		{
			if(other_item->getScope() != NULL)
				other_item->getScope()->getSymbolsDirect(syms);
		}
		std::set<std::string> names;
		size_t num = 0;
		bool more = false;
		for(std::set<AxiomItemBase *>::iterator i = syms.begin(); i != syms.end(); ++i)
		{
			if((*i)->getIdentifier() == NULL)
				continue;
			std::string s = (*i)->getIdentifier()->getName();
			if(!scopeName.isNull() && scopeName->getIdentifier() != NULL)
			{
				// skip 'self' for named namespaces/modules.
				if(&*scopeName == *i)
					continue;
			}
			if(names.find(s) == names.end())
			{
				++num;
				names.insert(s);
			}
			if(num == 200)
			{
				more = true;
				break;
			}
		}

		size_t count = 0;
		for(std::set<std::string>::iterator i = names.begin(); i != names.end(); ++i)
		{
			if(count == 0)
				os << " {";
			else
				os << ", ";
			os << *i;
			++count;
			if(count == num)
			{
				if(more)
					os << " {plus more symbols}";
				break;
			}
		}
		if(count != 0)
			os << "}";
		os << "}";
	}
	AxiomItem search(AxiomItemBase *ident, bool searchParentScope)
	{
		std::map<AxiomItemBase *, AxiomItem>::iterator i = symbols.find(ident);
		if(i != symbols.end())
			return i->second;
		if(!other_item.isNull())
		{
			AxiomItem &item = other_item;
			AxiomItemScope *s = item->getScope();
			if(s != NULL)
			{
				AxiomItem result = s->search(ident, false);
				if(!result.isNull())
					return result;
			}
		}
		AxiomItem result;
		if(searchParentScope && parent != NULL && parent->getScope() != NULL)
			result = parent->getScope()->search(ident, true);
		return result;
	}
private:
	bool getSymbolsDirect(std::set<AxiomItemBase *> &dest)
	{
		for(std::map<AxiomItemBase *, AxiomItem>::iterator i = symbols.begin(); i != symbols.end(); ++i)
		{
			if(i->first == NULL)
				return false;
			dest.insert(i->first);
		}
		return true;
	}
};

class AxiomItemList :
	public AxiomItemBase
{
	AxiomItem first;
	AxiomItem rest;
	BaseObjectPool *pool;
public:
	AxiomItem getFirst()
	{
		return first;
	}
	AxiomItem getRest()
	{
		return rest;
	}
	void setFirst(AxiomItem x)
	{
		first = x;
	}
	void setRest(AxiomItem x)
	{
		rest = x;
	}
	void set(AxiomItem firstT, AxiomItem restT)
	{
		first = firstT;
		rest = restT;
	}
	AxiomItemList()
	{
		pool = NULL;
	}
	virtual ~AxiomItemList()
	{
	}
	bool isNil()
	{
		return first.isNull();
	}
	virtual void clear(BaseObjectPool *poolT, size_t indexT)
	{
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 2
		if(!isNil())
		{
			std::cout << "REMOVING LIST - ";
			write(std::cout);
			std::cout << std::endl;
		}
#endif
		pool = poolT;
		first.clear();
		rest.clear();
		AxiomItemBase::clear(pool, indexT);
		setType(AXIOM_ITEM_LIST);
	}
	void init(AxiomItemContainerBase *container)
	{
		first.setTo(NULL, false, container);
		rest.setTo(NULL, false, container);
	}
	void write(std::ostream &os)
	{
		if(getType() == AXIOM_ITEM__COUNT)
		{
			os << "<error!>";
			return;
		}
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 2
		//os << std::hex << " <" << (size_t)(this) << "> " << std::flush;
		if(isArgFun())
			os << "@";
#endif
		if(first.isNull())
		{
			os << "()";
			return;
		}
		os << "(";
		bool needSpace = false;
		AxiomItem cur;
		cur.setTo(this, false, first.getContainer());
		for(; !cur->getList()->isNil(); cur = cur->getList()->getRest())
		{
			if(needSpace)
				os << " ";
			needSpace = true;
			cur->getList()->getFirst()->write(os);
		}
		os << ")";
	}
};

//Need to add support for reading this class of atom.
class AxiomItemFunArg :
	public AxiomItemBase
{
	AxiomItemBase *fcn_owner;
	size_t argNum;
public:
	AxiomItemBase *getOwner()
	{
		return fcn_owner;
	}
	size_t getArgNum() const
	{
		return argNum;
	}
	void write(std::ostream &os)
	{
		os << "{arg " << argNum << "}";
	}
	AxiomItemFunArg()
	{
		fcn_owner = NULL;
		argNum = 0;
	}
	virtual ~AxiomItemFunArg()
	{
	}
	virtual void clear(BaseObjectPool *pool, size_t indexT)
	{
		AxiomItemBase::clear(pool, indexT);
		setType(AXIOM_ITEM_ATOM_FUN_ARG);
		fcn_owner = NULL;
		argNum = 0;
	}
	void init(AxiomItemBase *ownerT, size_t argNumT)
	{
		argNum = argNumT;
		fcn_owner = ownerT;
	}
};

//Need to add support for reading this class of atom.
class AxiomItemFunDef :
	public AxiomItemBase
{
	AxiomItem name;			// this should be a quoted user identifier, or NULL
	AxiomItem args;			// this should be a list -- nil or a quoted list of argument names
	AxiomItem body;			// the thing this function is defined as
	size_t numArgs;			// number of arguments
public:
	void write(std::ostream &os)
	{
		os << "{function ";
		if(name.isNull())
			os << "lambda";
		else
			name->write(os);
		os << " ";
		args->write(os);
#if 0	// don't do this, as it might lead to infinite spew in case of recursion !
os << " :body: ";
body->write(os);
#endif
		os << "}";
	}
	size_t getNumArgs() const
	{
		return numArgs;
	}
	AxiomItem getBody()
	{
		return body;
	}
	AxiomItemFunDef()
	{
		name.clear();
		args.clear();
		body.clear();
		numArgs = 0;
	}
	virtual ~AxiomItemFunDef()
	{
	}
	virtual void clear(BaseObjectPool *pool, size_t indexT)
	{
		AxiomItemBase::clear(pool, indexT);
		setType(AXIOM_ITEM_ATOM_FUN_DEF);
		name.clear();
		args.clear();
		body.clear();
		numArgs = 0;
	}
	void init(AxiomItem nameOptT, AxiomItem argsT)
	{
		numArgs = 0;
		for(AxiomItem x = argsT; !x->getList()->isNil(); x = x->getList()->getRest())
		{
			++numArgs;
		}
		name = nameOptT;
		args = argsT;
		body.clear();
	}
	void setBody(AxiomItem bodyT)
	{
		body = bodyT;
	}
};

//Need to add support for reading this class of atom.
class AxiomItemString :
	public AxiomItemBase
{
	std::string s;
public:
	AxiomItemString()
	{
	}
	virtual ~AxiomItemString()
	{
	}
	virtual void clear(BaseObjectPool *poolT, size_t indexT)
	{
		AxiomItemBase::clear(poolT, indexT);
		setType(AXIOM_ITEM_ATOM_STRING);
		s.clear();
	}
	void init(std::string t)
	{
		s = t;
	}
	std::string getText()
	{
		return s;
	}
	void write(std::ostream &os)
	{
		os << "\"";
		for(std::string::iterator i = s.begin(); i != s.end(); ++i)
		{
			if(*i == '\"')
				os << "\"";
			else
			if(*i == '\r')
				os << "\\r";
			else
			if(*i == '\n')
				os << "\\n";
			else
			if(*i == '\\')
				os << "\\";
			else
			if(*i == '\t')
				os << "\\t";
			else
				os << *i;
		}
		os << "\"";
	}
};

//Need to add support for reading this class of atom.
class AxiomItemInteger :
	public AxiomItemBase
{
	AxiomItem nonScalar;	// this is either the nil list or a list of non-scalar items we are to add together
	union
	{
		U8 qword;
		U4 dword;
		U2 word;
		U1 byte;
	}	scalar;
	U1 sizeBytes : 7;	// this should be 1, 2, 4, 8, or 0 (indicating boolean type)
	U1 isHex : 1;
public:
	AxiomItemInteger()
	{
	}
	virtual ~AxiomItemInteger()
	{
	}
	virtual void clear(BaseObjectPool *poolT, size_t indexT)
	{
		AxiomItemBase::clear(poolT, indexT);
		setType(AXIOM_ITEM_ATOM_INTEGER);
		scalar.qword = 0;
		sizeBytes = 8;
		isHex = 0;
		nonScalar.clear();
	}
	void init(U8 valueT, U1 sizeBytesT, AxiomItem nonScalarT)
	{
		isHex = 0;
		sizeBytes = sizeBytesT;
		scalar.qword = valueT;
		nonScalar = nonScalarT;
	}
	U1 getSizeBytes() const
	{
		return sizeBytes;
	}
	U8 getScalarValue()
	{
		if(sizeBytes == 0)
			return scalar.byte & 1;	// boolean integer type
		if(sizeBytes == 1)
			return scalar.byte;
		if(sizeBytes == 2)
			return scalar.word;
		if(sizeBytes == 4)
			return scalar.dword;
		return scalar.qword;
	}
	void write(std::ostream &os)
	{
		doWrite(os, isHex);
	}
private:
	void doWrite(std::ostream &os, U1 isHex)
	{
		if(nonScalar.isNull() || nonScalar->getList() == NULL)
		{
			os << "<invalid integer>";
			return;
		}

		if(getScalarValue() == 0 || nonScalar->getList()->isNil())
		{
			doWriteScalar(os, isHex);
			return;
		}

		os << "(+ ";
		if(getScalarValue() != 0)
		{
			doWriteScalar(os, isHex);
		}

		for(AxiomItem cur = nonScalar; !cur->getList()->isNil(); cur = cur->getList()->getRest())
		{
			os << " ";
			cur->getList()->getFirst()->getInteger()->doWrite(os, isHex);
		}

		os << ")";
	}
	void doWriteScalar(std::ostream &os, U1 isHex)
	{
		if(sizeBytes == 8)
		{
			writeScalar(os, getScalarValue(), sizeBytes, isHex);
		}
		else
		{
			os << "(";
			writeSize(os, sizeBytes);
			os << " ";
			writeScalar(os, getScalarValue(), sizeBytes, isHex);
			os << ")";
		}
	}
	static void writeScalar(std::ostream &os, U8 valueT, U1 sizeBytesT, U1 isHexT)
	{
		using namespace std;
		if(!isHexT)
		{
			switch(sizeBytesT)
			{
			case 0:
				os << (valueT & 1);
				break;
			case 1:
				os << (S2)(S1)(valueT);
				break;
			case 2:
				os << (S2)(valueT);
				break;
			case 4:
				os << (S4)(valueT);
				break;
			case 8:
				os << (S8)(valueT);
				break;
			default:
				os << "<unknown value>";
			}
		}
		else
		{
			char buf[64];
			switch(sizeBytesT)
			{
			case 0:
				os << (valueT & 1);
				break;
			case 1:
				sprintf(buf, "0x%02x", (U4)(U1)(valueT));
				break;
			case 2:
				sprintf(buf, "0x%04x", (U4)(U2)(valueT));
				break;
			case 4:
				sprintf(buf, "0x%08x", (U4)(valueT));
				break;
			case 8:
				sprintf(buf, "0x%08x%08x", (U4)(((valueT >> 16) >> 16) & 0xffffffff), (U4)((valueT >> 0) & 0xffffffff));
				break;
			default:
				os << "<unknown value>";
				return;
			}
			os << buf;
		}
	}
	static void writeSize(std::ostream &os, U1 sizeBytesT)
	{
		if(sizeBytesT == 0)
			os << "BIT";
		else
		if(sizeBytesT == 1)
			os << "BYTE";
		else
		if(sizeBytesT == 2)
			os << "WORD";
		else
		if(sizeBytesT == 4)
			os << "DWORD";
		else
		if(sizeBytesT == 8)
			os << "QWORD";
		else
			os << "<unknown size>";
	}
	static void writeHex(std::ostream &os, U8 valueT, U1 sizeBytesT)
	{
	}
};

class AxiomItemFunRef :
	public AxiomItemBase
{
	AxiomItem targetModule;
	AxiomItemBase *name;
	AxiomItemBase *targetCache;
public:
	AxiomItemFunRef()
	{
	}
	virtual ~AxiomItemFunRef()
	{
	}
	AxiomItemBase *getName()
	{
		return name;
	}
	AxiomItemBase *getTarget()
	{
		if(targetCache != NULL)
			return targetCache;
		if(targetModule.isNull())
			return NULL;
		AxiomItem x = targetModule->getScope()->search(name, false);
		if(x.isNull())
			return NULL;
		targetCache = &*x;
		return targetCache;
	}
	virtual void write(std::ostream &os)
	{
		if(targetModule.isNull() || targetModule->getScope() == NULL || targetModule->getScope()->getScopeType() != AXIOM_SCOPE_TYPE_MODULE ||
			name == NULL || name->getIdentifier() == NULL
		)
			os << "{invalid function reference}";
		else
		{
			AxiomItemBase *x = getTarget();
			if(x == NULL)
				os << "{function " << name->getIdentifier()->getName() << " (...)}";
			else
				x->write(os);
		}
	}
	virtual void clear(BaseObjectPool *poolT, size_t indexT)
	{
		AxiomItemBase::clear(poolT, indexT);
		setType(AXIOM_ITEM_ATOM_FUN_REF);
		targetModule.clear();
		name = NULL;
		targetCache = NULL;
	}
	void init(AxiomItem targetModuleT, AxiomItemBase *nameT)
	{
		targetModule = targetModuleT;
		name = nameT;
		targetCache = NULL;
	}
};

class AxiomItemFunRefInternal :
	public AxiomItemBase
{
	AxiomItemBase *targetModulePtr;
	AxiomItemBase *name;
	AxiomItemBase *targetCache;
public:
	AxiomItemFunRefInternal()
	{
	}
	virtual ~AxiomItemFunRefInternal()
	{
	}
	AxiomItemBase *getName()
	{
		return name;
	}
	AxiomItemBase *getTarget()
	{
		if(targetCache != NULL)
			return targetCache;
		if(targetModulePtr == NULL)
			return NULL;
		AxiomItem x = targetModulePtr->getScope()->search(name, false);
		if(x.isNull())
			return NULL;
		if(x->getFunDef() == NULL)
			return NULL;
		targetCache = &*x;
		return targetCache;
	}
	virtual void write(std::ostream &os)
	{
		if(targetModulePtr == NULL || targetModulePtr->getScope() == NULL || targetModulePtr->getScope()->getScopeType() != AXIOM_SCOPE_TYPE_MODULE ||
			name == NULL || name->getIdentifier() == NULL
		)
			os << "{invalid function reference}";
		else
		{
			AxiomItemBase *x = getTarget();
			if(x == NULL)
				os << "{function " << name->getIdentifier()->getName() << " (...)}";
			else
				x->write(os);
		}
	}
	virtual void clear(BaseObjectPool *poolT, size_t indexT)
	{
		AxiomItemBase::clear(poolT, indexT);
		setType(AXIOM_ITEM_ATOM_FUN_REF_INTERNAL);
		targetModulePtr = NULL;
		name = NULL;
		targetCache = NULL;
	}
	void init(AxiomItemBase *targetModulePtrT, AxiomItemBase *nameT)
	{
		targetModulePtr = targetModulePtrT;
		name = nameT;
		targetCache = NULL;
	}
};

#include "axiom_adt.h"
#include "axiom_cpu_arch.h"

class AxiomItemContainer :
	public AxiomItemContainerBase
{
	BaseObjectPool *pool[AXIOM_ITEM__COUNT];
public:
	AxiomItemContainer()
	{
		for(size_t i = 0; i < AXIOM_ITEM__COUNT; ++i)
		{
			pool[i] = NULL;
		}
		pool[AXIOM_ITEM_ATOM_IDENTIFIER] = new ObjectPool<AxiomItemIdentifier>();
		pool[AXIOM_ITEM_LIST] = new ObjectPool<AxiomItemList>();
		pool[AXIOM_ITEM_ATOM_SCOPE] = new ObjectPool<AxiomItemScope>();
		pool[AXIOM_ITEM_ATOM_STRING] = new ObjectPool<AxiomItemString>();
		pool[AXIOM_ITEM_ATOM_INTEGER] = new ObjectPool<AxiomItemInteger>();
		pool[AXIOM_ITEM_ATOM_FUN_DEF] = new ObjectPool<AxiomItemFunDef>();
		pool[AXIOM_ITEM_ATOM_FUN_ARG] = new ObjectPool<AxiomItemFunArg>();
		pool[AXIOM_ITEM_ATOM_FUN_REF] = new ObjectPool<AxiomItemFunRef>();
		pool[AXIOM_ITEM_ATOM_FUN_REF_INTERNAL] = new ObjectPool<AxiomItemFunRefInternal>();
		pool[AXIOM_ITEM_ATOM_ARRAY] = new ObjectPool<AxiomItemArray>();
		pool[AXIOM_ITEM_ATOM_CPU_ARCH] = new ObjectPool<AxiomItemCpuArch>();
	}
	~AxiomItemContainer()
	{
		for(size_t i =0 ; i < AXIOM_ITEM__COUNT; ++i)
		{
			if(pool[i] != NULL)
				delete pool[i];
		}
	}
	virtual void remove(AxiomItemBase *item)
	{
		if(item != NULL)
		{
			if(item->getType() != AXIOM_ITEM__COUNT)
				pool[item->getType()]->remove(item);
			else
			{
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
				std::cout << "NOTE: attempt to remove an item of type " << AXIOM_ITEM__COUNT << std::endl;
#endif
			}
		}
	}
	AxiomItemBase *alloc(size_t type)
	{
		return (AxiomItemBase *)(pool[type]->alloc());
	}
};

class AxiomStateBase
{
protected:
	AxiomItemContainer *container;
	std::map<std::string, AxiomItem> quoted_identifiers;

	virtual ~AxiomStateBase()
	{
		quoted_identifiers.clear();	// this will create dangling pointers of type const std::string *, which is fine
		delete container;			// ..because the situation will be resolved after 'container' is deleted
	}

public:
	AxiomItemContainer *getContainer()
	{
		return container;
	}
	std::map<std::string, AxiomItem> &getQuotedIdentifiers()
	{
		return quoted_identifiers;
	}
};

class AxiomState :
	public AxiomStateBase
{
public:
	AxiomItem k_void;
	AxiomItem k_plus;
	AxiomItem k_minus;
	AxiomItem k_system;
	AxiomItem k_lambda;
	AxiomItem k_self;
	AxiomItem k_funself;
private:
	AxiomItem nilList;
	AxiomItem rootScope;
	AxiomItem globalScope;
	bool ready;

	AxiomItem doCreateKeyword(const char *name, U4 code)
	{
		AxiomItem quoted_ident;
		quoted_ident.setTo(container->alloc(AXIOM_ITEM_ATOM_IDENTIFIER), true, container);
		std::string s = name;
		quoted_identifiers[s].clear();
		((AxiomItemIdentifier *)(&*quoted_ident))->init(&(quoted_identifiers.find(s)->first), code);
		quoted_identifiers[s] = quoted_ident;
		return quoted_ident;
	}

	void createNil()
	{
		nilList.setTo(container->alloc(AXIOM_ITEM_LIST), true, container);
		((AxiomItemList *)(&*nilList))->init(container);
		quoted_identifiers["nil"] = nilList;
		k_void = doCreateKeyword("void", AXIOM_KEYWORD_VOID);
		k_plus = doCreateKeyword("plus", AXIOM_KEYWORD_PLUS);
		k_minus = doCreateKeyword("minus", AXIOM_KEYWORD_MINUS);
		k_system = getUserIdent("system");
		k_lambda = doCreateKeyword("lambda", AXIOM_KEYWORD_LAMBDA);
		k_self = doCreateKeyword("self", AXIOM_KEYWORD_SELF);
		k_funself = doCreateKeyword("funself", AXIOM_KEYWORD_FUNSELF);
	}

public:
	AxiomItem getUserIdent(std::string name, U4 code = AXIOM_KEYWORD__USER, bool createIfRequired = true)
	{
		std::map<std::string, AxiomItem>::iterator i = quoted_identifiers.find(name);
		if(i != quoted_identifiers.end())
			return i->second;
		if(!createIfRequired)
		{
			AxiomItem n;	// NULL target
			return n;
		}
		AxiomItem quoted_ident;
		quoted_ident.setTo(container->alloc(AXIOM_ITEM_ATOM_IDENTIFIER), true, container);
		std::string s = name;
		quoted_identifiers[s].clear();
		((AxiomItemIdentifier *)(&*quoted_ident))->init(&(quoted_identifiers.find(s)->first), code);
		quoted_identifiers[s] = quoted_ident;
		return quoted_ident;
	}
private:
	void createKeywords()
	{
		AxiomItem k_list;
		AxiomItem k_zero;
		AxiomItem k_one;
		AxiomItem k_asgn;

		k_list = doCreateKeyword("list", AXIOM_KEYWORD_LIST);
		k_asgn = doCreateKeyword("asgn", AXIOM_KEYWORD_ASGN);

		AxiomItem zero;
		zero.setTo(container->alloc(AXIOM_ITEM_ATOM_INTEGER), true, container);
		zero->getInteger()->init(0, 0/*BIT size*/, nilList);
		quoted_identifiers["zero"] = zero;
		k_zero = zero;

		AxiomItem one;
		one.setTo(container->alloc(AXIOM_ITEM_ATOM_INTEGER), true, container);
		one->getInteger()->init(1, 0/*BIT size*/, nilList);
		quoted_identifiers["one"] = one;
		k_one = one;

		doCreateKeyword("module", AXIOM_KEYWORD_MODULE);
		doCreateKeyword("namespace", AXIOM_KEYWORD_NAMESPACE);
		doCreateKeyword("fun", AXIOM_KEYWORD_FUN);
		doCreateKeyword("return", AXIOM_KEYWORD_RETURN);
		doCreateKeyword("public", AXIOM_KEYWORD_PUBLIC);
		doCreateKeyword("axiom", AXIOM_KEYWORD_AXIOM);
		doCreateKeyword("using", AXIOM_KEYWORD_USING);
		doCreateKeyword("include", AXIOM_KEYWORD_INCLUDE);
		doCreateKeyword("debug", AXIOM_KEYWORD_DEBUG);
		doCreateKeyword("loadcpu", AXIOM_KEYWORD_LOADCPU);
	}
public:
	AxiomItem cons(AxiomItem first, AxiomItem rest)
	{
		AxiomItem result;
		result.setTo(container->alloc(AXIOM_ITEM_LIST), true, container);
		result->getList()->init(container);
		result->getList()->set(first, rest);
		return result;
	}
	AxiomItem makeScopeAnonymous(AxiomItem parent)
	{
		AxiomItem result;
		result.setTo(container->alloc(AXIOM_ITEM_ATOM_SCOPE), true, container);
		result->getScope()->init(AXIOM_SCOPE_TYPE_GENERAL);
		result->getScope()->setParent(parent);
		return result;
	}

	AxiomItem makeScopeNamed(AxiomItem parent, AxiomItem name, char scopeType)
	{
		AxiomItem result;
		result.setTo(container->alloc(AXIOM_ITEM_ATOM_SCOPE), true, container);
		result->getScope()->init(scopeType);
		result->getScope()->setParent(parent);
		result->getScope()->setName(name);
		return result;
	}

private:
	void init()
	{
		container = new AxiomItemContainer();
		createNil();	// this is implied, is not in state file
		AxiomItem nullParent;
		rootScope = makeScopeAnonymous(nullParent);
		globalScope = makeScopeAnonymous(rootScope);
	}

public:
	AxiomState(FILE *state)
	{
		using namespace std;
		ready = false;
		init();
		U8 type, size;
		U8 filepos;
		filepos = _ftelli64(state);
		// Read first atom 'type' field.
		if(fread(&type, 8, 1, state) != 1)
			return;
		// Read first atom 'size' field.
		if(fread(&size, 8, 1, state) != 1)
			return;
		// Make sure atom type is "axiomsta" (axiom state).
		if(memcmp(&type, "axiomsta", 8) != 0)
			return;
		if(size < 32)
			return;
		size -= 16;
		std::map<U8, AxiomItemBase *> identifierMap;
		std::list<AxiomItem> items;
		bool ok = false;
		while(size != 0)
		{
			U8 type2, size2;
			if(fread(&type2, 8, 1, state) != 1)
				return;
			if(fread(&size2, 8, 1, state) != 1)
				return;
			if(size2 < 16)
				return;
			U8 filepos2 = _ftelli64(state);
			size2 -= 16;

			if(memcmp(&type2, "endstate", 8) == 0)
			{
				// End of state atom. This goes at the end to indicate success.
				ok = true;
			}
			else
			if(memcmp(&type2, "needvers", 8) == 0)
			{
				U4 min_req_ver;
				if(fread(&min_req_ver, 4, 1, state) != 1)
					return;
				if(min_req_ver != 0)		// min req. ver of 0 means 'unknown'
				{
					if(min_req_ver > 0x001000)	// we are version 1.000--if we require a newer version, fail
						return;
				}
				U4 expected_ver;	// not used -- but must be present
				if(fread(&expected_ver, 4, 1, state) != 1)
					return;
			}
			else
			if(memcmp(&type2, "identifs", 8) == 0)
			{
				// quoted identifiers. these must appear before anyone can refer to identifiers.
				char *buf = NULL;
				size_t buf_size = 0;
				while(size2 != 0)
				{
					U8 local_code;
					if(fread(&local_code, 8, 1, state) != 1)
					{
						delete [] buf;
						return;
					}
					U4 ident_code;
					if(fread(&ident_code, 4, 1, state) != 1)
					{
						delete [] buf;
						return;
					}
					U4 length;
					if(fread(&length, 4, 1, state) != 1)
					{
						delete [] buf;
						return;
					}
					if(length == 0)
					{
						delete [] buf;
						return;
					}
					if(length + 1 >= buf_size)
					{
						delete [] buf;
						buf = new char [length + 1];
						buf_size = 1 + length;
					}
					buf[length] = '\0';
					if(fread(buf, length, 1, state) != 1)
					{
						delete [] buf;
						return;
					}
					std::string s = buf;
					// Create identifier.
					AxiomItem item;
					if(s == "nil")
						item = nilList;
					else
					if(s == "void")
						item = k_void;
					else
					if(s == "plus")
						item = k_plus;
					else
					if(s == "minus")
						item = k_minus;
					else
					if(s == "system")
						item = k_system;
					else
					if(s == "lambda")
						item = k_lambda;
					else
					if(s == "self")
						item = k_self;
					else
					if(s == "funself")
						item = k_funself;
					if(s == "<global scope>")
						item = globalScope;
					else
					if(s == "<root scope>")
						item = rootScope;
					else
						item = getUserIdent(s, ident_code);
					identifierMap[local_code] = &*item;
				}
				delete [] buf;
			}
			else
			if(memcmp(&type2, "rootitem", 8) == 0)
			{
				U8 identifier;
				if(fread(&identifier, 8, 1, state) != 1)
					return;
				U8 local_code;
				if(fread(&local_code, 8, 1, state) != 1)
					return;
				AxiomItem target;
				target.setTo(identifierMap[local_code], false, container);
				rootScope->getScope()->add(identifierMap[identifier], target);
			}
			else
			if(memcmp(&type2, "globitem", 8) == 0)
			{
				U8 identifier;
				if(fread(&identifier, 8, 1, state) != 1)
					return;
				U8 local_code;
				if(fread(&local_code, 8, 1, state) != 1)
					return;
				AxiomItem target;
				target.setTo(identifierMap[local_code], false, container);
				globalScope->getScope()->add(identifierMap[identifier], target);
			}
			else
			if(memcmp(&type2, "listitem", 8) == 0)
			{
				U8 flags, local_code;
				if(fread(&flags, 8, 1, state) != 1)
					return;
				if(fread(&local_code, 8, 1, state) != 1)
					return;
				U8 first, rest;
				if(fread(&first, 8, 1, state) != 1)
					return;
				if(fread(&rest, 8, 1, state) != 1)
					return;
				AxiomItem firstT;
				firstT.setTo(identifierMap[first], false, container);
				AxiomItem restT;
				restT.setTo(identifierMap[rest], false, container);
				AxiomItem lst = cons(firstT, restT);
				lst->setQuoted((flags >> 0) & 1);
				lst->setArgFun((flags >> 1) & 1);
				items.push_back(lst);	// keep ref. count from falling to 0
				identifierMap[local_code] = &*items.back();
			}
			else
			if(memcmp(&type2, "scopitem", 8) == 0)
			{
				// scope item.
				U8 flags, local_code;
				if(fread(&flags, 8, 1, state) != 1)
					return;
				if(fread(&local_code, 8, 1, state) != 1)
					return;
				U8 parent, name, scopeType;
				if(fread(&parent, 8, 1, state) != 1)
					return;
				if(fread(&name, 8, 1, state) != 1)
					return;	// only used if scopeType is not AXIOM_SCOPE_TYPE_GENERAL
				if(fread(&scopeType, 8, 1, state) != 1)
					return;
				AxiomItem item;
				AxiomItem parentT;
				parentT.setTo(identifierMap[parent], false, container);
				if(scopeType == AXIOM_SCOPE_TYPE_GENERAL)
				{
					item = makeScopeAnonymous(parentT);
				}
				else
				{
					AxiomItem nameT;
					nameT.setTo(identifierMap[name], false, container);
					item = makeScopeNamed(parentT, nameT, (char)scopeType);
				}

				U8 numOthers;
				if(fread(&numOthers, 8, 1, state) != 1)
					return;
				U8 numSymbols;
				if(fread(&numSymbols, 8, 1, state) != 1)
					return;
				for(U8 i = 0; i < numOthers; ++i)
				{
					U8 other_item;	// these should be in reverse order
					if(fread(&other_item, 8, 1, state) != 1)
						return;
					AxiomItem x;
					x.setTo(identifierMap[other_item], false, container);
					item->getScope()->addOther(x);
				}
				for(U8 i = 0; i < numSymbols; ++i)
				{
					U8 key, value;
					if(fread(&key, 8, 1, state) != 1)
						return;
					if(fread(&value, 8, 1, state) != 1)
						return;
					AxiomItemBase *keyT = identifierMap[key];
					AxiomItem valueT;
					valueT.setTo(identifierMap[value], false, container);
					item->getScope()->add(keyT, valueT);
				}

				item->setQuoted((flags >> 0) & 1);
				item->setArgFun((flags >> 1) & 1);
				items.push_back(item);						// keep ref. count from falling to 0
				identifierMap[local_code] = &*items.back();
			}

			size -= size2;
			_fseeki64(state, filepos2 + size2, SEEK_SET);
		}

		if(ok)
			ready = true;
	}

	AxiomState()
	{
		ready = false;
		init();
		createKeywords();

#if 0
		{
			AxiomItem c;
			{
			AxiomItem alpha = getUserIdent("alpha");
			AxiomItem a = cons(alpha, nilList);
			AxiomItem b = cons(getUserIdent("beta"), a);
			c = cons(getUserIdent("gamma"), b);
			c->write(std::cout);std::cout << std::endl;
			}
		}
		std::cout << "No more reference!" << std::endl;
#endif

		ready = true;
	}
	bool isReady()
	{
		return ready;
	}
	virtual ~AxiomState()
	{
	}
	AxiomItem getNil()
	{
		return nilList;
	}
	AxiomItem getGlobalScope()
	{
		return globalScope;
	}
	AxiomItem getRootScope()
	{
		return rootScope;
	}
};

}	// namespace AxiomLanguage

namespace AxiomLanguage
{
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
size_t numObjectsGlobal = 0;
#endif
}

#endif	// l_crudasm__axiom_lang_h__included
