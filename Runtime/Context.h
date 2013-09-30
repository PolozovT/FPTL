#pragma once

#include <vector>
#include <stack>

#include <boost/atomic.hpp>

#include "Data.h"

namespace FPTL {
namespace Runtime {

//---------------------------------------------------------------------------------------------
class EvaluatorUnit;
class FSchemeNode;

// �������� �����������.
struct SExecutionContext
{
	// ��������� �� �����.
	FSchemeNode * Scheme;

	// ��������� �� �������� ��������.
	SExecutionContext * Parent;

	// ���� ���������� �������.
	boost::atomic<size_t> Ready;

	// ���� ��� ������ � ���������.
	std::vector<DataValue> stack;

	// ��������� ������� ����������.
	int argPos;

	// ������ ������� ��������.
	int arity;

	// ������ ���������� ������.
	std::list<void *> allocatedMemory;

	// ����� ������, ���������� �� ������ ������.
	int numCollected;

	SExecutionContext();

	// ������ ������ � �������.
	const DataValue & getArg(int index) const;
	void push(const DataValue & aData);
	void advance();
	void unwind(int aArgPosOld, int aArity, int aPos);

	// ���������� ������� � ����.
	void * alloc(size_t aSize);
	int collect();
	void tryCollect();

	// ������ ����������.
	void run(EvaluatorUnit * aEvaluatorUnit);

	// ���������� ������ �������. ����� ���������� ������� ��������� ����� ������� �� ������ aResult.
	SExecutionContext * fork(FSchemeNode * aFirstNode);

	// ��������� ����������, �������� ������� ��������.
	void yield();

	// ����������� ������ ������� ��� ������ �� JIT-����������������� ����.

	/*static SExecutionContext * doFork(SExecutionContext * aCtx, FSchemeNode * aFirstNode, DataElement * volatile * aResult)
	{
		return aCtx->fork(aFirstNode, aResult);
	}

	static void doYield(SExecutionContext * aCtx)
	{
		aCtx->yield();
	}*/

private:

	EvaluatorUnit * mEvaluatorUnit;
};

// ��������� �������� � �������������� ����������� �������.
class Collectable
{
public:
	virtual void mark(std::stack<Collectable *> & aMarkStack) = 0;
};

} // Runtime
} // FPTL