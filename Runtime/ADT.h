#pragma once

namespace FPTL
{
	namespace Runtime
	{
		class DataValue;
		class Constructor;

		// ���������� ������������� ������������ ���� ������ (boxed tuple).
		struct ADTValue
		{
			const Constructor * ctor;
			DataValue * values;

			ADTValue()
			{}

			ADTValue(const Constructor * ctor, DataValue * values)
			{
				this->ctor = ctor;
				this->values = values;
			}

			ADTValue(const ADTValue & other)
			{
				ctor = other.ctor;
				values = other.values;
			}
		};
	}
}
