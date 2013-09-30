#pragma once

#include <vector>

#include <boost/thread.hpp>

#include "WorkStealingQueue.h"

namespace FPTL { namespace Runtime {

struct SExecutionContext;
class FSchemeNode;
class SchemeEvaluator;

// ����������� �����, �������� � ����������� ������.
class EvaluatorUnit
{
public:

	EvaluatorUnit(SchemeEvaluator * aSchemeEvaluator);

	// �������� ���������, � ������� ������������ ����������.
	void evaluateScheme();

	// ���������� ������ ������� � �������.
	// ���������� ������ �� ������, � �������� �������� ��� �� ��� ��������.
	void addJob(SExecutionContext * aContext);

	// ��������� ������ ��� ������� ������.
	// ���������� �� ������ ������.
	SExecutionContext * stealJob();

	// ����� � ���������� �������.
	void schedule();

private:

	int mJobsCompleted;
	int mJobsCreated;
	int mJobsStealed;
	WorkStealingQueue<SExecutionContext *> mJobQueue;
	SchemeEvaluator * mEvaluator;
};

// ���������� ���������� ���������, �������� �������������� ������.
// ���������, ������� � ��������� ����� ��������.
class SchemeEvaluator
{
public:
	SchemeEvaluator();

	// ������ ���������� �����.
	void runScheme(const FSchemeNode * aScheme, const FSchemeNode * aInputGenerator, int aNumEvaluators);

	// ��������� ����������.
	void stop();

	// ����� ������� � ������ ������������. ���������� 0, ���� �� ����������.
	SExecutionContext * findJob(const EvaluatorUnit * aUnit);

private:
	std::vector<EvaluatorUnit *> mEvaluatorUnits;
	boost::thread_group mThreadGroup;
};

}} // FPTL::Runtime
