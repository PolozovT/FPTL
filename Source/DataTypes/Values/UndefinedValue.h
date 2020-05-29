#pragma once
#include "DataValue.h"

namespace FPTL
{
	namespace Runtime
	{
		// �������������� ��������.
		class UndefinedValue : public DataValue
		{
			friend class DataBuilders;

			explicit UndefinedValue(const Ops * aOps)
				: DataValue(aOps)
			{}
		};
	}
}
