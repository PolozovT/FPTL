#include "Parser/Nodes.h"
#include "Parser/NodeVisitor.h"

using namespace System;
using namespace System::Collections;

namespace FPTL
{
	namespace Runtime
	{
		ref class Constructor
		{
		public:

		private:
			array<Type^> ^ mTypeParameters;
			array<int,2> mTypeParametersOrder;
			Type ^ mConstructingType;
		};

		//
		// ����� ��� �������� ������������� � FPTL.
		//
		ref class ConstructorBuilder
		{
		public:
			
			void createConstructor( DataNode * aDataNode )
			{
				Generic::Dictionary<String^,Type^>^ typeParametersTypes = gcnew Generic::Dictionary<String^,Type^>();

				// ������� � �������������� ��������� nullptr ������� ���������.
				ListNode * typeParams = aDataNode->getTypeParams();
				if( !typeParams->empty() )
				{
					for( unsigned i = 0; i < typeParams->size(); i++ )
					{
						DefinitionNode * paramName = static_cast<DefinitionNode*>( (*typeParams)[i] );
						typeParametersTypes->Add( gcnew String( paramName->getDefinitionName().getStr().c_str() ), nullptr );
					}

					// ���������� ��� ��� ������ ������� ����������.
				}
			}

		private:
		};
	}
}