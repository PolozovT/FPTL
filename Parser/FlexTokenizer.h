﻿#ifndef FLEXTOKENIZER_H
#define FLEXTOKENIZER_H

#include <string>
#include "Generated/parser.tab.hh"

namespace FPTL {
namespace Parser {

	class Support;
	class ConstantNode;

	/*
	 * Tokenizer - обертка над flex.
	 * Выделяет лексему из входной строки.
	 */
	class Tokenizer : yyFlexLexer
	{
	public:
		explicit Tokenizer(std::istream& input);

		BisonParser::token_type  getToken( BisonParser::semantic_type * aVal, Support * aSupport );
		Ident                    getErrorIdent() const;

	private:

		int yylex() override;

		// Вспомогательные методы, вызываемые из yylex.

		ConstantNode * formDecimalConstant() const;
		ConstantNode * formLongLongConstant() const;
		ConstantNode * formFPConstant( bool aForceFloat ) const;
		ConstantNode * formStringConstant() const;

		int	processCommentBlock();
		int	processIdentifier() const;

	private:
		Support * mSupport;
		BisonParser::semantic_type * mVal;

		// Переменные позиционирования внутри исходного файла.
		size_t mLine;
		size_t mCol;
		size_t mPrevTokenLine;
		size_t mTokenBegin;
		size_t mLastTokenLen;
	};

	int yylex( BisonParser::semantic_type * aVal, Support * aSupport, Tokenizer * aTokenizer );

}
}

#endif
