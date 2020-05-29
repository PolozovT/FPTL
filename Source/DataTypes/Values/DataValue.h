#pragma once

namespace FPTL
{
	namespace Runtime
	{
		class Ops;
		struct ADTValue;
		struct StringValue;
		struct ArrayValue;

		// Variant ��� ������������� ���� ����� ������, ����� ��������������� ��������.
		class DataValue
		{
			friend class DataBuilders;

		public:
			// ����������� �� ���������. ������� �������� ���� "����������������".
			DataValue();
			explicit DataValue(const Ops * aOps);
			DataValue(const DataValue & other)
			{
				*this = other;
			}

			const Ops * getOps() const;

		protected:
			const Ops * mOps;

			// ������ � �������������� ����� ������ ��� ��������.
		public:
			// �� ����� ����� ������������ ����, ������ ������� �� 8� -
			// ������ ����� ����������� �� ���, ��� ���������.
			union
			{
				int64_t mIntVal;
				double mDoubleVal;
				ADTValue* mADT;
				StringValue* mString;
				ArrayValue* mArray;
			}; // size = max of members size 
		};
	}
}
