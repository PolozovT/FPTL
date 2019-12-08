﻿#include <memory>
#include <string>

#include "Generated/parser.tab.hh"

#include "Support.h"
#include "SemanticCheck.h"
#include "Nodes.h"
#include "Libraries/StandardLibrary.h"
#include "Tokenizer.h"
#include "Evaluator/Run.h"

namespace FPTL {
	namespace Parser {

		//-------------------------------------------------------------------------------------------
		ErrorMessage::ErrorMessage(const ErrTypes::ErrType aErr, const Ident aIdent)
			: mErr(aErr), mIdent(aIdent)
		{}

		//-------------------------------------------------------------------------------------------
		Support::Support() : mWasError(false)
		{
		}

		//-------------------------------------------------------------------------------------------
		Support::~Support() = default;

		//-------------------------------------------------------------------------------------------
		void Support::semanticError(const ErrTypes::ErrType aErr, const Ident aIdent)
		{
			mErrorList.emplace_back(aErr, aIdent);
			mWasError = true;
		}

		//
		// InitializeIdentTable - регистрируем основные лексемы: ключевые слова, имена типов и т.п.
		//
		void Support::initializeKeywordTable(void)
		{
			// Типы данных.
			registerKeyword("int", BisonParser::token::T_TINT);
			registerKeyword("uint", BisonParser::token::T_TUINT);
			registerKeyword("float", BisonParser::token::T_TDOUBLE); // float не реализован.
			registerKeyword("double", BisonParser::token::T_TDOUBLE);
			registerKeyword("string", BisonParser::token::T_TSTRING);
			registerKeyword("boolean", BisonParser::token::T_TBOOL);

			// Ключевые слова.
			registerKeyword("Functional program", BisonParser::token::T_FUNCTIONAL_PROGRAM);
			registerKeyword("Functional Program", BisonParser::token::T_FUNCTIONAL_PROGRAM);
			registerKeyword("Data", BisonParser::token::T_DATA);
			registerKeyword("Constructors", BisonParser::token::T_CONSTRUCTORS);
			registerKeyword("Scheme", BisonParser::token::T_SCHEME);
			registerKeyword("Fun", BisonParser::token::T_FUN);
			registerKeyword("Application", BisonParser::token::T_APPLICATION);
			registerKeyword("@", 0);
			registerKeyword("true", BisonParser::token::T_TRUE);
			registerKeyword("false", BisonParser::token::T_FALSE);

			for (auto fName : Runtime::StandardLibrary::mFunctions)
			{
				registerKeyword(fName.first, BisonParser::token::BFNAME);
			}
		}

		//-------------------------------------------------------------------------------------------
		void Support::registerKeyword(const std::string & aName, const int aId)
		{
			mNameTable[aName] = aId;
		}

		//
		// lookForIdent - поиск идентификатора в таблице имен.
		//
		int Support::lookForIdent(const std::string & aName, Ident & aIdent)
		{
			const auto pos = mNameTable.find(aName);
			if (pos != mNameTable.end())
			{
				aIdent.Ptr = &pos->first;
				return mNameTable[aName];
			}
			return 0;
		}

		//
		// newIdent - добавление идентификатора в таблицу имен.
		//
		void Support::newIdent(const std::string & aName, const int aId, Ident & aIdent)
		{
			mNameTable[aName] = aId;
			aIdent.Ptr = &mNameTable.find(aName)->first;
		}

		//-------------------------------------------------------------------------------------------
		Ident Support::newConstant(const std::string & aConstant, const size_t aLine, const size_t aCol)
		{
			Ident ident{};
			ident.Col = aCol;
			ident.Line = aLine;
			ident.Ptr = &*mConstantTable.insert(aConstant).first;
			return ident;
		}

		//-------------------------------------------------------------------------------------------
		const char * Support::getErrorString(const ErrTypes::ErrType aErr)
		{
			const char * msg = nullptr;
			switch (aErr)
			{
			case ErrTypes::GeneralSyntaxError:             
				msg = ""; 
				break;
			case ErrTypes::EOFInComment:                   
				msg = "Error: end of file was reached in comment block";
				break;
			case ErrTypes::IntConstOverflow:               
				msg = "Error: integer constant overflow";
				break;
			case ErrTypes::IllegalCharacter:               
				msg = "Error: illegal character";
				break;
			case ErrTypes::MissingMainTypeDef:             
				msg = "Error: missing main type definition";
				break;
			case ErrTypes::DuplicateDefinition:            
				msg = "Error: identifier was defined before";
				break;
			case ErrTypes::UndefinedIdentifier:            
				msg = "Error: undefined identifier";
				break;
			case ErrTypes::InvalidNumberOfParameters:      
				msg = "Error: invalid number of parameters"; 
				break;
			case ErrTypes::UndefinedSchemeName:            
				msg = "Error: undefined scheme name";
				break;
			case ErrTypes::IncorrectIdentifierUsage:       
				msg = "Error: incorrect use of identifier";
				break;
			case ErrTypes::NotATemplateType:               
				msg = "Error: not a template type";
				break;
			case ErrTypes::InvalidTemplateArgumentsNumber: 
				msg = "Error: invalid type parameters number";
				break;
			case ErrTypes::InvalidConstructorUsage:        
				msg = "Error: invalid constructor usage";
				break;
			case ErrTypes::NestedDataDefinition:           
				msg = "Error: nested data definitions are not allowed";
				break;
			case ErrTypes::MultipleTypeExpression:         
				msg = "Error: only one type expression is allowed";
				break;
			case ErrTypes::InvalidFunCallParameters: 
				msg = "Error: function parameters cannot be used as another function parameters"; 
				break;
			case ErrTypes::InvalidConstant:                
				msg = "Error: constant is invalid or out of range"; 
				break;
			case ErrTypes::MissingMainDefinition:          
				msg = "Error: missing main definition in function"; 
				break;
			case ErrTypes::InvalidTupleIndex:              
				msg = "Error: invalid tuple element index"; 
				break;
			default:                                       
				msg = "Unknown error";
			}
			return msg;
		}

		//-------------------------------------------------------------------------------------------
		void Support::getErrorList(std::ostream & aOutStream)
		{
			std::vector<ErrorMessage> processed;
			for (auto& errMsg : mErrorList)
			{
				if (std::find(processed.begin(), processed.end(), errMsg) == processed.end()) {

					aOutStream << getErrorString(errMsg.mErr)
						<< *errMsg.mIdent.Ptr
						<< ". Line " << errMsg.mIdent.Line
						<< ", ch " << errMsg.mIdent.Col << "\n";
					processed.push_back(errMsg);
				}
			}
		}

		//-------------------------------------------------------------------------------------------
		void Support::pushIdent(const Ident & aIdent)
		{
			mIdentStack.push_back(aIdent);
		}

		//-------------------------------------------------------------------------------------------
		void Support::popIdent()
		{
			mIdentStack.pop_back();
		}

		//-------------------------------------------------------------------------------------------
		Ident Support::getTopIdent() const
		{
			return mIdentStack.back();
		}

		//-------------------------------------------------------------------------------------------
		ASTNode* Support::getInternalForm(const std::vector<std::string>& inputTuple, const std::string& programStr)
		{
			std::stringstream program;

			// Сбрасываем состояние.
			mIdentStack.clear();
			mNameTable.clear();
			mConstantTable.clear();

			initializeKeywordTable();

			// Плохая временная реализация получения начального кортежа из параметров запуска.
			// Изменится с реализацией именованных параметров.
			if (!inputTuple.empty())
			{
				std::string inputTupleStr;

				inputTupleStr += "(" + inputTuple[0];
				for (size_t i = 1; i < inputTuple.size(); ++i)
				{
					inputTupleStr += ", " + inputTuple[i];
				}
				inputTupleStr += ")";

				const size_t inPos = programStr.rfind('%');
				if (inPos != static_cast<size_t>(-1))
				{
					std::string end = programStr.substr(inPos);
					size_t fPos, lPos = end.rfind(')');
					if (lPos != static_cast<size_t>(-1))
					{
						fPos = end.find('(');
						++lPos;
					}
					else
					{
						fPos = end.find_first_of("\n\r");
						if (fPos == static_cast<size_t>(-1)) fPos = end.length();
						lPos = fPos;
					}
					// Если блок Application не отсутствует в принципе.
					if (fPos != static_cast<size_t>(-1))
					{
						program << programStr.substr(0, inPos);
						program << end.substr(0, fPos);
						program << inputTupleStr;
						program << end.substr(lPos, end.length());
					}
				}
			}//*/
			else program << programStr;

			Tokenizer tokenizer(program);
			ASTNode *root = nullptr;
			BisonParser parser(this, &tokenizer, root);

			//parser.set_debug_stream(std::cerr);
			//parser.set_debug_level(1);

			const int result = parser.parse();

			std::unique_ptr<ASTNode> rootPtr(root);

			if (result)
			{
				return nullptr;
			}

			NamesChecker checkNames(this, root);

			if (mWasError)
			{
				return nullptr;
			}

			RecursionFinder recursionFinder(root);

			return rootPtr.release();
		}
	}
}
