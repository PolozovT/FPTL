#pragma once

#include "Context.h"

namespace FPTL {
namespace Runtime {

struct StringData;

//-----------------------------------------------------------------------------
struct StringValue : public Collectable
{
	StringData * data;
	int begin;
	int end;

	virtual void mark(std::stack<Collectable *> & aMarkStack);

	char * getChars() const;
	int length() const;
	std::string str() const;
};

//-----------------------------------------------------------------------------
// ����� ��� �������� �����.
class StringBuilder
{
public:
	static DataValue create(SExecutionContext & aCtx, const std::string & aData);
	static DataValue create(SExecutionContext & aCtx, int aSize);
	static DataValue create(SExecutionContext & aCtx, const StringValue * aOther, int aBegin, int aEnd);
};

//-----------------------------------------------------------------------------

} // Runtime
} // FPTL