#include <cstdlib>
#include <iostream>
#include <regex>
#include <fstream>
#include <iterator>

#include "FScheme.h"
#include "StandartLibrary.h"
#include "Parser/BuildInFunctionNames.h"
#include "String.h"

namespace FPTL {
namespace Runtime {

namespace {

void id(SExecutionContext & aCtx)
{
	// �������� ������ ��������� �� ������ ������.
	auto numArgs = aCtx.stack.size() - aCtx.argPos - aCtx.arity;

	for (int i = 0; i < numArgs; ++i)
	{
		aCtx.push(aCtx.getArg(i));
	}
}

void add(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(lhs.getOps()->combine(rhs.getOps())->add(lhs, rhs));
}

void sub(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(lhs.getOps()->combine(rhs.getOps())->sub(lhs, rhs));
}

void mul(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(lhs.getOps()->combine(rhs.getOps())->mul(lhs, rhs));
}

void div(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(lhs.getOps()->combine(rhs.getOps())->div(lhs, rhs));
}

void mod(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(lhs.getOps()->combine(rhs.getOps())->mod(lhs, rhs));
}

void equals(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(lhs.getOps()->combine(rhs.getOps())->equal(lhs, rhs));
}

void less(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(lhs.getOps()->combine(rhs.getOps())->less(lhs, rhs));
}

void greater(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(lhs.getOps()->combine(rhs.getOps())->greater(lhs, rhs));
}

void notEqual(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(
		DataBuilders::createBoolean(
			!lhs.getOps()->combine(rhs.getOps())->equal(lhs, rhs).mIntVal
		)
	);
}

void lessOrEqual(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(
		DataBuilders::createBoolean(
			!lhs.getOps()->combine(rhs.getOps())->greater(lhs, rhs).mIntVal
		)
	);
}

void greaterOrEqual(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	aCtx.push(
		DataBuilders::createBoolean(
			!lhs.getOps()->combine(rhs.getOps())->less(lhs, rhs).mIntVal
		)
	);
}

// ���������� ��������� ������������ ����� � ��������� �� 0 �� 1.
void rand(SExecutionContext & aCtx)
{
	aCtx.push(DataBuilders::createDouble((double)std::rand() / RAND_MAX));
}

void sqrt(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(DataBuilders::createDouble(std::sqrt(arg.getOps()->toDouble(arg))));
}

void sin(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(DataBuilders::createDouble(std::sin(arg.getOps()->toDouble(arg))));
}

void cos(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(DataBuilders::createDouble(std::cos(arg.getOps()->toDouble(arg))));
}

void tan(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(DataBuilders::createDouble(std::tan(arg.getOps()->toDouble(arg))));
}

void asin(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(DataBuilders::createDouble(std::asin(arg.getOps()->toDouble(arg))));
}

void atan(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(DataBuilders::createDouble(std::atan(arg.getOps()->toDouble(arg))));
}

void round(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(DataBuilders::createDouble(std::floor(arg.getOps()->toDouble(arg) + 0.5)));
}

void log(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(DataBuilders::createDouble(std::log(arg.getOps()->toDouble(arg))));
}

void exp(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(DataBuilders::createDouble(std::exp(arg.getOps()->toDouble(arg))));
}

void loadPi(SExecutionContext & aCtx)
{
	aCtx.push(DataBuilders::createDouble(3.141592653589793));
}

void loadE(SExecutionContext & aCtx)
{
	aCtx.push(DataBuilders::createDouble(2.718281828459045));
}

void abs(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	aCtx.push(arg.getOps()->abs(arg));
}

void print(SExecutionContext & aCtx)
{
	auto numArgs = aCtx.stack.size() - aCtx.argPos - aCtx.arity;

	for (int i = 0; i < numArgs; ++i)
	{
		auto & arg = aCtx.getArg(i);
		arg.getOps()->print(arg, std::cout);
		if (i + 1 < numArgs)
		{
			std::cout << "*";
		}
	}
}

void printType(SExecutionContext & aCtx)
{
	auto numArgs = aCtx.stack.size() - aCtx.argPos - aCtx.arity;

	for (int i = 0; i < numArgs; ++i)
	{
		auto & arg = aCtx.getArg(i);

		std::cout << arg.getOps()->getType(arg)->TypeName;

		if (i + 1 < numArgs)
		{
			std::cout << "*";
		}
	}
}

// �������������� � ������.
void toString(SExecutionContext & aCtx)
{
	std::stringstream sstream;
	auto & arg = aCtx.getArg(0);

	arg.getOps()->print(arg, sstream);

	aCtx.push(StringBuilder::create(aCtx, sstream.str()));
}

// �������������� � ������������ �����.
void toInteger(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);

	aCtx.push(DataBuilders::createInt(arg.getOps()->toInt(arg)));
}

// �������������� � ����� �����.
void toDouble(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);

	aCtx.push(DataBuilders::createDouble(arg.getOps()->toDouble(arg)));
}

// ������������ �����.
void concat(SExecutionContext & aCtx)
{
	auto & lhs = aCtx.getArg(0);
	auto & rhs = aCtx.getArg(1);

	auto lhsStr = lhs.getOps()->toString(lhs);
	auto rhsStr = rhs.getOps()->toString(rhs);

	int length = lhsStr->length() + rhsStr->length();

	auto val = StringBuilder::create(aCtx, length);
	char * str = val.mString->getChars();

	std::memcpy(str, lhsStr->getChars(), lhsStr->length());
	std::memcpy(str + lhsStr->length(), rhsStr->getChars(), rhsStr->length());

	aCtx.push(val);
}

// ����� ������.
void length(SExecutionContext & aCtx)
{
	auto & arg = aCtx.getArg(0);
	auto str = arg.getOps()->toString(arg);

	aCtx.push(DataBuilders::createInt(str->length()));
}

// ����� �� ����������� ���������.
void search(SExecutionContext & aCtx)
{
	auto & arg0 = aCtx.getArg(0);
	auto & arg1 = aCtx.getArg(1);

	auto src = arg0.getOps()->toString(arg0);
	auto regEx = arg1.getOps()->toString(arg1);

	std::regex rx(regEx->str());
	std::cmatch match;

	if (std::regex_search((const char *)src->getChars(), (const char *)(src->getChars() + src->length()), match, rx))
	{		
		for (unsigned i = 0; i < rx.mark_count(); i++)
		{
			auto & m = match[i+1];

			auto val = StringBuilder::create(aCtx, src, m.first - src->getChars(), m.second - src->getChars());
			aCtx.push(val);
		}
	}
	else
	{
		aCtx.push(DataBuilders::createUndefinedValue());
	}
}

// �������� ����������� �� ����������� ���������.
void match(SExecutionContext & aCtx)
{
	auto & arg0 = aCtx.getArg(0);
	auto & arg1 = aCtx.getArg(1);

	auto src = arg0.getOps()->toString(arg0);
	auto regEx = arg1.getOps()->toString(arg1);

	std::regex rx(regEx->str());
	std::cmatch match;

	if (std::regex_match((const char *)src->getChars(), (const char *)(src->getChars() + src->length()), match, rx))
	{
		for (unsigned i = 0; i < rx.mark_count(); i++)
		{
			auto & m = match[i+1];
			auto val = StringBuilder::create(aCtx, src, m.first - src->getChars(), m.second - src->getChars());
			aCtx.push(val);
		}
	}
	else
	{
		aCtx.push(DataBuilders::createUndefinedValue());
	}
}

// ������ �� ����������� ���������.
void replace(SExecutionContext & aCtx)
{
	auto & arg0 = aCtx.getArg(0);
	auto & arg1 = aCtx.getArg(1);
	auto & arg2 = aCtx.getArg(2);


	auto src     = arg0.getOps()->toString(arg0);
	auto pattern = arg1.getOps()->toString(arg1);
	auto format  = arg2.getOps()->toString(arg2);

	std::regex rx(pattern->str());

	std::string result = std::regex_replace(src->str(), rx, format->str());

	aCtx.push(StringBuilder::create(aCtx, result));
}

// ��������� ������� � ������ ������.
void getToken(SExecutionContext & aCtx)
{
	auto & arg0 = aCtx.getArg(0);
	auto & arg1 = aCtx.getArg(1);

	auto src = arg0.getOps()->toString(arg0);
	auto pattern = arg1.getOps()->toString(arg1);

	std::regex rx("^(?:\\s*)(" + pattern->str() + ")");

	std::cmatch match;

	if (std::regex_search((const char *)src->getChars(), (const char *)(src->getChars() + src->length()), match, rx))
	{
		auto prefix = StringBuilder::create(aCtx, src, match[1].first - src->getChars(), match[1].second - src->getChars());
		aCtx.push(prefix);

		auto suffix = StringBuilder::create(aCtx, src, match.suffix().first - src->getChars(), match.suffix().second - src->getChars());
		aCtx.push(suffix);
	}
	else
	{
		aCtx.push(DataBuilders::createUndefinedValue());
	}
}

// ������ ����������� �����.
void readFile(SExecutionContext & aCtx)
{
	// ��������� ��� �����.
	auto & arg = aCtx.getArg(0);

	auto fileName = arg.getOps()->toString(arg);

	std::ifstream input;
	input.exceptions(std::ios::failbit | std::ios::badbit);
	input.open(fileName->str(), std::ios::binary);

	// ��������� ������ �����.
	auto begin = input.tellg();
	input.seekg(0, std::ios::end);
	auto size = input.tellg() - begin;
	input.seekg(0, std::ios::beg);

	// ����������� ������ ��� ����.
	auto val = StringBuilder::create(aCtx, size);

	// ������ ������.
	std::copy(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>(), val.mString->getChars());

	aCtx.push(val);
}

} // anonymous namespace

StandartLibrary::StandartLibrary() : FunctionLibrary("StdLib")
{
	addFunction(FPTL::Parser::BuildInFunctions::Id, &id);

	// �������������� ��������.
	addFunction(FPTL::Parser::BuildInFunctions::Add, &add);
	addFunction(FPTL::Parser::BuildInFunctions::Subtract, &sub);
	addFunction(FPTL::Parser::BuildInFunctions::Multiply, &mul);
	addFunction(FPTL::Parser::BuildInFunctions::Divide, &div);
	addFunction(FPTL::Parser::BuildInFunctions::Modulus, &mod);
	addFunction(FPTL::Parser::BuildInFunctions::Equal, &equals);
	addFunction(FPTL::Parser::BuildInFunctions::Less, &less);
	addFunction(FPTL::Parser::BuildInFunctions::Greater, &greater);
	addFunction(FPTL::Parser::BuildInFunctions::GreaterOrEqual, &greaterOrEqual);
	addFunction(FPTL::Parser::BuildInFunctions::LessOrEqual, &lessOrEqual);
	addFunction(FPTL::Parser::BuildInFunctions::NotEqual, &notEqual);
	addFunction(FPTL::Parser::BuildInFunctions::Abs, &abs);

	// ��������������� �������.
	addFunction(FPTL::Parser::BuildInFunctions::Rand, &rand);
	addFunction(FPTL::Parser::BuildInFunctions::Sqrt, &sqrt);
	addFunction(FPTL::Parser::BuildInFunctions::Sin, &sin);
	addFunction(FPTL::Parser::BuildInFunctions::Cos, &cos);
	addFunction(FPTL::Parser::BuildInFunctions::Tan, &tan);
	addFunction(FPTL::Parser::BuildInFunctions::Asin, &asin);
	addFunction(FPTL::Parser::BuildInFunctions::Atan, &atan);
	addFunction(FPTL::Parser::BuildInFunctions::Exp, &exp);
	addFunction(FPTL::Parser::BuildInFunctions::Ln, &log);
	addFunction(FPTL::Parser::BuildInFunctions::Round, &round);
	addFunction(FPTL::Parser::BuildInFunctions::Pi, &loadPi);
	addFunction(FPTL::Parser::BuildInFunctions::E, &loadE);

	addFunction(FPTL::Parser::BuildInFunctions::Print, &print);
	addFunction(FPTL::Parser::BuildInFunctions::PrintType, &printType);

	// �������������� �����.
	addFunction(FPTL::Parser::BuildInFunctions::toString, &toString);
	addFunction(FPTL::Parser::BuildInFunctions::toInt, &toInteger);
	addFunction(FPTL::Parser::BuildInFunctions::toReal, &toDouble);

	// ������ �� ��������.
	addFunction(FPTL::Parser::BuildInFunctions::Cat, &concat);
	addFunction(FPTL::Parser::BuildInFunctions::Length, &length);
	addFunction(FPTL::Parser::BuildInFunctions::Search, &search);
	addFunction(FPTL::Parser::BuildInFunctions::readFile, &readFile);
	addFunction(FPTL::Parser::BuildInFunctions::Match, &match);
	addFunction(FPTL::Parser::BuildInFunctions::Replace, &replace);
	addFunction(FPTL::Parser::BuildInFunctions::GetToken, &getToken);

}

}} // FPTL::Runtime