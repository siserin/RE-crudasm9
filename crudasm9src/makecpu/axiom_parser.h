// axiom_parser.h - Copyright (C) 2012 Willow Schlanger. All rights reserved.

#ifndef l_crudasm__axiom_parser_h__included
#define l_crudasm__axiom_parser_h__included

#include "axiom_lang.h"

#include <fstream>
#include <string>
#include <cctype>

namespace AxiomLanguage
{

template <class CharInputIterator>
class AxiomParser
{
	AxiomState *state;
	std::ostream *oserr;
	bool inSystem;

public:
	AxiomParser(AxiomState *stateT)
	{
		state = stateT;
		inSystem = false;
	}

	virtual ~AxiomParser()
	{
	}

	bool hasOpenComment(CharInputIterator b, CharInputIterator e)
	{
		for(;;)
		{
			if(b == e)
				break;
			if(*b != '/')
			{
				++b;
				continue;
			}
			++b;
			if(b == e)
				break;
			if(*b != '*')
			{
				++b;
				continue;
			}
			++b;
			for(;;)
			{
				if(b == e)
					return true;
				if(*b == '*')
				{
					++b;
					if(b == e)
						return true;
					if(*b == '/')
					{
						++b;
						break;
					}
				}
				else
					++b;
			}
		}
		return false;
	}

	bool getInfo(std::ostream &oerr, CharInputIterator b, CharInputIterator e, size_t &lparen, size_t &rparen, size_t &numLines)
	{
		oserr = &oerr;

		lparen = rparen = numLines = 0;
		skipWhitespace(b, e, &numLines);
		if(b == e)
			return false;
		while(b != e)
		{
			skipWhitespace(b, e, &numLines);
			if(b == e)
				break;
			if(*b == '(')
				++lparen;
			else
			if(*b == ')')
				++rparen;
			++b;
		}
		return true;
	}

	AxiomItem parseIncludeFileGlobal(std::string fname, std::ostream &oerr, bool isSystemT)
	{
		std::ifstream fi(fname.c_str());
		if(fi == NULL)
		{
			AxiomItem item;
			*oserr << "#error: " << fname << ": unable to open include file" << std::endl;
			return item;
		}
		std::string text, line;
		while(std::getline(fi, line))
		{
			line += "\n";
			text += line;
		}
		return parse(text.begin(), text.end(), oerr, fname, 1, isSystemT);
	}

	AxiomItem parseIncludeFile(std::string fname, std::ostream &oerr, AxiomItemBase *mod = NULL, AxiomItemBase *fcn = NULL, AxiomItemBase *scope = NULL)
	{
		std::ifstream fi(fname.c_str());
		if(fi == NULL)
		{
			AxiomItem item;
			*oserr << "#error: " << fname << ": unable to open include file" << std::endl;
			return item;
		}
		std::string text, line;
		while(std::getline(fi, line))
		{
			line += "\n";
			text += line;
		}

		return parse(text.begin(), text.end(), oerr, fname, 1, false, mod, fcn, scope);
	}

	AxiomItem parse(CharInputIterator b, CharInputIterator e, std::ostream &oerr, std::string &file, size_t lineNum, bool inSystemT = false,
		AxiomItemBase *mod = NULL, AxiomItemBase *fcn = NULL, AxiomItemBase *scope = NULL)
	{
		oserr = &oerr;
		inSystem = inSystemT;

		if(hasOpenComment(b, e))
		{
				AxiomItem item;
				*oserr << "#error: " << file << " line " << (lineNum) << ": unmatched comment /*" << std::endl;
				return item;
		}
		size_t lparen, rparen, numLines;
		getInfo(oerr, b, e, lparen, rparen, numLines);
		if(lparen != rparen)
		{
				AxiomItem item;
				*oserr << "#error: " << file << " line " << (lineNum + numLines) << ": parenthesis mismatch" << std::endl;
				return item;
		}

		size_t linesOut = 0;
		
		//AxiomItem item = doParse(b, e, file, lineNum, linesOut);
		//AxiomItem doParseInternal(CharInputIterator &b, CharInputIterator e, std::string &file, size_t lineNum, size_t &linesOut,
		//	AxiomItem curModule, AxiomItem curFunction, AxiomItem curScope, bool forceQuote, bool *gotApostropheOpt = NULL)
		//;
		AxiomItem curModule, curFunction;
		AxiomItem curScope = (inSystemT) ? state->getRootScope() : state->getGlobalScope();
		AxiomItem item = doParseInternal(b, e, file, lineNum, linesOut, curModule, curFunction, curScope, false);
		if(mod != NULL)
			curModule.setTo(mod, false, state->getContainer());
		if(fcn != NULL)
			curFunction.setTo(fcn, false, state->getContainer());
		if(scope != NULL)
			curScope.setTo(scope, false, state->getContainer());

		if(!item.isNull())
		{
			if(b != e)
			{
				item.clear();
				*oserr << "#error: " << file << " line " << (lineNum + linesOut) << ": too many items (hint: use a return, axiom, or list object)" << std::endl;
				return item;
			}
		}
		return item;
	}

	AxiomItem parseInteractive(CharInputIterator &b, CharInputIterator e, std::ostream &oerr, std::string &file, size_t &lineNum)
	{
		oserr = &oerr;
		inSystem = false;

		if(hasOpenComment(b, e))
		{
				AxiomItem item;
				*oserr << "#error: " << file << " line " << (lineNum) << ": unmatched comment /*" << std::endl;
				return item;
		}
		size_t lparen, rparen, numLines;
		getInfo(oerr, b, e, lparen, rparen, numLines);
		if(lparen != rparen)
		{
				AxiomItem item;
				*oserr << "#error: " << file << " line " << (lineNum + numLines) << ": parenthesis mismatch" << std::endl;
				return item;
		}

		size_t linesOut = 0;

		AxiomItem item = doParse(b, e, file, lineNum, linesOut);

		if(item.isNull())
			return item;	// we will stop parsing here and thus don't need the line number updated
		skipWhitespace(b, e, &linesOut);	// this way the caller can compare 'b' with 'e' to decide whether or not to invoke us again
		lineNum += linesOut;
		return item;
	}

private:
	void skipWhitespace(CharInputIterator &b, CharInputIterator &e, size_t *num_linesT)
	{
		while(b != e && (isspace(*b) || *b == '#' || *b == '/'))
		{
			if(*b == '/')
			{
				bool handled = false;
				CharInputIterator tmp = b;
				++tmp;
				if(*tmp == '*')
				{
					++tmp;
					for(;;)
					{
						if(tmp == e)
						{
							///*oserr << "#warning: unmatched /* comment" << std::endl;
							b = e;
							return;
						}
						if(*tmp == '\n')
							++*num_linesT;
						if(*tmp == '*')
						{
							++tmp;
							if(tmp != e && *tmp == '/')
							{
								b = tmp;
								handled = true;
								break;
							}
						}
						else
							++tmp;
					}
				}
				if(!handled)
					return;
				if(b == e)
					return;
			}
			if(*b == '#')
			{
				while(b != e && *b != '\n')
					++b;
				if(b == e)
					break;
			}
			if(*b == '\n')
				++*num_linesT;
			++b;
		}
	}

	AxiomItem doParse(CharInputIterator &b, CharInputIterator e, std::string &file, size_t lineNum, size_t &linesOut)
	{
		AxiomItem n;	// null target item ref. for cur module and cur function
		return doParseInternal(b, e, file, lineNum, linesOut, n/*cur. module*/, n/*cur. function*/, state->getGlobalScope(), false);
	}

	AxiomItem doParseInternal(CharInputIterator &b, CharInputIterator e, std::string &file, size_t lineNum, size_t &linesOut,
		AxiomItem curModule, AxiomItem curFunction, AxiomItem curScope, bool forceQuote, bool *gotApostropheOpt = NULL
	)
	{
		AxiomItem result;
		size_t numLines = 0;
		skipWhitespace(b, e, &numLines);
		lineNum += linesOut;	// test

		if(gotApostropheOpt != NULL)
		{
			*gotApostropheOpt = false;
		}

		if(b != e)
		{
			if(*b == '\'')
			{
				forceQuote = true;
				++b;
				if(b != e)
				{
					using namespace std;
					if(isspace(*b) || *b == '/' || *b == '#')
					{
						*oserr << "#error: " << file << " line " << (lineNum + numLines) << ": invalid use of apostrophe to quote nothing" << std::endl;
						result.clear();
						return result;
					}
				}
				if(b == e)
				{
					*oserr << "#error: " << file << " line " << (lineNum + numLines) << ": unexpected end of input after apostrophe" << std::endl;
					result.clear();
					return result;
				}

				if(gotApostropheOpt != NULL)
				{
					// We have to let the caller know if the first item in a list was quoted, so they know
					// to generate an error.
					*gotApostropheOpt = true;
				}
			}
		}

		if(b == e)
			result = state->k_void;
		else
		if(*b == '(')
			result = parseList(numLines, b, e, file, lineNum, numLines, curModule, curFunction, curScope, forceQuote);
		else
		if(*b == ')')
		{
			*oserr << "#error: " << file << " line " << (lineNum + numLines) << ": unexpected ')'" << std::endl;
			result.clear();
			return result;
		}
		else
			result = parseAtom(numLines, b, e, file, lineNum, numLines, curModule, curFunction, curScope, forceQuote);

		skipWhitespace(b, e, &numLines);

		if(!result.isNull())
		{
			if(result->getIdentifier() && result->getIdentifier()->getCode() == AXIOM_KEYWORD_FUNSELF)
			{
				*oserr << "#error: " << file << " line " << (lineNum + numLines) << ": parsed 'funself': recursion is only allowed inside a module with a forward definition" << std::endl;
				result.clear();
				return result;
			}
			linesOut += numLines;
		}
		return result;
	}

	AxiomItem parseList(size_t &numLines, CharInputIterator &b, CharInputIterator e, std::string &file, size_t lineNum, size_t &linesOut,
		AxiomItem curModule, AxiomItem curFunction, AxiomItem curScope, bool forceQuote
	)
	{
		std::ostream &oerr = *oserr;
		AxiomItem result;

		// Get a list.
		++b;	// skip '('
		skipWhitespace(b, e, &linesOut);
		if(b == e)
		{
			oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": parenthesis mismatch" << std::endl;
			result.clear();
			return result;
		}

		if(*b == ')')
		{
			++b;
			return state->getNil();
		}

		AxiomItem first_item;
		bool firstItemUserQuoted = false;
		for(;;)
		{
			first_item = doParseInternal(b, e, file, lineNum, linesOut, curModule, curFunction, curScope, forceQuote, &firstItemUserQuoted);
			if(first_item.isNull())
				return first_item;
			skipWhitespace(b, e, &linesOut);
			if(b == e)
			{
				oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": parenthesis mismatch" << std::endl;
				result.clear();
				return result;
			}
			if(first_item != state->k_void)
				break;
			if(*b == ')')
			{
				++b;
				return state->getNil();
			}
		}

		// We now have the first item, and we've skipped whitespace.
		if(forceQuote)
		{
			result = getQuotedList(first_item, b, e, lineNum, linesOut, file, curModule, curFunction, curScope);
		}
		else
		{
			if(firstItemUserQuoted)
			{
				oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": attempt parse a list whose first item is user quoted via apostrophe (not allowed)" << std::endl;
				result.clear();
				return result;
			}

			bool allowScopeSrc = (curScope == state->getGlobalScope());

			// List is NOT being quoted.
			if(first_item->getIdentifier() != NULL)
			{
				// First item was quoted, now process it as if it were unquoted.
				first_item = getAtom(file, lineNum + numLines, first_item->getIdentifier()->getName(), false, curScope, curModule, curFunction);
				if(first_item.isNull())
				{
					result.clear();
					return result;
				}
			}

			if(first_item->getFunRefInternal() != NULL && curFunction.isNull())
			{
				// It is important to disallow this or else you could, inside a module where forward references are possible, get a function to return
				// a variation of what it returns, i.e. not only would there be an infinite loop, but the data cycle doesn't make sense from a parse
				// tree perspective:
				//
				// (module mod (a) ()
				//   (asgn b (a))
				//   (fun (a) (list b))
				// )
				oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": attempt to invoke forward function from outside a function: ";
				first_item->write(oerr);
				oerr << std::endl;
				result.clear();
				return result;
			}

			if(first_item->getScope() != NULL)
			{
				result = getQuotedList(first_item, b, e, lineNum, linesOut, file, curModule, curFunction, curScope);
				return getScopeRef(result, first_item, lineNum, linesOut, file, curModule, curFunction, curScope, allowScopeSrc);
			}

			bool ok = first_item->isList();
			if(!ok && first_item->getFunDef())
				ok = true;
			if(!ok && first_item->getFunRef())
				ok = true;
			if(!ok && first_item->getFunRefInternal())
				ok = true;
			if(!ok && first_item->getIdentifier() != NULL)
			{
				if(first_item->getIdentifier()->getCode() != AXIOM_KEYWORD__USER)
					ok = true;
			}
			if(!ok)
			{
					oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": first item in list must be a list, keyword, lambda, or scope" << std::endl;
					result.clear();
					return result;
			}

			// Create new scope.
			AxiomItem newScope = state->makeScopeAnonymous(curScope);
			AxiomItemBase *scope = &*newScope;
			size_t quotedCountdown = 0;
			bool quoteEverything = false;

			if(first_item->getIdentifier() != NULL)
			{
				switch(first_item->getIdentifier()->getCode())
				{
				case AXIOM_KEYWORD_ASGN:
					scope = &*curScope;
					quotedCountdown = 2;	// (asgn dest src) -- dest is auto-quoted.
					allowScopeSrc = true;
					break;
				case AXIOM_KEYWORD_LAMBDA:
					quotedCountdown = 2;	// (lambda quoted-args value)
					break;
				case AXIOM_KEYWORD_NAMESPACE:
					quotedCountdown = 2;	// (namespace <name> ...)
					break;
				case AXIOM_KEYWORD_MODULE:
					quotedCountdown = 4;	// (module '<name, or list of symbols to import> '<fwd-defs-list> '<using-list> ...)
					break;
				case AXIOM_KEYWORD_FUN:
					quotedCountdown = 2;
					break;
				case AXIOM_KEYWORD_PUBLIC:
					quotedCountdown = 2;
					break;
				case AXIOM_KEYWORD_AXIOM:
					scope = &*curScope;
					break;
				case AXIOM_KEYWORD_USING:
					scope = &*curScope;
					quotedCountdown = 1;
					quoteEverything = true;
					break;
				case AXIOM_KEYWORD_INCLUDE:
					scope = &*curScope;
					break;
				case AXIOM_KEYWORD_LOADCPU:
					quotedCountdown = 1;
					quoteEverything = true;
					break;
				}
			}

			AxiomItem res;
			size_t itemCount = 0;
			bool isArgFun = false;
			AxiomItem lastArgFunItem;
			AxiomItem functionDef;
			AxiomItem namespaceName;
			AxiomItem moduleName, moduleFwdDefsLst, moduleUsingLst;
			for(;;)
			{
				if(first_item != state->k_void)
				{
					if(first_item->getScope() != NULL && !allowScopeSrc)
					{
						oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: misplaced scope: ";
						first_item->write(oerr);
						oerr << std::endl;
						result.clear();
						return result;
					}

					if(result.isNull())
					{
						result = state->cons(first_item, state->getNil());
						res = result;
					}
					else
					{
						AxiomItem tmp = state->cons(first_item, state->getNil());
						res->getList()->setRest(tmp);
						res = tmp;
					}
					res->setQuoted(false);
					if(first_item->isArgFun())
					{
						lastArgFunItem = res;
						isArgFun = true;
					}
					if(quotedCountdown != 0 && !quoteEverything)
						--quotedCountdown;
					++itemCount;

					bool isNamespace = false;
					bool isLambda = false;
					bool isModule = false;
					bool isFun = false;
					if(result->getList()->getFirst()->getIdentifier() != NULL)
					{
						U4 code = result->getList()->getFirst()->getIdentifier()->getCode();
						switch(code)
						{
						case AXIOM_KEYWORD_LAMBDA:
							isLambda = true;
							break;
						case AXIOM_KEYWORD_NAMESPACE:
							isNamespace = true;
							break;
						case AXIOM_KEYWORD_MODULE:
							isModule = true;
							break;
						case AXIOM_KEYWORD_FUN:
							isFun = true;
							break;
						}
					}
					if(isModule && itemCount == 4)
					{
						AxiomItem tmp = result->getList()->getRest();	// skip 'module'
						moduleName = tmp->getList()->getFirst();
						tmp = tmp->getList()->getRest();
						moduleFwdDefsLst = tmp->getList()->getFirst();
						tmp = tmp->getList()->getRest();
						moduleUsingLst = tmp->getList()->getFirst();
						bool ok = true;
						if(ok)
						{
							if(!moduleName->isList())
							{
								if(moduleName->getIdentifier() == NULL || moduleName->getIdentifier()->getCode() != AXIOM_KEYWORD__USER)
									ok = false;
							}
						}
						if(ok)
						{
							if(!moduleFwdDefsLst->isList())
								ok = false;
						}
						if(ok)
						{
							if(!moduleUsingLst->isList())
								ok = false;
						}
						if(!inSystem && moduleName == state->k_system)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": attempt to create 'system' module (only system can do that)" << std::endl;
							result.clear();
							return result;
						}
						if(!ok)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (module name-or-names fwd-defs using-list ...)" << std::endl;
							result.clear();
							return result;
						}

						// Leave 'result' alone, but update 'scope'.
						scope->getScope()->setScopeType(AXIOM_SCOPE_TYPE_MODULE);
						if(!moduleName->isList())
						{
							scope->getScope()->setName(moduleName);
							scope->getScope()->add(&*moduleName, state->k_self);
						}
						scope->getScope()->setParent(state->getRootScope());	// we can only use things in 'system' unless listed in our uses-list clause

						// Now go through 'moduleUsingLst' and import those symbols.
						tmp = moduleUsingLst;
						while(!tmp->getList()->isNil())
						{
							AxiomItem name = tmp->getList()->getFirst();
							AxiomItem item;
							
							if(name->isList() && !name->getList()->isNil())
							{
								// We were given a list. Attempt to resolve the item.
								AxiomItem firstItemT = name->getList()->getFirst();
								if(firstItemT->getIdentifier() != NULL)
								{
									firstItemT = getAtom(file, lineNum + numLines, firstItemT->getIdentifier()->getName(), false, curScope, curModule, curFunction);
									if(firstItemT.isNull())
									{
										result.clear();
										return result;
									}

									if(firstItemT->getScope() != NULL)
									{
										AxiomItem nameOut;
										item = getScopeRef(name, firstItemT, lineNum, linesOut, file, curModule, curFunction, curScope, true, &nameOut);
										if(item.isNull())
										{
											// We already generated an error message.
											result.clear();
											return result;
										}
										name = nameOut;
									}
								}
							}
							else
							if(name->getIdentifier() != NULL)
							{
								item = curScope->getScope()->search(&*name, true);
							}

							if(item.isNull() || name.isNull())
							{
								oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: in module '";
								if(moduleName->getIdentifier() != NULL)
									moduleName->write(oerr);
								else
									oerr << "anonymous module";
								oerr << "', unable to import symbol: ";
								if(name.isNull())
									oerr << "{unknown symbol name}";
								else
									name->write(oerr);
								oerr << std::endl;
								result.clear();
								return result;
							}

							if(item == state->k_self)
							{
								oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": bad module using-list: refers to 'self'" << std::endl;
								result.clear();
								return result;
							}

							if(item->isArgFun())
							{
								oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: identifier '";
								name->write(oerr);
								oerr << "' used by module '";
								if(moduleName->getIdentifier() != NULL)
									moduleName->write(oerr);
								else
									oerr << "anonymous module";
								oerr << "' is a function of an argument (not allowed)" << std::endl;
								result.clear();
								return result;
							}

							scope->getScope()->add(&*name, item);

							tmp = tmp->getList()->getRest();
						}

						//begin code to create forward definitions
						if(!moduleFwdDefsLst->getList()->isNil())
						{
							std::set<std::string> needed;
							AxiomItem tmp = moduleFwdDefsLst;
							while(!tmp->getList()->isNil())
							{
								AxiomItem item = tmp->getList()->getFirst();

								if(item->getIdentifier() == NULL)
								{
									oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": module's forward reference list must be a list of identifiers" << std::endl;
									result.clear();
									return result;
								}

								AxiomItem funRef;
								funRef.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_FUN_REF_INTERNAL), true, state->getContainer());
								funRef->getFunRefInternal()->init(scope, &*item);
								funRef->getFunRefInternal()->setQuoted(false);
								scope->getScope()->add(&*item, funRef);

								tmp = tmp->getList()->getRest();
							}
						}
						//end

						// Update current module for any additional items.
						curModule.setTo(scope, false, state->getContainer());
						curFunction.clear();	// don't let the module's contents be aware if it's in a function - specifically, disallow arg accesses.
					}
					if(isNamespace && itemCount == 2)
					{
						bool synOk = true;
						namespaceName.clear();
						if(synOk)
						{
							if(result->getList()->getRest()->getList()->isNil())
								synOk = false;
						}
						if(synOk)
						{
							namespaceName = result->getList()->getRest()->getList()->getFirst();
						}
						if(namespaceName.isNull())
							synOk = false;
						else
						if(namespaceName->getIdentifier() == NULL)
							synOk = false;
						else
						{
							if(namespaceName->getIdentifier()->getCode() != AXIOM_KEYWORD__USER)
								synOk = false;
							else
							if(namespaceName == state->k_system)
							{
								if(!inSystem)
								{
									oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": error: attempt to create 'system' namespace (only system can do that)" << std::endl;
									result.clear();
									return result;
								}
							}
						}
						if(!synOk)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (namespace namespace-name ...)" << std::endl;
							result.clear();
							return result;
						}
						scope->getScope()->setScopeType(AXIOM_SCOPE_TYPE_NAMESPACE);
						scope->getScope()->setName(namespaceName);
						scope->getScope()->add(&*namespaceName, state->k_self);		// self is an error to use, but we set it to help catch bugs

						AxiomItem prevNamespace = curScope->getScope()->search(&*namespaceName, false);
						if(!prevNamespace.isNull() && prevNamespace->getScope() != NULL)
						{
							if(prevNamespace->getScope()->getScopeType() != AXIOM_SCOPE_TYPE_NAMESPACE)
							{
								oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: bad re-definition of namepsace: was ";
								prevNamespace->write(oerr);
								oerr << std::endl;
								result.clear();
								return result;
							}
							scope->getScope()->addOther(prevNamespace);
						}
					}
					if(isFun && itemCount == 2)
					{
						bool synOk = true;
						AxiomItem name;
						AxiomItem args;
						AxiomItem body;
						if(synOk)
						{
							if(result->getList()->getRest()->getList()->isNil())
								synOk = false;
						}
						if(synOk)
						{
							args = result->getList()->getRest()->getList()->getFirst();
						}
						if(!args.isNull())
						{
							if(!args->isList())
							{
								args.clear();
								synOk = false;
							}
						}
						if(synOk)
						{
							if(args->getList()->isNil())
								synOk = false;
						}
						if(synOk)
						{
							name = args->getList()->getFirst();
							args = args->getList()->getRest();
							if(name->getIdentifier() == NULL)
								synOk = false;
						}
						if(name.isNull() || args.isNull() || !synOk)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (fun (function-name [args...]) body-item)" << std::endl;
							result.clear();
							return result;
						}

						// Disallow recursion outside a module.
						bool allowRecursion = false;
						if(!curModule.isNull())
						{
							AxiomItem prevDefn = curScope->getScope()->search(&*name, true);
							if(!prevDefn.isNull())
							{
								if(prevDefn->getFunRefInternal() != NULL)
								{
									allowRecursion = true;
								}
							}
						}
						if(!allowRecursion)
						{
							scope->getScope()->add(&*name, state->k_funself);
						}

						// Create an AxiomItemFunDef object.
						functionDef.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_FUN_DEF), true, state->getContainer());
						functionDef->getFunDef()->init(name, args);
						functionDef->getFunDef()->setQuoted(false);

						// Process arguments here.
						size_t argNum = 0;
						for(AxiomItem x = args; !x->getList()->isNil(); x = x->getList()->getRest())
						{
							AxiomItem argName = x->getList()->getFirst();
							bool ok = true;
							if(argName->getIdentifier() == NULL)
								ok = false;
							if(ok && argName->getIdentifier()->getCode() != AXIOM_KEYWORD__USER)
								ok = false;
							if(!inSystem && argName == state->k_system)
								ok = false;
							if(!ok)
							{
								oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": bad argument number " << (1 + argNum) << std::endl;
								result.clear();
								return result;
							}

							AxiomItem arg;
							arg.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_FUN_ARG), true, state->getContainer());
							arg->getFunArg()->init(&*functionDef, argNum);
							arg->setArgFun(true);
							arg->setQuoted(false);
							scope->getScope()->add(&*argName, arg);
							++argNum;
						}

						curFunction.setTo(&*functionDef, false, state->getContainer());
					}
					if(isLambda && itemCount == 2)
					{
						bool synOk = true;
						AxiomItem args;
						AxiomItem body;
						if(synOk)
						{
							if(result->getList()->getRest()->getList()->isNil())
								synOk = false;
						}
						if(synOk)
						{
							args = result->getList()->getRest()->getList()->getFirst();
						}
						if(!args.isNull())
						{
							if(!args->isList())
							{
								args.clear();
								synOk = false;
							}
						}
						if(args.isNull() || !synOk)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (lambda args-list value-item)" << std::endl;
							result.clear();
							return result;
						}

						// Create an AxiomItemFunDef object.
						functionDef.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_FUN_DEF), true, state->getContainer());
						AxiomItem noName;		// lambda's don't have names
						functionDef->getFunDef()->init(noName, args);
						functionDef->getFunDef()->setQuoted(false);

						// Process arguments here.
						size_t argNum = 0;
						for(AxiomItem x = args; !x->getList()->isNil(); x = x->getList()->getRest())
						{
							AxiomItem argName = x->getList()->getFirst();
							bool ok = true;
							if(argName->getIdentifier() == NULL)
								ok = false;
							if(ok && argName->getIdentifier()->getCode() != AXIOM_KEYWORD__USER)
								ok = false;
							if(!inSystem && argName == state->k_system)
								ok = false;
							if(!ok)
							{
								oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": bad argument number " << (1 + argNum) << std::endl;
								result.clear();
								return result;
							}

							AxiomItem arg;
							arg.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_FUN_ARG), true, state->getContainer());
							arg->getFunArg()->init(&*functionDef, argNum);
							arg->setArgFun(true);
							arg->setQuoted(false);
							scope->getScope()->add(&*argName, arg);
							++argNum;
						}

						curFunction.setTo(&*functionDef, false, state->getContainer());
					}
				}

				if(*b == ')')
				{
					++b;
					break;
				}
				bool quote = (quotedCountdown != 0);
				AxiomItem scopeItem;
				scopeItem.setTo(scope, false, state->getContainer());
				first_item = doParseInternal(b, e, file, lineNum, linesOut, curModule, curFunction, scopeItem, quote);
				if(first_item.isNull())
					return first_item;
				skipWhitespace(b, e, &linesOut);
				if(b == e)
				{
					oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": parenthesis mismatch" << std::endl;
					result.clear();
					return result;
				}
			}

			// Mark items to the left of any item that is a function of an argument,
			// so we know it too is a function of argument(s).
			if(isArgFun && !lastArgFunItem.isNull())
			{
				AxiomItem x = result;
				while(!x->getList()->isNil())
				{
					x->getList()->setArgFun(true);
					if(x == lastArgFunItem)
						break;
					x = x->getList()->getRest();
				}
			}

			first_item = result->getList()->getFirst();
			if(first_item->getIdentifier() != NULL)
			{
				switch(first_item->getIdentifier()->getCode())
				{
				case AXIOM_KEYWORD_DEBUG:
					{
						result = result->getList()->getRest();
						AxiomItem srcFile = result;
						bool ok = false;
						std::string fname;
						if(!srcFile->getList()->isNil())
						{
							srcFile = srcFile->getList()->getFirst();
							if(srcFile->getString() != NULL)
							{
								fname = srcFile->getString()->getText();
								result = result->getList()->getRest();
								if(result->getList()->isNil())
									ok = true;
							}
						}

						if(!ok)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (debug \"text\")" << std::endl;
							result.clear();
							return result;
						}

						if(fname.empty())
							oerr << "\n";
						else
							oerr << "#note: " << fname << std::endl;
						
						result = state->k_void;
						break;
					}
				case AXIOM_KEYWORD_LOADCPU:
					{
						result = result->getList()->getRest();
						AxiomItem srcList = result;
						bool ok = false;

						if(!srcList->getList()->isNil())
						{
							srcList = srcList->getList()->getFirst();
							if(srcList->getList() != NULL)
							{
								result = result->getList()->getRest();
								if(result->getList()->isNil())
									ok = true;
							}
						}

						if(!ok)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (loadcpu <cpu-declaration-list>)" << std::endl;
							result.clear();
							return result;
						}

						result.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_CPU_ARCH), true, state->getContainer());
						if(!result->getCpuArch()->init(oerr, srcList))
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": error parsing loadcpu declaration list" << std::endl;
							result.clear();
							return result;
						}

						break;
					}
				case AXIOM_KEYWORD_INCLUDE:
					{
						result = result->getList()->getRest();
						AxiomItem srcFile = result;
						bool ok = false;
						std::string fname;
						if(!srcFile->getList()->isNil())
						{
							srcFile = srcFile->getList()->getFirst();
							if(srcFile->getString() != NULL)
							{
								fname = srcFile->getString()->getText();
								result = result->getList()->getRest();
								if(result->getList()->isNil() && !fname.empty())
									ok = true;
							}
						}

						if(!ok)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (include \"filename\")" << std::endl;
							result.clear();
							return result;
						}

						AxiomItem tmp = parseIncludeFile(fname, oerr, &*curModule, &*curFunction, scope);
						if(tmp.isNull())
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": error with include file: " << fname << std::endl;
							result.clear();
							return result;
						}

						result = state->k_void;
						break;
					}
				case AXIOM_KEYWORD_USING:
					{
						AxiomItem name = result->getList()->getRest();	// get sub-list that follows the 'using' keyword
						AxiomItem item;

						if(name->getList()->isNil())
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (using scope [names...])" << std::endl;
							result.clear();
							return result;
						}

						// We were given a list. Attempt to resolve the item.
						AxiomItem firstItemT = name->getList()->getFirst();
						if(firstItemT->getIdentifier() != NULL)
						{
							firstItemT = getAtom(file, lineNum + numLines, firstItemT->getIdentifier()->getName(), false, curScope, curModule, curFunction);
							if(firstItemT.isNull())
							{
								result.clear();
								return result;
							}

							if(firstItemT->getScope() != NULL)
							{
								AxiomItem nameOut;
								item = getScopeRef(name, firstItemT, lineNum, linesOut, file, curModule, curFunction, curScope, true, &nameOut);
								if(item.isNull())
								{
									// We already generated an error message.
									result.clear();
									return result;
								}
								name = nameOut;
							}
							else
							{
								oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: 'using' statement expects first argument to be a scope" << std::endl;
								result.clear();
								return result;
							}
						}

						if(item.isNull() || name.isNull() || name->getIdentifier() == NULL || scope == &*state->k_self)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (using scope [names...])" << std::endl;
							result.clear();
							return result;
						}

						if(item->getScope() == NULL)
						{
								curScope->getScope()->add(&*name, item);
						}
						else
						{
							if(item->getScope()->getScopeType() == AXIOM_SCOPE_TYPE_NAMESPACE)
								curScope->getScope()->receiveSymbolsFromScope(item);
							else
							{
								// Only allow (using <scope>) to work for namespace scopes.
								oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: attempt to apply 'using' to non-namespace scope" << std::endl;
								result.clear();
								return result;
							}
						}

						result = state->k_void;
						break;
					}
				case AXIOM_KEYWORD_RETURN:
					{
						result = result->getList()->getRest();	// skip 'return' keyword
						if(result->getList()->isNil())
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: 'return' block missing a return value" << std::endl;
							result.clear();
							return result;
						}
						if(!result->getList()->getRest()->getList()->isNil())
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: 'return' block attempted to generate output" << std::endl;
							result.clear();
							return result;
						}
						result = result->getList()->getFirst();
						break;
					}
				case AXIOM_KEYWORD_AXIOM:
					{
						result = result->getList()->getRest();	// skip 'axiom' keyword
						if(!result->getList()->isNil())
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: 'axiom' block attempted to generate output" << std::endl;
							result.clear();
							return result;
						}
						result = state->k_void;
						break;
					}
				case AXIOM_KEYWORD_PUBLIC:
					{
						result = result->getList()->getRest();	// skip 'public'
						AxiomItem publicLst;
						if(!result->getList()->isNil())
						{
							publicLst = result->getList()->getFirst();
							result = result->getList()->getRest();	// skip public list
							if(publicLst.isNull() || !result->getList()->isNil())
							{
								oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: 'public' block attempted to generate output" << std::endl;
								result.clear();
								return result;
							}
						}
						if(publicLst.isNull() || !publicLst->isList())
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected: (public public-list [items...])" << std::endl;
							result.clear();
							return result;
						}
						if(!publicLst->getList()->isNil())
						{
							// Go thru items in publicLst and introduce them to our 'curScope'.
							while(!publicLst->getList()->isNil())
							{
								AxiomItem name = publicLst->getList()->getFirst();
								if(name->getIdentifier() == NULL)
								{
									oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: 'public' block's public-list must be a list of identifiers" << std::endl;
									result.clear();
									return result;
								}

								AxiomItem item = scope->getScope()->search(&*name, false);
								if(item.isNull())
								{
									oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": public symbol '";
									name->write(oerr);
									oerr << "' not declared within 'public' block scope" << std::endl;
									result.clear();
									return result;
								}

								curScope->getScope()->add(&*name, item);

								publicLst = publicLst->getList()->getRest();
							}
						}
						result = state->k_void;
						break;
					}
				case AXIOM_KEYWORD_MODULE:
					{
						bool fail = false;
						if(!moduleName.isNull() && moduleName->getIdentifier() == NULL)
						{
							if(!moduleName->isList())
								fail = true;
						}
						if(itemCount < 4 || fail)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (module name-or-names fwd-defs using-list ...)" << std::endl;
							result.clear();
							return result;
						}
						if(itemCount > 4)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": module attempted to generate output: ";
							if(!moduleName.isNull() && moduleName->getIdentifier() != NULL)
								moduleName->write(oerr);
							else
								oerr << "{anonymous module}";
							oerr << std::endl;
							result.clear();
							return result;
						}

						// Create 'module scope'. Make sure all forward definitions are now defined as FUN_DEF's.
						// Convert them to FUN_REF's and put them in modScope.
						AxiomItem modScope = state->makeScopeAnonymous(state->getRootScope());
						modScope->getScope()->setScopeType(AXIOM_SCOPE_TYPE_MODULE);
						if(!moduleName->isList())
						{
							modScope->getScope()->setName(moduleName);
							modScope->getScope()->add(&*moduleName, state->k_self);
						}

						std::map<AxiomItemBase *, AxiomItem> &syms = scope->getScope()->getDirectSymbols();
						for(std::map<AxiomItemBase *, AxiomItem>::iterator i = syms.begin(); i != syms.end(); ++i)
						{
							AxiomItem dest = i->second;
							if(dest->getFunDef() != NULL)
							{
								// Got a function definition symbol.
								AxiomItem funRef;
								funRef.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_FUN_REF), true, state->getContainer());
								AxiomItem destScope;
								destScope.setTo(scope, false, state->getContainer());
								funRef->getFunRef()->init(destScope, i->first);
								funRef->getFunRef()->setQuoted(false);
								modScope->getScope()->add(i->first, funRef);
							}
						}
						if(!moduleFwdDefsLst->getList()->isNil())
						{
							std::set<std::string> needed;
							AxiomItem tmp = moduleFwdDefsLst;
							while(!tmp->getList()->isNil())
							{
								AxiomItem item = tmp->getList()->getFirst();

								if(item->getIdentifier() == NULL)
								{
									oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": module's forward reference list must be a list of identifiers" << std::endl;
									result.clear();
									return result;
								}

								AxiomItem node = modScope->getScope()->search(&*item, false);
								if(node.isNull() || node->getFunRef() == NULL)
								{
									oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": forward reference symbol not defined as a function: ";
									item->write(oerr);
									oerr << std::endl;
									result.clear();
									return result;
								}

								tmp = tmp->getList()->getRest();
							}
						}

						//modScope.setTo(scope, false, state->getContainer());

						if(moduleName->getIdentifier() != NULL)
						{
							curScope->getScope()->add(&*moduleName, modScope);
						}
						else
						{
							AxiomItem cur = moduleName;
							while(!cur->getList()->isNil())
							{
								AxiomItem item = cur->getList()->getFirst();
								if(item->getIdentifier() == NULL || item->getIdentifier()->getCode() != AXIOM_KEYWORD__USER)
								{
									oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": module's public symbol name must be an identifier or a list of identifiers" << std::endl;
									result.clear();
									return result;
								}
								AxiomItem dest = modScope->getScope()->search(&*item, false);
								if(dest.isNull() || dest->isArgFun())
								{
									oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": module's public symbol '";
									item->write(oerr);
									oerr << "' is undefined or invalid." << std::endl;
									result.clear();
									return result;
								}
								cur = cur->getList()->getRest();
							}
							cur = moduleName;
							while(!cur->getList()->isNil())
							{
								AxiomItem item = cur->getList()->getFirst();
								AxiomItem dest = modScope->getScope()->search(&*item, false);
								curScope->getScope()->add(&*item, dest);
								cur = cur->getList()->getRest();
							}
						}
						result = state->k_void;
						break;
					}
				case AXIOM_KEYWORD_NAMESPACE:
					{
						if(namespaceName.isNull())
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (namespace namespace-name ...)" << std::endl;
							result.clear();
							return result;
						}
						if(itemCount != 2)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": namespace attempted to generate output: ";
							namespaceName->write(oerr);
							oerr << std::endl;
							result.clear();
							return result;
						}
						AxiomItem s;
						s.setTo(scope, false, state->getContainer());
						curScope->getScope()->add(&*namespaceName, s);
						result = state->k_void;
						break;
					}
				case AXIOM_KEYWORD_FUN:
					{
						bool synOk = true;
						if(itemCount != 3)
							synOk = false;
						AxiomItem argsAndName;
						AxiomItem body;
						if(synOk)
						{
							if(result->getList()->getRest()->getList()->isNil())
								synOk = false;
						}
						if(synOk)
						{
							argsAndName = result->getList()->getRest()->getList()->getFirst();
							if(result->getList()->getRest()->getList()->getRest()->getList()->isNil())
								synOk = false;
						}
						if(synOk)
						{
							body = result->getList()->getRest()->getList()->getRest()->getList()->getFirst();
							if(!result->getList()->getRest()->getList()->getRest()->getList()->getRest()->getList()->isNil())
								synOk = false;
						}
						if(!argsAndName.isNull())
						{
							if(!argsAndName->isList() || argsAndName->getList()->isNil())
							{
								argsAndName.clear();
								synOk = false;
							}
						}
						if(argsAndName.isNull() || body.isNull() || !synOk || functionDef.isNull())
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (fun (function-name [args...]) body-item)" << std::endl;
							result.clear();
							return result;
						}

						functionDef->getFunDef()->setBody(body);

						AxiomItem src = functionDef;
						AxiomItem dest = argsAndName->getList()->getFirst();

						// Now simulate asgn.
						if(dest->getIdentifier() == NULL || dest->getIdentifier()->getCode() != AXIOM_KEYWORD__USER)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: bad function name" << std::endl;
							result.clear();
							return result;
						}
						if(src == state->k_self)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: bad function body 'self'" << std::endl;
							result.clear();
							return result;
						}
						if(!inSystem && dest == state->k_system)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: attempt to create function named 'system' (only system can do that)" << std::endl;
							result.clear();
							return result;
						}

						scope->getScope()->getParent()->getScope()->add(&*(dest->getIdentifier()), src);

						result = state->k_void;
						break;
					}
				case AXIOM_KEYWORD_LAMBDA:
					{
						bool synOk = true;
						if(itemCount != 3)
							synOk = false;
						AxiomItem args;
						AxiomItem body;
						if(synOk)
						{
							if(result->getList()->getRest()->getList()->isNil())
								synOk = false;
						}
						if(synOk)
						{
							args = result->getList()->getRest()->getList()->getFirst();
							if(result->getList()->getRest()->getList()->getRest()->getList()->isNil())
								synOk = false;
						}
						if(synOk)
						{
							body = result->getList()->getRest()->getList()->getRest()->getList()->getFirst();
							if(!result->getList()->getRest()->getList()->getRest()->getList()->getRest()->getList()->isNil())
								synOk = false;
						}
						if(!args.isNull())
						{
							if(!args->isList())
							{
								args.clear();
								synOk = false;
							}
						}
						if(args.isNull() || body.isNull() || !synOk || functionDef.isNull())
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (lambda args-list value-item)" << std::endl;
							result.clear();
							return result;
						}

						// Create an AxiomItemFunDef object.
						result = functionDef;
						result->getFunDef()->setBody(body);
						break;
					}
				case AXIOM_KEYWORD_ASGN:
					{
						if(itemCount != 3)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: expected (asgn dest src)" << std::endl;
							result.clear();
							return result;
						}
						AxiomItem dest = result->getList()->getRest()->getList()->getFirst();
						AxiomItem src = result->getList()->getRest()->getList()->getRest()->getList()->getFirst();
						if(dest->getIdentifier() == NULL || dest->getIdentifier()->getCode() != AXIOM_KEYWORD__USER)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: bad asgn destination" << std::endl;
							result.clear();
							return result;
						}
						if(src == state->k_self)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: bad asgn source (contains 'self')" << std::endl;
							result.clear();
							return result;
						}
						if(!inSystem && dest == state->k_system)
						{
							oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": syntax error: attempt to asgn to 'system' (only system can do that)" << std::endl;
							result.clear();
							return result;
						}

						scope->getScope()->add(&*(dest->getIdentifier()), src);

						result = state->k_void;
						break;
					}
				}
			}
		}

		return result;
	}

	AxiomItem getScopeRef(AxiomItem result, AxiomItem first_item, size_t lineNum, size_t linesOut, std::string &file, AxiomItem curModule,
		AxiomItem curFunction, AxiomItem curScope, bool allowScopeSrc, AxiomItem *nameOut = NULL
	)
	{
		std::ostream &oerr = *oserr;
		AxiomItem prev = result;
		AxiomItem s = first_item;

		if(nameOut != NULL)
		{
			nameOut->clear();
			if(prev->isList() && !prev->getList()->isNil())
				*nameOut = prev->getList()->getFirst();
		}

		prev = prev->getList()->getRest();

		while(!prev->getList()->isNil())
		{
			AxiomItem cur = prev->getList()->getFirst();
			if(cur->getIdentifier() == NULL)
				break;
			if(nameOut != NULL)
			{
				*nameOut = cur;
			}

			AxiomItem tmp = s->getScope()->search(&*cur, false);

			if(tmp.isNull())
			{
				oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": can't find symbol: ";
				cur->write(oerr);
				oerr << std::endl;
				result.clear();
				return result;
			}

			if(!tmp->getScope())
			{
				if(tmp == state->k_self)
				{
					oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": invalid scope reference: contains self" << std::endl;
					result.clear();
					return result;
				}
				if(!prev->getList()->getRest()->getList()->isNil())
				{
					oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": invalid scope reference: too many items" << std::endl;
					result.clear();
					return result;
				}
				return tmp;
			}
			s = tmp;

			prev = prev->getList()->getRest();
		}

		if(allowScopeSrc && !s.isNull() && s->getScope() != NULL)
		{
			return s;
		}

		oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": invalid scope reference" << std::endl;
		result.clear();
		return result;
	}

	AxiomItem getQuotedList(AxiomItem first_item, CharInputIterator &b, CharInputIterator e, size_t lineNum, size_t &linesOut, std::string &file, AxiomItem curModule, AxiomItem curFunction, AxiomItem curScope)
	{
		bool forceQuote = true;
		std::ostream &oerr = *oserr;

		AxiomItem result;
		AxiomItem res;
		for(;;)
		{
			if(first_item != state->k_void)
			{
				if(result.isNull())
				{
					result = state->cons(first_item, state->getNil());
					res = result;
				}
				else
				{
					AxiomItem tmp = state->cons(first_item, state->getNil());
					res->getList()->setRest(tmp);
					res = tmp;
				}
			}

			if(*b == ')')
			{
				++b;
				break;
			}
			first_item = doParseInternal(b, e, file, lineNum, linesOut, curModule, curFunction, curScope, forceQuote);
			if(first_item.isNull())
				return first_item;
			skipWhitespace(b, e, &linesOut);
			if(b == e)
			{
				oerr << "#error: " << file << " line " << (lineNum + linesOut) << ": parenthesis mismatch" << std::endl;
				result.clear();
				return result;
			}
		}
		return result;
	}

	// Get an atom. Note that if you quote nil, you get the empty list still because that quoted reserved word has special meaning.
	AxiomItem parseAtom(size_t &numLines, CharInputIterator &b, CharInputIterator e, std::string &file, size_t lineNum, size_t &linesOut,
		AxiomItem curModule, AxiomItem curFunction, AxiomItem curScope, bool forceQuote
	)
	{
		using namespace std;
		AxiomItem result;

		std::string atom_str;
		while(b != e && !isspace(*b) && *b != '(' && *b != ')' && *b != '#' && *b != '"')
		{
			if(*b == '/')
			{
				CharInputIterator tmp = b;
				++tmp;
				if(tmp != e)
				{
					if(*tmp == '*')
					{
						//std::cout << "=>" << std::string(b, e) << std::endl;
						break;
					}
				}
			}
			atom_str += *b;
			++b;
		}

		if(b != e && *b == '"')
		{
			++b;	// skip '"'
			std::string str;
			while(b != e && *b != '"')
			{
				if(*b == '\n')
					break;	// not valid in string literals
				if(*b == '\\')
				{
					// note. if you include a path on a Windows-like system, use forward slashes.
					++b;	// skip '\\'.
					if(b == e || *b == '\n')
					{
						*oserr << "#error: " << file << " line " << (lineNum + linesOut) << ": invalid string" << std::endl;
						result.clear();
						return result;
					}
						
					if(*b == 'n')
					{
						str += "\n";
						++b;
						continue;
					}
						
					if(*b == 'r')
					{
						str += "\r";
						++b;
						continue;
					}
						
					if(*b == '\\')
					{
						str += "\\";
						++b;
						continue;
					}
						
					if(*b == 't')
					{
						str += "\t";
						++b;
						continue;
					}
						
					// Otherwise just quote the unrecognized character.
				}
				str += *b;
				++b;
			}
			if(b == e || *b != '"')
			{
				*oserr << "#error: " << file << " line " << (lineNum + numLines) << ": invalid string" << std::endl;
				result.clear();
				return result;
			}
			++b;	// skip '"'
			
			result.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_STRING), true, state->getContainer());
			result->getString()->init(str);
		}
		else
			result = getAtom(file, lineNum + numLines, atom_str, forceQuote, curScope, curModule, curFunction);

		return result;
	}

	AxiomItem makeInteger(U8 value)
	{
		AxiomItem result;
		result.setTo(state->getContainer()->alloc(AXIOM_ITEM_ATOM_INTEGER), true, state->getContainer());
		result->getInteger()->init(value, 8, state->getNil());
		return result;
	}

	AxiomItem getAtom(std::string file, size_t line, std::string atom_str, bool quoted, AxiomItem curScope,
		AxiomItem curModule, AxiomItem curFunction
	)
	{
		AxiomItem result;
		std::string input_str = atom_str;
		std::ostream &oerr = *oserr;

		if(atom_str.empty())
			return result;	// sanity check
		
		if(atom_str == "-")
			return state->k_minus;
		if(atom_str == "+")
			return state->k_plus;
		using namespace std;
		if(atom_str.size() >= 2 && atom_str[0] == '+' && isdigit(atom_str[1]))
		{
			atom_str = std::string(++atom_str.begin(), atom_str.end());
		}
		if(isdigit(atom_str[0]) || (atom_str.size() > 1 && atom_str[0] == '-' && isdigit(atom_str[1])))
		{
			if(isdigit(atom_str[0]) && atom_str.size() == 1)
			{
				result = makeInteger(atom_str[0] - '0');
			}
			else
			{
				bool hexNeg = false;
				if(atom_str.size() >= 3 && atom_str[0] == '-' && atom_str[1] == '0' &&
					(atom_str[2] == 'x' || atom_str[2] == 'X')
				)
				{
					hexNeg = true;
					atom_str = std::string(++atom_str.begin(), atom_str.end());
				}
				// begin hex
				if(atom_str[1] == 'x' || atom_str[1] == 'X')
				{
					U8 value = 0;
					if(atom_str.size() < 3)
					{
						oerr << "#error: " << file << " line " << line << ": invalid hex number: " << input_str << std::endl;
						result.clear();
						return result;
					}
					for(size_t idx = 2; idx < atom_str.size(); ++idx)
					{
						U8 prev = value;
						if(atom_str[idx] >= '0' && atom_str[idx] <= '9')
							value = (value * 16) + (U8)(atom_str[idx] - '0');
						else
						if(atom_str[idx] >= 'a' && atom_str[idx] <= 'f')
							value = (value * 16) + (U8)(atom_str[idx] - 'a' + 0xa);
						else
						if(atom_str[idx] >= 'A' && atom_str[idx] <= 'F')
							value = (value * 16) + (U8)(atom_str[idx] - 'A' + 0xa);
						else
						{
							oerr << "#error: " << file << " line " << line << ": invalid hex number: " << input_str << std::endl;
							result.clear();
							return result;
						}
						if(value < prev)
						{
							oerr << "#error: " << file << " line " << line << ": invalid hex number (does not fit in 64 bits): " << input_str << std::endl;
							result.clear();
							return result;
						}
					}

					if(hexNeg)
					{
						U8 prevValue = value;
						value = (U8)(-(S8)(value));
						if(value != 0 && value == prevValue)
						{
							oerr << "#error: " << file << " line " << line << ": invalid hex number (does not fit in 64 bits): " << input_str << std::endl;
							result.clear();
							return result;
						}
					}

					result = makeInteger(value);
				}
				// end hex
				else
				{
					size_t idx = 0;
					bool negate = false;
					if(atom_str[0] == '-')
					{
						negate = true;
						idx = 1;
					}
					bool valid = false;
					U8 value = 0;
					for(; idx < atom_str.size(); ++idx)
					{
						if(atom_str[idx] >= '0' && atom_str[idx] <= '9')
						{
							U8 prev = value;
							value = (value * 10) + (U8)(atom_str[idx] - '0');
							if(value >= prev)
								valid = true;
							else
							{
								valid = false;
								break;
							}
						}
						else
						{
							valid = false;
							break;
						}
					}

					if(valid && negate)
					{
						U8 prevValue = value;
						value = (U8)(-(S8)(value));
						if(value == prevValue && value != 0)
							valid = false;
					}

					if(!valid)
					{
						oerr << "#error: " << file << " line " << line << ": invalid 64-bit number: " << input_str << std::endl;
						result.clear();
						return result;
					}

					result = makeInteger(value);
				}
			}

			return result;
		}

		if(quoted)
			return state->getUserIdent(atom_str, AXIOM_KEYWORD__USER, true);

		result = state->getUserIdent(atom_str, AXIOM_KEYWORD__USER, false);
		AxiomItem srcAtom = result;

		if(!result.isNull() && result->getIdentifier() != NULL)
		{
			if(result->getIdentifier()->getCode() == AXIOM_KEYWORD__USER)
			{
				AxiomItem symbol = result;
				result = curScope->getScope()->search(&*symbol, true);
			}
		}

		if(!result.isNull() && result->isQuoted())
		{
			return result;
		}

		if(result.isNull())
		{
			oerr << "#error: " << file << " line " << line << ": unknown identifier: ";
			if(srcAtom.isNull())
				oerr << atom_str;
			else
				srcAtom->write(oerr);
			oerr << std::endl;
			result.clear();
			return result;
		}

		if(!srcAtom.isNull() && srcAtom->getIdentifier() != NULL)
		{
			result = checkAtom(file, line, result, curFunction, srcAtom);
			// if result's target is NULL here, we've already printed an error message.
		}

		return result;
	}

	// This is used by getAtom() to make sure the given atom, if it's a function argument, refers to the current function and not an
	// outer-level nested function's argument(s).
	AxiomItem checkAtom(std::string &file, size_t line, AxiomItem result, AxiomItem curFunction, AxiomItem srcAtom, bool argsOk = true)
	{
		std::ostream &oerr = *oserr;

		if(result.isNull())
		{
			oerr << "#error: " << file << " line " << line << ": unknown identifier: ";
			srcAtom->write(std::cout);
			oerr << std::endl;
			result.clear();
			return result;
		}

		if(result->getFunArg() != NULL)
		{
			if(!argsOk || curFunction.isNull())
			{
				oerr << "#error: " << file << " line " << line << ": function argument not allowed here: ";
				srcAtom->write(std::cout);
				oerr << std::endl;
				result.clear();
				return result;
			}

			AxiomItemBase *owner = result->getFunArg()->getOwner();
			if(owner != &*curFunction)
			{
				oerr << "#error: " << file << " line " << line << ": function argument refers to an outer function: ";
				srcAtom->write(std::cout);
				oerr << std::endl;
				result.clear();
				return result;
			}
		}

		return result;
	}
};

}	// namespace AxiomLanguage

#endif	// l_crudasm__axiom_parser_h__included
