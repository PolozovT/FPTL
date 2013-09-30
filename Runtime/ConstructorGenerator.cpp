#include <iterator>

#include "../Parser/Nodes.h"
#include "ConstructorGenerator.h"
#include "Functions.h"

namespace FPTL { namespace Runtime {

//-------------------------------------------------------------------------------------------
ConstructorGenerator::ConstructorGenerator()
{
}

//-------------------------------------------------------------------------------------------
void ConstructorGenerator::visit(Parser::DataNode * aData)
{
	mCurrentData = aData;

	NodeVisitor::visit(aData);
}

//-------------------------------------------------------------------------------------------
void ConstructorGenerator::visit(Parser::NameRefNode * aNameReference)
{
	if (aNameReference->getType() == Parser::ASTNode::Template)
	{
		mStack.push(mTypeTuple);
		mTypeTuple.clear();
	}

	NodeVisitor::visit(aNameReference);

	switch (aNameReference->getType())
	{
		case Parser::ASTNode::TypeParamName:
		case Parser::ASTNode::BaseType:
		case Parser::ASTNode::Type:
		{
			mTypeTuple.push_back(TypeInfo(aNameReference->getName().getStr()));
			break;
		}

		// ���������������� ��� � �����������.
		case Parser::ASTNode::Template:
		{
			TypeInfo newType(aNameReference->getName().getStr());
			
			// ��������� ���������.
			int i = 0;

			Parser::DataNode * data = static_cast<Parser::DataNode *>(aNameReference->getTarget());

			std::for_each(data->getTypeParams()->begin(), data->getTypeParams()->end(),
				[&newType, this, &i](Parser::ASTNode * aParam)
				{
					Parser::NameRefNode * paramName = static_cast<Parser::NameRefNode *>(aParam);
					newType.Parameters[paramName->getName().getStr()] = mTypeTuple[i];
					i++;
				}
			);

			mTypeTuple = mStack.top();
			mStack.pop();
			mTypeTuple.push_back(newType);
			break;
		}
	}
}

//-------------------------------------------------------------------------------------------
void ConstructorGenerator::visit(Parser::DefinitionNode * aDefinition)
{
	NodeVisitor::visit(aDefinition);

	if (aDefinition->getType() == Parser::ASTNode::TypeConstructorDefinition)
	{
		std::string dataName = mCurrentData->getDataName().getStr();
		std::string constructorName = aDefinition->getDefinitionName().getStr();

		Constructor * constructor = !mTypeTuple.empty() ? new Constructor(constructorName, dataName, mTypeTuple)
			: new EmptyConstructor(constructorName, dataName);

		// ��������� ����� ����������� � ������.
		mConstructors.insert(std::make_pair(constructorName, constructor));

		mTypeTuple.clear();
	}
}

//-------------------------------------------------------------------------------------------
Constructor * ConstructorGenerator::getConstructor(const std::string & aConstructorName) const
{
	return mConstructors.at(aConstructorName).get();
}

}} // FPTL::Runtime