#ifndef SEMANTIC_CHECK_H
#define SEMANTIC_CHECK_H

#include <map>
#include <algorithm>
#include <cassert>
#include "Support.h"
#include "Nodes.h"
#include <boost/bind.hpp>

namespace FPTL {
namespace Parser {

//
// NamesChecker. ��������� ���� �� ������������ ��� ���������� � ������� ��������� (������, �������).
//
class NamesChecker : public NodeVisitor
{
	struct STermDescriptor
	{
		Ident TermName;
		NameRefNode * Node;

		bool operator<( const STermDescriptor & aOther ) const { return TermName < aOther.TermName; }
	};

	struct SLexicalContext
	{
		std::vector<STermDescriptor> TermsList;
		std::map<Ident, ASTNode*> DefinitionsList;

		void clear()
		{
			TermsList.clear();
			DefinitionsList.clear();
		}

		bool insertName( const Ident & aIdent, ASTNode * aNode )
		{
			return DefinitionsList.insert( std::make_pair( aIdent, aNode ) ).second;
		}
	};

public:

	NamesChecker( Support * aSupport );

	void visit( DataNode * aDataNode );
	void visit( FunctionNode * aFunctionNode );
	void visit( DefinitionNode * aDefinitionNode );
	void visit( NameRefNode * aNameNode );
	void visit( ApplicationBlock * aApplicationBlock );

private:

	void pushContext();
	void popContext();
	void checkName( STermDescriptor & aTermDesc );
	void checkNames();

	SLexicalContext mContext;
	std::vector<SLexicalContext> mContextStack;

	Support * mSupport;
};

//
// ������� ����������� ������� ���������, ���������� �� ��������� ����������� ������.
//
class RecursionFinder : public NodeVisitor
{
public:

	void visit( DefinitionNode * aDefinitionNode )
	{
		mTestDefinitions.push_back( aDefinitionNode->getDefinitionName() );

		if( aDefinitionNode->getType() == ASTNode::Definition )
		{
			process(aDefinitionNode->getDefinition());

		}

		mTestDefinitions.pop_back();
	}

	void visit( NameRefNode * aNameRefNode )
	{
		if (aNameRefNode->getType() == ASTNode::FuncObjectName || aNameRefNode->getType() == ASTNode::FuncParameterName)
		{
			ASTNode * target = aNameRefNode->getTarget();

			// ����������� ��������.
			if (std::find(mTestDefinitions.begin(), mTestDefinitions.end(), aNameRefNode->getName() ) != mTestDefinitions.end())
			{
				target->increaseCyclicIndex();
			}
			else
			{	
				process(target);
			}
		}

		if (aNameRefNode->getType() == ASTNode::FuncObjectWithParameters)
		{
			// ���� �������� ����� �������������� ����������.
			process(aNameRefNode->getParameters());
		}
	}

private:

	std::vector<Ident> mTestDefinitions;
};

} // Parser
} // FPTL

#endif