#include "StringLib.h"

#include <regex>
#include <string>

#include "Macros.h"
#include "DataTypes/Ops/Ops.h"
#include "DataTypes/Ops/StringOps.h"

namespace FPTL
{
	namespace Runtime
	{
		namespace {

			// ����� ������.
			void length(SExecutionContext & aCtx)
			{
				const auto& arg = aCtx.getArg(0);

				BaseOps::opsCheck(StringOps::get(), arg);

				aCtx.push(DataBuilders::createInt(static_cast<int64_t>(arg.mString->length())));
			}

			// ����� �� ����������� ���������.
			void search(SExecutionContext & aCtx)
			{
				const auto & arg0 = aCtx.getArg(0);
				const auto & arg1 = aCtx.getArg(1);

				BaseOps::opsCheck(StringOps::get(), arg0);
				BaseOps::opsCheck(StringOps::get(), arg1);

				const auto* const src = arg0.mString;
				const auto* const regEx = arg1.mString;

				std::regex rx(regEx->str());
				std::cmatch matchResults;

				if (std::regex_search(static_cast<const char *>(src->getChars()), static_cast<const char *>(src->getChars() + src->length()), matchResults, rx))
				{
					for (size_t i = 0; i < rx.mark_count(); i++)
					{
						const auto& m = matchResults[i + 1];
						auto val = StringBuilder::create(aCtx, src, m.first - src->contents(), m.second - src->contents());
						aCtx.push(val);
					}
				}
				else
				{
					aCtx.push(DataBuilders::createUndefinedValue());
				}
			}

			// �������� ������������ �� ����������� ���������.
			void match(SExecutionContext & aCtx)
			{
				const auto& arg0 = aCtx.getArg(0);
				const auto& arg1 = aCtx.getArg(1);

				BaseOps::opsCheck(StringOps::get(), arg0);
				BaseOps::opsCheck(StringOps::get(), arg1);

				const auto* const src = arg0.mString;
				const auto* const regEx = arg1.mString;

				std::regex rx(regEx->str());
				std::cmatch match;

				if (std::regex_match(static_cast<const char *>(src->getChars()), static_cast<const char *>(src->getChars() + src->length()), match, rx))
				{
					for (size_t i = 0; i < rx.mark_count(); ++i)
					{
						const auto& m = match[i + 1];
						const auto val = StringBuilder::create(aCtx, src, m.first - src->contents(), m.second - src->contents());
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
				const auto& arg0 = aCtx.getArg(0);
				const auto& arg1 = aCtx.getArg(1);
				const auto& arg2 = aCtx.getArg(2);

				BaseOps::opsCheck(StringOps::get(), arg0);
				BaseOps::opsCheck(StringOps::get(), arg1);
				BaseOps::opsCheck(StringOps::get(), arg2);

				const auto* const src = arg0.mString;
				const auto* const pattern = arg1.mString;
				const auto* const format = arg2.mString;

				const std::regex rx(pattern->str());
				const auto result = std::regex_replace(src->str(), rx, format->str());
				aCtx.push(StringBuilder::create(aCtx, result));
			}

			// ��������� ������� � ������ ������.
			void getToken(SExecutionContext & aCtx)
			{
				const auto & arg0 = aCtx.getArg(0);
				const auto & arg1 = aCtx.getArg(1);

				BaseOps::opsCheck(StringOps::get(), arg0);
				BaseOps::opsCheck(StringOps::get(), arg1);

				const auto* const src = arg0.mString;
				const auto* const pattern = arg1.mString;

				const std::regex rx("^(?:\\s*)(" + pattern->str() + ")");

				std::cmatch matchResults;

				const auto* const first = static_cast<const char *>(src->getChars());
				const auto* const last = static_cast<const char *>(src->getChars() + src->length());

				if (std::regex_search(first, last, matchResults, rx))
				{
					const auto prefix = StringBuilder::create(aCtx, src, 
						matchResults[1].first - src->contents(), 
						matchResults[1].second - src->contents());
					aCtx.push(prefix);

					const auto suffix = StringBuilder::create(aCtx, src, 
						matchResults.suffix().first - src->contents(), 
						matchResults.suffix().second - src->contents());
					aCtx.push(suffix);
				}
				else
				{
					aCtx.push(DataBuilders::createUndefinedValue());
				}
			}

			// �������������� � ������.
			void toString(SExecutionContext & aCtx)
			{
				std::stringstream strStream;
				strStream.precision(std::numeric_limits<double>::max_digits10);
				const auto& arg = aCtx.getArg(0);

				arg.getOps()->rawPrint(arg, strStream);

				aCtx.push(StringBuilder::create(aCtx, strStream.str()));
			}
		} // anonymous namespace

		const std::map<std::string, std::pair<TFunction, bool>> functions =
		{
			{ "toString", std::make_pair(&toString, true) },
			{ "length", std::make_pair(&length, false) },
			{ "search", std::make_pair(&search, true) },
			{ "replace", std::make_pair(&replace, true) },
			{ "match", std::make_pair(&match, true) },
			{ "getToken", std::make_pair(&getToken, true) },
		};

		void StringLib::Register()
		{
			FunctionLibrary::addFunctions(functions);
		}
	}
}
