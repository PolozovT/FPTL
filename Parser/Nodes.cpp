#include <cassert>
#include <boost/bind.hpp>
#include "Support.h"
#include "Nodes.h"
#include "Generated/Parser.tab.hh"

namespace FPTL { namespace Parser {

//-------------------------------------------------------------------------------------------
ExpressionNode::ExpressionNode( ASTNodeType aType, ASTNode * aLeft, ASTNode * aRight )
	: ASTNode(aType),
	mLeft(aLeft),
	mRight(aRight),
	mMiddle(0)
{
}

ExpressionNode::ExpressionNode( ASTNodeType aType, ASTNode * aLeft, ASTNode * aRight, ASTNode * aMiddle )
	: ASTNode(aType),
	mLeft(aLeft),
	mMiddle(aMiddle),
	mRight(aRight)
{
}

ExpressionNode::~ExpressionNode()
{
	delete mLeft;
	delete mRight;
	delete mMiddle;
}

void ExpressionNode::accept( NodeVisitor * aVisitor )
{
	aVisitor->visit(this);
}

ASTNode * ExpressionNode::copy() const
{
	return new ExpressionNode( getType(),
		mLeft ? mLeft->copy() : 0,
		mRight ? mRight->copy() : 0,
		mMiddle ? mMiddle->copy() : 0
	);
}

//-------------------------------------------------------------------------------------------

ListNode::~ListNode()
{
	for( ListNode::iterator it = begin(); it != end(); ++it )
		delete *it;
}

void ListNode::accept( NodeVisitor * aVisitor )
{
	aVisitor->visit(this);
}

ListNode * ListNode::copy() const
{
	ListNode * newList = new ListNode( getType() );
	std::for_each( begin(), end(), [newList](ASTNode * aElem) { newList->push_back(aElem->copy()); } );
	return newList;
}

//-------------------------------------------------------------------------------------------

DataNode::DataNode( Ident aTypeName, ListNode * aTypeDefs, ListNode * aTypeParams, ListNode * aConstructors )
	: ASTNode(DataTypeDefinitionBlock),
	mTypeName(aTypeName),
	mTypeDefinitions(aTypeDefs),
	mTypeParameters(aTypeParams),
	mConstructors(aConstructors)
{
}

DataNode::~DataNode()
{
	delete mConstructors;
	delete mTypeDefinitions;
	delete mTypeParameters;
}

void DataNode::accept( NodeVisitor * aVisitor )
{
	aVisitor->visit(this);
}

ASTNode * DataNode::copy() const
{
	return new DataNode( mTypeName,
		mTypeDefinitions ? mTypeDefinitions->copy() : 0,
		mTypeParameters ? mTypeParameters->copy() : 0,
		mConstructors ? mConstructors->copy() : 0 
	);
}

//-------------------------------------------------------------------------------------------

ConstructorNode::ConstructorNode( Ident aName, ListNode * aCtorParameters, Ident aCtorResultTypeName )
	: ASTNode( Constructor ),
	mName(aName),
	mCtorParameters(aCtorParameters),
	mCtorResultTypeName(aCtorResultTypeName)
{
}

void ConstructorNode::accept( NodeVisitor * aVisitor )
{
	aVisitor->visit(this);
}

ConstructorNode::~ConstructorNode()
{
	delete mCtorParameters;
}

ASTNode * ConstructorNode::copy() const
{
	return new ConstructorNode( mName, mCtorParameters ? mCtorParameters->copy() : 0, mCtorResultTypeName );
}

//-------------------------------------------------------------------------------------------

NameRefNode::NameRefNode( Ident aTypeName, ASTNodeType aNodeType )
	: ASTNode( aNodeType ),
	mTypeName( aTypeName ),
	mParameters(0),
	mTarget(0)
{
}

NameRefNode::NameRefNode(  Ident aTypeName, ASTNodeType aNodeType, ListNode * aParams )
	: ASTNode( aNodeType ),
	mTypeName( aTypeName ),
	mParameters( aParams ),
	mTarget(0)
{
}

NameRefNode::~NameRefNode()
{
	delete mParameters;
}

void NameRefNode::accept( NodeVisitor * aVisitor )
{
	aVisitor->visit(this);
}

ASTNode * NameRefNode::copy() const
{
	return new NameRefNode( mTypeName, getType(), mParameters ? mParameters->copy() : 0 );
}

//-------------------------------------------------------------------------------------------

FunctionNode::FunctionNode( Ident aFuncName, ListNode * aDefinitions, ListNode * aFormalParams )
	: ASTNode( ASTNode::FunctionBlock ),
	mFuncName(aFuncName),
	mDefinitions(aDefinitions ),
	mFormalParameters(aFormalParams)
{
}

FunctionNode::~FunctionNode()
{
	delete mDefinitions;
	delete mFormalParameters;
}

void FunctionNode::accept( NodeVisitor * aVisitor )
{
	aVisitor->visit(this);
}

FunctionNode * FunctionNode::copy() const
{
	FunctionNode * copy = new FunctionNode( mFuncName, mDefinitions ? mDefinitions->copy() : 0,
		mFormalParameters ? mFormalParameters->copy() : 0 );

	return copy;
}

DefinitionNode * FunctionNode::getDefinition(Ident aName) const
{
	for each (DefinitionNode * def in *mDefinitions)
	{
		if (def->getDefinitionName() == aName)
		{
			return def;
		}
	}

	return 0;
}

//-------------------------------------------------------------------------------------------

DefinitionNode::DefinitionNode( ASTNodeType aType, Ident aName, ASTNode * aDefinition )
	: ASTNode( aType ),
	mDefinitionName(aName),
	mDefinition(aDefinition)
{
}

DefinitionNode::~DefinitionNode()
{
	delete mDefinition;
}

void DefinitionNode::accept( NodeVisitor * aVisitor )
{
	aVisitor->visit(this);
}

ASTNode * DefinitionNode::copy() const
{
	return new DefinitionNode( getType(), mDefinitionName, mDefinition ? mDefinition->copy() : 0 );
}

//-------------------------------------------------------------------------------------------

FunctionalProgram::FunctionalProgram( ListNode * aDataDefinitions, FunctionNode * aScheme, ApplicationBlock * aApplication )
	: ASTNode(FunctionalProgramDef),
	mDataDefinitions(aDataDefinitions),
	mScheme(aScheme),
	mApplication(aApplication)
{
}

FunctionalProgram::~FunctionalProgram()
{
	delete mDataDefinitions;
	delete mScheme;
	delete mApplication;
}

void FunctionalProgram::accept( NodeVisitor * aVisitor )
{
	aVisitor->visit(this);
}

ASTNode * FunctionalProgram::copy() const
{
	return new FunctionalProgram(
		mDataDefinitions ? mDataDefinitions->copy() : 0,
		mScheme ? mScheme->copy() : 0,
		mApplication ? mApplication->copy() : 0
	);
}

//-------------------------------------------------------------------------------------------

ApplicationBlock::ApplicationBlock(NameRefNode * aRunSchemeName, ASTNode * aSchemeParameters, ListNode * aDataVarDefs)
	: ASTNode(Application),
	mRunSchemeName(aRunSchemeName),
	mSchemeParameters(aSchemeParameters),
	mDataVarDefinitions(aDataVarDefs)
{
}

ApplicationBlock::~ApplicationBlock()
{
	delete mSchemeParameters;
	delete mDataVarDefinitions;
}

ApplicationBlock * ApplicationBlock::copy() const
{
	return new ApplicationBlock(mRunSchemeName, mSchemeParameters ? mSchemeParameters->copy() : 0,
		mDataVarDefinitions ? mDataVarDefinitions->copy() : 0);
}

void ApplicationBlock::accept(NodeVisitor * aVisitor)
{
	aVisitor->visit(this);
}

//-------------------------------------------------------------------------------------------

void ConstantNode::accept(NodeVisitor * aVisitor)
{
	aVisitor->visit(this);
}

ASTNode * ConstantNode::copy() const
{
	return new ConstantNode( getType(), mIdent );
}

bool ConstantNode::isNatural() const
{
	return getType() == ASTNode::IntConstant && atoi(mIdent.getStr().c_str()) > 0;
}

}} // FPTL::Parser