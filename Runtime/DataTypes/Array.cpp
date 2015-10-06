﻿#include <boost/format.hpp>

#include "../Array.h"

namespace FPTL
{
namespace Runtime 
{
//-----------------------------------------------------------------------------
// Операции над массивами.
class ArrayOps : public BaseOps
{
public:
	static ArrayOps * get()
	{
		static ArrayOps ops;
		return &ops;
	}

	// Информация о конкретном типе массива во время выполнения не сохраняется.
	virtual TypeInfo * getType(const DataValue & aVal) const
	{
		static TypeInfo info = typeInfo();
		return &info;
	}
	
	// Добавлять сюда методы по мере добавления новых типов.
	virtual Ops * combine(const Ops * aOther) const
	{
		throw invalidOperation("combine");
	}

	virtual Ops * withOps(class IntegerOps const * aOps) const
	{
		throw invalidOperation("toInt");
	}

	virtual Ops * withOps(class BooleanOps const * aOps) const
	{
		throw invalidOperation("toBoolean");
	}

	virtual Ops * withOps(class DoubleOps const * aOps) const
	{
		throw invalidOperation("toDouble");
	}

	virtual void mark(const DataValue & aVal, std::stack<class Collectable *> & aMarkStack) const
	{
		aMarkStack.push(aVal.mArray);
	}

	// Вывод содержимое массива.
	virtual void print(const DataValue & aVal, std::ostream & aStream) const
	{
		aStream << "[";
		ArrayValue * arr = aVal.mArray;
		for (int i = 0; i < arr->length; ++i)
		{
			DataValue & val = arr->arrayData[i];
			val.getOps()->print(val, aStream);

			if (i < arr->length - 1)
			{
				aStream << ",";
			}
		}		
		aStream << "]";
	}

private:
	static TypeInfo typeInfo()
	{
		TypeInfo info("array");
		info.addParameter("'t", TypeInfo("'t"));
		return info;
	}

	std::runtime_error invalidOperation(const std::string & name) const
	{
		return std::runtime_error(boost::str(boost::format("Invalid operation on array: %1%.") % name));
	}
};

//-----------------------------------------------------------------------------
DataValue ArrayValue::create(SExecutionContext & ctx, int size, const DataValue & initial)
{
	if (size <= 0)
	{
		throw std::exception("Array size must be positive integer number.");
	}

	void * memory = ctx.alloc(sizeof(ArrayValue) + sizeof(DataValue) * size);
	auto val = new(memory) ArrayValue(initial.getOps(), size);

	// Создаем массив и заполняем его начальным значением.
	val->arrayData = new (static_cast<char *>(memory) + sizeof(ArrayValue)) DataValue();
	std::fill_n(val->arrayData, size, initial);

	DataValue res = DataBuilders::createVal(ArrayOps::get());
	res.mArray = val;
	return res;
}

//-----------------------------------------------------------------------------
DataValue ArrayValue::get(const DataValue & arr, int pos)
{
	if (arr.getOps() != ArrayOps::get())
	{
		throw std::runtime_error("Invalid operation on not array value.");
	}

	ArrayValue * trg = arr.mArray;

	if (unsigned(pos) >= trg->length)
	{
		throw outOfRange();
	}

	return trg->arrayData[pos];
}

//-----------------------------------------------------------------------------
void ArrayValue::set(DataValue & arr, int pos, const DataValue & val)
{
	if (arr.getOps() != ArrayOps::get())
	{
		throw std::runtime_error("Invalid operation on not array value.");
	}

	ArrayValue * trg = arr.mArray;

	if (unsigned(pos) >= trg->length)
	{
		throw outOfRange();
	}

	if (trg->ops != val.getOps())
	{
		auto srcType = val.getOps()->getType(val);
		auto dstType = trg->ops->getType(trg->arrayData[0]);
		std::string str = boost::str(boost::format("Cannot assign %1% to an array of type %2%") % srcType->TypeName % dstType->TypeName);
		throw std::runtime_error(str);
	}

	trg->arrayData[pos] = val;
}

//-----------------------------------------------------------------------------

}} // FPTL::Runtime