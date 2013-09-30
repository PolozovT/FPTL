#include "FScheme.h"
#include "FSchemeVisitor.h"

#include <iostream>
#include <cassert>

namespace FPTL { namespace Runtime {

// execute-������ � ����� ����� ���� ����� ��� ������ ������ ���������� ����������.
// � ����� ������ ��� ������� ���� ������������ ��� �� ����� ����������.
// ��������������� ��� ��������� ��������� �������������� ��� ������� execute.

//-----------------------------------------------------------------------------------
FSchemeNode::FSchemeNode(bool aIsLong) : mIsLong(aIsLong)
{
	//mCompiledProc = createJITStub();
}

//-----------------------------------------------------------------------------------
FSequentialNode::FSequentialNode(FSchemeNode * aFirst, FSchemeNode * aSecond)
	: FSchemeNode(aFirst->isLong() || aSecond->isLong()),
	mFirst(aFirst),
	mSecond(aSecond)
{
}

void FSequentialNode::execute(SExecutionContext & aCtx) const
{
	// ���������� ���������� ���������.
	int arity = aCtx.arity;
	int argPos = aCtx.argPos;
	int size = aCtx.stack.size();
	aCtx.arity = 0;

	mFirst->execute(aCtx);

	// ������������ � ������ ������ �� �����.
	aCtx.advance();

	mSecond->execute(aCtx);

	// ����������� ����.
	aCtx.unwind(argPos, arity, size);
}

void FSequentialNode::accept(FSchemeVisitor * aVisitor) const
{
	aVisitor->visit(this);
}

//-----------------------------------------------------------------------------------
FConditionNode::FConditionNode(FSchemeNode * aCondition, FSchemeNode * aTrueBranch, FSchemeNode * aFalseBranch)
	: FSchemeNode(aCondition->isLong() || aTrueBranch->isLong() || (aFalseBranch && aFalseBranch->isLong())), 
	mCondition(aCondition),
	mTrueBranch(aTrueBranch),
	mFalseBranch(aFalseBranch)
{
	if (!mFalseBranch)
	{
		// ���� � �������� ����������� ���������� ���� ����� else, ������� ��� ��� ���������� UndefinedValue.
		mFalseBranch = new FFunctionNode([](SExecutionContext & aCtx) { 
			aCtx.push(DataBuilders::createUndefinedValue());
		});
	}
}

void FConditionNode::execute(SExecutionContext & aCtx) const
{
	static DataValue falseConst = DataBuilders::createBoolean(false);
	static DataValue undefined = DataBuilders::createUndefinedValue();

	int arity = aCtx.arity;

	// ���������� �������.
	mCondition->execute(aCtx);

	// ���������� �� ����� ����� 1 �������� - ��������� ���������� ���������.
	DataValue cond = aCtx.stack.back();
	bool isUndefined = false;

	int numArgs = aCtx.arity - arity;
	for (int i = 0; i < numArgs; ++i)
	{
		DataValue & arg = aCtx.stack.back();
		
		// ���������, ���������� �� � ������� �������������� �������� w ��� ���������� ��������� w*a = a*w = w.
		if (arg.getOps() == undefined.getOps())
		{
			isUndefined = true;
		}

		aCtx.stack.pop_back();
	}

	aCtx.arity = arity;

	// �������� �������.
	if (numArgs > 0 && (isUndefined || (cond.getOps() == falseConst.getOps() && !cond.mIntVal)))
	{
		mFalseBranch->execute(aCtx);
	}
	else
	{
		mTrueBranch->execute(aCtx);
	}
}

void FConditionNode::accept(FSchemeVisitor * aVisitor) const
{
	aVisitor->visit(this);
}

//-----------------------------------------------------------------------------------
FParallelNode::FParallelNode(FSchemeNode * aLeft, FSchemeNode * aRight)
	: FSchemeNode(aLeft->isLong() || aRight->isLong()), mLeft(aLeft), mRight(aRight)
{
}

void FParallelNode::execute(SExecutionContext & aCtx) const
{
	if (mLeft->isLong() && mRight->isLong())
	{
		// ������������ ����������.
		SExecutionContext * fork = aCtx.fork(mRight);

		mLeft->execute(aCtx);

		while (!fork->Ready.load(boost::memory_order_acquire))
		{
			aCtx.yield();
		}

		// �������� ���������.
		for (int i = 0; i < fork->arity; ++i)
		{
			aCtx.push(fork->stack.at(fork->stack.size() - fork->arity + i));
		}

		// ��������� ������ ���������� ������.
		aCtx.allocatedMemory.splice(aCtx.allocatedMemory.end(), fork->allocatedMemory);

		// TEST
		//aCtx.tryCollect();

		delete fork;
	}
	else
	{
		// ���������������� ����������.
		mLeft->execute(aCtx);

		mRight->execute(aCtx);
	}
}

void FParallelNode::accept(FSchemeVisitor * aVisitor) const
{
	aVisitor->visit(this);
}

//-----------------------------------------------------------------------------------
FScheme::FScheme(FSchemeNode * aFirstNode)
	: FSchemeNode(true), mFirstNode(aFirstNode)
{
}

void FScheme::execute(SExecutionContext & aCtx) const
{
	mFirstNode->execute(aCtx);
}

void FScheme::accept(FSchemeVisitor * aVisitor) const
{
	aVisitor->visit(this);
}

//-----------------------------------------------------------------------------------
void FFunctionNode::execute(SExecutionContext & aCtx) const
{
	call(this, &aCtx);
}

void FFunctionNode::accept(FSchemeVisitor * aVisitor) const
{
	aVisitor->visit(this);
}

void FFunctionNode::call(const FFunctionNode * aNode, SExecutionContext * aCtx)
{
	try
	{
		aNode->mFunction(*aCtx);
	}
	catch (const std::exception & e)
	{
		std::stringstream error;
		error << "Runtime error in function '" << aNode->mName << "' line: " << aNode->mLine << " column: " <<aNode->mColumn << ": " << e.what() << std::endl;
		
		// TODO: �������� ���� ���������� �������, � �������� ����������� �������� (printType).

		std::cerr << error.str();

		// �������� �� ���� ���������������.
		aCtx->push(DataBuilders::createUndefinedValue());
	}
}

}} // FPTL::Runtime