#include "SemanticCheck.h"
#include "boost/bind.hpp"

// Semantic.cpp - ���������� ������ ��� ������������� �������� ��������� ���������.

namespace FPTL { namespace Parser {

//---------------------------------------------------------------------------
NamesChecker::NamesChecker( Support * aSupport ) : mSupport(aSupport)
{}

//---------------------------------------------------------------------------
void NamesChecker::visit( DataNode * aDataNode )
{
	if( mContext.insertName( aDataNode->getDataName(), aDataNode ) == false )
	{
		mSupport->semanticError( ErrTypes::DuplicateDefinition, aDataNode->getDataName() );
	}

	pushContext();

	NodeVisitor::visit(aDataNode);

	checkNames();
	popContext();
}

//---------------------------------------------------------------------------
void NamesChecker::visit( FunctionNode * aFunctionNode )
{
	mContext.insertName( aFunctionNode->getFuncName(), aFunctionNode );
	pushContext();

	NodeVisitor::visit(aFunctionNode);

	// ��������� ������� �������� ���������.
	if (!aFunctionNode->getDefinition(aFunctionNode->getFuncName()))
	{
		mSupport->semanticError(ErrTypes::MissingMainDefinition, aFunctionNode->getFuncName());
	}

	// ��������� �������� ����.
	checkNames();
	popContext();
}


//---------------------------------------------------------------------------
void NamesChecker::visit( DefinitionNode * aDefinitionNode )
{
	switch (aDefinitionNode->getType())
	{
		case ASTNode::FunctionParameterDefinition:
		case ASTNode::Definition:
		case ASTNode::TypeParameterDefinition:
		case ASTNode::InputVarDefinition:
			if (mContext.insertName( aDefinitionNode->getDefinitionName(),aDefinitionNode ) == false)
			{
				// ��������� �����������.
				mSupport->semanticError( ErrTypes::DuplicateDefinition, aDefinitionNode->getDefinitionName() );
			}
			break;

		case ASTNode::TypeConstructorDefinition:
			if (mContextStack[0].insertName( aDefinitionNode->getDefinitionName(), aDefinitionNode ) == false)
			{
				mSupport->semanticError( ErrTypes::DuplicateDefinition, aDefinitionNode->getDefinitionName() );
			}
			break;
	}

	NodeVisitor::visit(aDefinitionNode);
}

//---------------------------------------------------------------------------
void NamesChecker::visit( NameRefNode * aNameNode )
{
	STermDescriptor termDesc;
	termDesc.TermName = aNameNode->getName();
	termDesc.Node = aNameNode;

	switch( aNameNode->getType() )
	{
		case ASTNode::InputVarName:
		case ASTNode::FuncObjectName:
		case ASTNode::TypeParamName:
		case ASTNode::ConstructorName:
		case ASTNode::FuncObjectWithParameters:
		case ASTNode::FuncParameterName:
		case ASTNode::RunningSchemeName:
			mContext.TermsList.push_back( termDesc );
			break;

		case ASTNode::Type:
		case ASTNode::Template:
			mContextStack[0].TermsList.push_back( termDesc );
			break;
	}

	NodeVisitor::visit(aNameNode);
}

//---------------------------------------------------------------------------
void NamesChecker::visit( ApplicationBlock * aApplicationBlock )
{
	NodeVisitor::visit(aApplicationBlock);

	checkNames();
}

//---------------------------------------------------------------------------
void NamesChecker::pushContext()
{
	mContextStack.push_back( mContext );
	mContext.clear();
}

//---------------------------------------------------------------------------
void NamesChecker::popContext()
{
	mContext = mContextStack.back();
	mContextStack.pop_back();
}

//---------------------------------------------------------------------------
void NamesChecker::checkName( STermDescriptor & aTermDesc )
{
	ASTNode * target = 0;

	std::vector<STermDescriptor> undefinedTerms;

	// ������� ���� � ��������� ������������ ����.
	std::map<Ident,ASTNode*>::iterator pos = mContext.DefinitionsList.find( aTermDesc.TermName );

	if( pos != mContext.DefinitionsList.end() )
	{
		target = pos->second;
	}
	else
	{
		// ����� ���� � ���������� ������������ ����.
		if( !mContextStack.empty() )
		{
			pos = mContextStack[0].DefinitionsList.find( aTermDesc.TermName );

			if( pos != mContextStack[0].DefinitionsList.end() )
			{
				target = pos->second;
			}
			else
			{
				mSupport->semanticError( ErrTypes::UndefinedIdentifier, aTermDesc.TermName );
				return;
			}
		}
		else
		{
			mSupport->semanticError( ErrTypes::UndefinedIdentifier, aTermDesc.TermName );
			return;
		}
	}

	if (aTermDesc.Node->getType() == ASTNode::FuncParameterName)
	{
		if (target->getType() == ASTNode::FunctionParameterDefinition)
		{
			mSupport->semanticError(ErrTypes::InvalidFuncallParameters, aTermDesc.TermName);
		}
	}

	if (aTermDesc.Node->getType() == ASTNode::RunningSchemeName && target->getType() && target->getType() != ASTNode::FunctionBlock)
	{
		mSupport->semanticError(ErrTypes::UndefinedSchemeName, aTermDesc.TermName);
	}

	if( aTermDesc.Node->numParameters() == target->numParameters() )
	{
		aTermDesc.Node->setTarget( target );
	}
	else
	{
		// TODO: �������� ����� ��������� � ����������� ����������.
		mSupport->semanticError( ErrTypes::InvalidNumberOfParameters, aTermDesc.TermName );
	}
}

//---------------------------------------------------------------------------
void NamesChecker::checkNames()
{
	std::for_each( mContext.TermsList.begin(), mContext.TermsList.end(), boost::bind( &NamesChecker::checkName, this, _1 ) );
}

}} // FPTL::Parser