// test.cpp - Copyright (C) 2012 Willow Schlanger. All rights reserved.
// ---
// Test programs to try:
//     ((lambda (a b) (list a b)) 'alpha 'beta)
//     ((lambda (x w) (list ((lambda (y) x) x ))) 'cool 'dude)  # generates error message

#include "axiom_lang.h"
#include "axiom_parser.h"
#include "axiom_eval.h"

#include <fstream>
#include <string>
#include <iostream>
using namespace AxiomLanguage;

void doPause()
{
	std::cout << "Press Enter to continue: " << std::flush;
	std::string s;
	std::getline(std::cin, s);
}

int main()
{
	try
	{
		AxiomState *aState = new AxiomState();
		AxiomParser<std::string::iterator> parser(aState);
		AxiomEvaluator eval(aState, std::cerr);
#if 0
		{
			AxiomItem cpu = parser.parseIncludeFile("C:/src/crudasm9/from-linux/crudasm9/src/misc/plans/axiom5/x86iset.ax", std::cerr);
			if(cpu.isNull())
			{
				doPause();
				return 1;
			}
			;
		}
#elif 0	// test array code
		{
			AxiomItem myArray0;
			myArray0.setTo(aState->getContainer()->alloc(AXIOM_ITEM_ATOM_ARRAY), true, aState->getContainer());
			myArray0->getArray()->init(17, aState->getNil());

			AxiomItem myArray1 = myArray0->getArray()->set(0, aState->getUserIdent("my-item-at-0"), aState);
			AxiomItem myArray2 = myArray1->getArray()->set(8, aState->getUserIdent("my-item-at-8"), aState);
			AxiomItem myArray3 = myArray2->getArray()->set(9, aState->getUserIdent("my-item-at-9"), aState);
			AxiomItem myArray4 = myArray3->getArray()->set(9, aState->getUserIdent("my-item-at-9x"), aState);

			myArray4->getArray()->debugWrite(std::cout);
			std::cout << std::endl;

			myArray3->getArray()->debugWrite(std::cout);
			std::cout << std::endl;

			myArray2->getArray()->debugWrite(std::cout);
			std::cout << std::endl;

			myArray1->getArray()->debugWrite(std::cout);
			std::cout << std::endl;
		}
#elif 1
		std::string file = "<standard input>";
		size_t lineNum = 1;
		std::ostream &oerr = std::cerr;

		std::string firstCmd =
"(asgn inner (lambda (x) (list 'inner x)))\n\
(asgn outer (lambda (y) (list y 'outer)))\n\
(asgn both (lambda (a b) (list (asgn q (inner a)) q q (outer b) q q)))\n\
(module bar (a b) () (fun (get-cool) 'cool) (asgn a (lambda (v) (list v (b)))) (asgn b (lambda () (get-cool))))\n\
(module foo2 (a) () (asgn a (lambda (x) (list 'here x))) )\n"
"((bar a) 'very)\n"
"(both 1 2)\n((foo2 a) 'there)\n"
;

		//firstCmd = "(include \"C:/src/crudasm9/from-linux/crudasm9/src/misc/plans/axiom5/x86iset.ax\")\n";

		//Following doesn't repeatedly evaluate q, because it leaves a forwarding
		//address so-to-speak.
		//firstCmd = "(list (asgn q (list 'a 'b 'c)) q q q)\n";

		for(;;)
		{
			std::string cmd;
			size_t lineCount = 0;
			size_t lparen, rparen;
			bool ok = true;
			bool needPrompt = true;
			do
			{
				if(needPrompt)
					std::cout << lineNum << ">" << std::flush;
				else
					std::cout << "  " << std::flush;
				needPrompt = false;
				std::string ln;
				if(firstCmd.empty())
					std::getline(std::cin, ln);
				else
				{
					ln = firstCmd;
					firstCmd.clear();
					std::cout << ln << std::flush;
				}
				if(!ln.empty() && ln[0] == '/' && (ln.size() <= 1 || ln[1] != '*'))
				{
					cmd = ln;
					break;
				}
				cmd += ln;
				cmd += "\n";
				if(parser.hasOpenComment(cmd.begin(), cmd.end()))
				{
					lparen = 1;
					rparen = 0;
					continue;
				}
				ok = parser.getInfo(std::cerr, cmd.begin(), cmd.end(), lparen, rparen, lineCount);
			}	while(lparen > rparen);
			if(!ok)
				continue;	// user just pressed Enter or had a # comment or something
			
			if(!cmd.empty() && cmd[0] == '/' && (cmd.size() <= 1 || cmd[1] != '*'))
			{
				if(cmd == "/exit")
					break;
				if(cmd == "/names")
				{
					// This will not write 'system' symbols, as they are not in global scope but are a level higher (in root scope).
					aState->getGlobalScope()->write(std::cout);
					std::cout << std::endl;
					continue;
				}
				oerr << "#error: " << file << " line " << lineNum << ": unknown command: " << cmd << std::endl;
				continue;
			}

			std::string::iterator b = cmd.begin();
			size_t lineTmp = lineNum;
			bool fail = false;
			do
			{
				size_t startLine = lineTmp;
				AxiomItem item = parser.parseInteractive(b, cmd.end(), std::cerr, file, lineTmp);
				if(item.isNull())
				{
					fail = true;
					break;
				}

				if(item->getIdentifier() == NULL || item->getIdentifier()->getCode() != AXIOM_KEYWORD_VOID)
				{
					std::cout << "Question: " << std::flush;
					item->write(std::cout);
					std::cout << std::endl;

					// evaluate here!
					AxiomItem item2 = eval.evaluate(file, startLine, item);

					// print result.
					if(!item2.isNull())
					{
						std::cout << "Answer: " << std::flush;	// tmp
						item2->write(std::cout);
						std::cout << "\n";						// tmp
						std::cout << std::endl;
					}
				}
			}	while(b != cmd.end());

			//std::cout << "Ok" << std::endl;
			if(!fail)
				lineNum += lineCount;
		}
#endif

		delete aState;
	}
	catch(...)
	{
	}
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
	std::cout << "\n" << numObjectsGlobal << " objects active." << std::endl;
#endif
	doPause();
	return 0;
}
