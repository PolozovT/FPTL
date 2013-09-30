#pragma once

#include <stack>
#include <sstream>
#include <functional>

#include "Types.h"

namespace FPTL { namespace Runtime {

//-----------------------------------------------------------------------------
class Ops;

struct ADTValue;
struct StringValue;

// Variant ��� ������������� ���� ����� ������, ����� ��������������� ��������.
class DataValue
{
	friend class DataBuilders;

public:
	// ����������� �� ��������. ������� �������� ���� "����������������".
	DataValue();

	const Ops * getOps() const;

protected:
	DataValue(const Ops * aOps);

	const Ops * mOps;

// ������ � �������������� ����� ������ ��� ��������.
public:
	union
	{
		int mIntVal;
		double mDoubleVal;
		ADTValue * mADT;
		StringValue * mString;
	};
};

// �������������� ��������.
class UndefinedValue : public DataValue
{
	friend class DataBuilders;

	UndefinedValue(const Ops * aOps)
		: DataValue(aOps)
	{}
};

// ��������� ������� �������� ��� ������ ������
class Ops
{
public:
	virtual TypeInfo * getType(const DataValue & aVal) const = 0;
	
	// ��������� ���� ������ �� ���� ���������� ����� �����.
	virtual Ops * combine(const Ops * aOther) const = 0;
	virtual Ops * withOps(class IntegerOps const * aOps) const = 0;
	virtual Ops * withOps(class BooleanOps const * aOps) const = 0;
	virtual Ops * withOps(class DoubleOps const * aOps) const = 0;

	// �������������� �����.
	virtual int toInt(const DataValue & aVal) const = 0;
	virtual double toDouble(const DataValue & aVal) const = 0;
	virtual StringValue * toString(const DataValue & aVal) const = 0;

	// �������������� �������.
	virtual DataValue add(const DataValue & aLhs, const DataValue & aRhs) const = 0;
	virtual DataValue sub(const DataValue & aLhs, const DataValue & aRhs) const = 0;
	virtual DataValue mul(const DataValue & aLhs, const DataValue & aRhs) const = 0;
	virtual DataValue div(const DataValue & aLhs, const DataValue & aRhs) const = 0;
	virtual DataValue mod(const DataValue & aLhs, const DataValue & aRhs) const = 0;
	virtual DataValue abs(const DataValue & aArg) const = 0;

	// ������� ���������.
	virtual DataValue equal(const DataValue & aLhs, const DataValue & aRhs) const = 0;
	virtual DataValue less(const DataValue & aLhs, const DataValue & aRhs) const = 0;
	virtual DataValue greater(const DataValue & aLhs, const DataValue & aRhs) const = 0;

	// ����� ��� ����������� ���������� �������� ��� ������ ������.
	virtual void mark(const DataValue & aVal, std::stack<class Collectable *> & aMarkStack) const = 0;

	// ����� � �����.
	virtual void print(const DataValue & aVal, std::ostream & aStream) const = 0;
};

// ������������ ����� ������.
class DataBuilders
{
public:
	static DataValue createVal(Ops * aOps);

	static DataValue createInt(int aVal);
	static DataValue createDouble(double aVal);
	static DataValue createBoolean(bool aVal);

	static UndefinedValue createUndefinedValue();
	static DataValue createADT(ADTValue * mADT, Ops * aOps);
};

} // Runtime
} // FPTL
