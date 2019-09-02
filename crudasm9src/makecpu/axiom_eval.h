// axiom_eval.h - Copyright (C) 2012 Willow Schlanger. All rights reserved.

#ifndef l_crudasm__axiom_eval_h__included
#define l_crudasm__axiom_eval_h__included

#include "axiom_lang.h"
#include "axiom_parser.h"

namespace AxiomLanguage
{

class AxiomEvaluator
{
	AxiomState *state;
	std::ostream &oerr;
public:
	AxiomEvaluator(AxiomState *stateT, std::ostream &oerrT) :
		oerr(oerrT)
	{
		state = stateT;
	}
	virtual ~AxiomEvaluator()
	{
	}
	AxiomItem evaluate(std::string &file, size_t line, AxiomItem item)
	{
		AxiomItem result = item;
		/*if(!result.isNull() && result->isQuoted())
		{
			std::cout << "Pre-Quoted: "; result->write(std::cout); std::cout<<std::endl;
		}*/
		while(!result.isNull() && !result->isQuoted() && result->isList() /*&& result->getFunDef() == NULL*/)
		{
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
			std::cout << "Evaluating: "; result->write(std::cout); std::cout<<std::endl;
#endif

			// 'result' is an unquoted list, so there is something for us to do.
			if(result->getList()->isNil())
				break;
			AxiomItem dest;

			AxiomItem firstT = result->getList()->getFirst();
			AxiomItem first = (firstT->isQuoted()) ? firstT : evaluate(file, line, firstT);
			if(first.isNull())
				return first;

			AxiomItem funDef;
			if(first->getFunDef() != NULL)
			{
				funDef.setTo(first->getFunDef(), false, state->getContainer());
			}
			else
			if(first->getFunRef() != NULL)
			{
				AxiomItemBase *target = first->getFunRef()->getTarget();
				if(target == NULL)
				{
					oerr << "#run-time error: " << file << " line " << line << ": attempt to evaluate undefined function: ";
					AxiomItemBase *name = first->getFunRef()->getName();
					if(name == NULL || name->getIdentifier() == NULL)
						oerr << "{unknown function name}";
					else
						name->write(oerr);
					oerr << std::endl;
				}
				funDef.setTo(target, false, state->getContainer());
			}
			else
			if(first->getFunRefInternal() != NULL)
			{
				AxiomItemBase *target = first->getFunRefInternal()->getTarget();
				if(target == NULL)
				{
					oerr << "#run-time error: " << file << " line " << line << ": attempt to evaluate undefined function: ";
					AxiomItemBase *name = first->getFunRefInternal()->getName();
					if(name == NULL || name->getIdentifier() == NULL)
						oerr << "{unknown function name}";
					else
						name->write(oerr);
					oerr << std::endl;
				}
				funDef.setTo(target, false, state->getContainer());
			}

			bool evalAllItems = false;
			bool makeQuoted = false;

			if(!funDef.isNull())
			{
				evalAllItems = true;
			}

			if(!evalAllItems && first->getIdentifier() != NULL)
			{
				switch(first->getIdentifier()->getCode())
				{
				case AXIOM_KEYWORD_LIST:
					evalAllItems = true;
					makeQuoted = true;
					break;
				default:
					dest.clear();
				}
			}

			size_t numItems = 0;
			if(evalAllItems)
			{
				AxiomItem tmp = result->getList()->getRest();
				AxiomItem prev;
				dest.clear();
				while(!tmp->getList()->isNil())
				{
					AxiomItem cur = evaluate(file, line, tmp->getList()->getFirst());
					if(cur.isNull())
						return cur;
					if(dest.isNull())
					{
						dest = state->cons(cur, state->getNil());
						prev = dest;
					}
					else
					{
						prev->getList()->setRest(state->cons(cur, state->getNil()));
						prev = prev->getList()->getRest();
					}
					++numItems;
					tmp = tmp->getList()->getRest();
				}
				if(dest.isNull())
					dest = state->getNil();
			}

			if(!funDef.isNull())
			{
				AxiomItem alst = dest;
				dest.clear();

				if(numItems != funDef->getFunDef()->getNumArgs())
				{
					oerr << "#run-time error: " << file << " line " << line << ": wrong number of arguments (got " << numItems << " but wanted " << funDef->getFunDef()->getNumArgs() << " for function: ";
					funDef->write(oerr);
					oerr << std::endl;
					result.clear();
					return result;
				}

				if(numItems == 0)
				{
					dest = funDef->getFunDef()->getBody();
					if(dest.isNull())
					{
						oerr << "#run-time error: " << file << " line " << line << ": unable to evaluate function: ";
						funDef->getFunDef()->write(oerr);
						oerr << std::endl;
						result.clear();
						return result;
					}
				}
				else
				{
					AxiomItem *args = new AxiomItem [numItems];
					AxiomItem tmp = alst;
					size_t n = 0;
					while(!tmp->getList()->isNil())
					{
						args[n++] = tmp->getList()->getFirst();
						tmp = tmp->getList()->getRest();
					}

					dest = substArgs(funDef->getFunDef()->getBody(), args, numItems);
					delete [] args;
					if(dest.isNull())
					{
						oerr << "#run-time error: " << file << " line " << line << ": unable to evaluate function: ";
						funDef->getFunDef()->write(oerr);
						oerr << std::endl;
						result.clear();
						return result;
					}
				}

				if(!dest.isNull() && !dest->isQuoted())
				{
					if(dest->isList())
					{
						// Got a non-quoted list; repeat the evaluation but avoid reentrency!
						dest->addRef();
						result->setForwarder(&*dest);
						result = dest;
						continue;
					}
					// 'dest' is not a list.
					dest->addRef();
					result->setForwarder(&*dest);
					break;
				}
			}

			// If we reach here, we've got a simple answer to the question that we can substitute then break out of
			// the for loop.
			if(dest.isNull() || !dest->isQuoted())
			{
				oerr << "#run-time error: " << file << " line " << line << ": unable to evaluate input" << std::endl;
				result.clear();
				return result;
			}
			dest->addRef();
			result->setForwarder(&*dest);
			break;
		}
		if(result.isNull())
			return result;
		if(!result->isList() && !result->isArgFun() /*&& result->getFunDef() != NULL*/)
			result->setQuoted(true);	// prevent repeat evaluation for atoms (but not for arg fun atoms)
		return result;
	}
private:
	// Substitute in argument values -- used when a function is called.
	// Don't reenter or visit nodes that are not a function of an argument, just copy them.
	AxiomItem substArgs(AxiomItem src, AxiomItem *args, size_t numArgs)
	{
		std::map<AxiomItemBase *, AxiomItem> tab;
		return doSubstArgs(src, args, numArgs, tab);
	}

	AxiomItem doSubstArgs(AxiomItem src, AxiomItem *args, size_t numArgs, std::map<AxiomItemBase *, AxiomItem> &tab)
	{
		if(!src->isArgFun() || src->isQuoted())
			return src;
		if(!src->isList())
		{
			if(src->getFunArg() == NULL)
			{
				src.clear();
				return src;
			}
			size_t x = src->getFunArg()->getArgNum();
			if(x >= numArgs)
			{
				src.clear();
				return src;
			}
			///tab[&*src] = args[x];	// no pt. here
			return args[x];
		}

		std::map<AxiomItemBase *, AxiomItem>::iterator iter = tab.find(&*src);
		if(iter != tab.end())
		{
			//std::cout << "Cache hit: "; src->write(std::cout); std::cout << std::endl;
			return iter->second;
		}
		//std::cout << "Cache miss: "; src->write(std::cout); std::cout << std::endl;

		AxiomItem tmp = src;
		AxiomItem result;
		AxiomItem prev;
		for(;;)
		{
			if(tmp->getList()->isNil())
				break;

			AxiomItem value = tmp->getList()->getFirst();
			value = doSubstArgs(value, args, numArgs, tab);
			if(value.isNull())
				return value;
			if(result.isNull())
			{
				prev = state->cons(value, state->getNil());
				result = prev;
				prev->setQuoted(tmp->isQuoted());
			}
			else
			{
				prev->getList()->setRest(state->cons(value, state->getNil()));
				prev = prev->getList()->getRest();
				prev->setQuoted(tmp->isQuoted());
			}

			tmp = tmp->getList()->getRest();

			if(!tmp->getList()->isNil() && !tmp->isArgFun())
			{
				prev->getList()->setRest(tmp);
				break;
			}
		}

		tab[&*src] = result;
		return result;
	}
};

}	// namespace AxiomLanguage

#endif	// l_crudasm__axiom_eval_h__included
