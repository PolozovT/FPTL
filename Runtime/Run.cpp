﻿#include <assert.h>

#include <iostream>
#include <unordered_set>
#include <stack>
#include <boost/timer/timer.hpp>

#include "Run.h"
#include "FScheme.h"
#include "WorkStealingQueue.h"

#undef max

namespace FPTL { namespace Runtime {

//-----------------------------------------------------------------------------
SExecutionContext::SExecutionContext()
	: Parent(0),
	mEvaluatorUnit(0),
	arity(0),
	argPos(0),
	numAllocated(0),
	Ready(0),
	prevAllocated(0),
	argNum(0)
{
	stack.reserve(10);
}

bool SExecutionContext::isReady() const
{
	return Ready.load(std::memory_order_acquire) == 1;
}

SExecutionContext * SExecutionContext::spawn(FSchemeNode * aScheme)
{
	SExecutionContext * fork = new SExecutionContext();
	fork->Scheme = aScheme;
	fork->Parent = this;

	// Копируем стек.
	for (int i = argPos; i < (argPos + argNum); i++)
	{
		fork->stack.push_back(stack.at(i));
	}

	fork->argNum = argNum;
	return fork;
}

EvaluatorUnit * SExecutionContext::evaluator() const
{
	return mEvaluatorUnit;
}

CollectedHeap & SExecutionContext::heap() const
{
	return mEvaluatorUnit->heap();
}

void SExecutionContext::run(EvaluatorUnit * aEvaluatorUnit)
{
	assert(!mEvaluatorUnit);
	mEvaluatorUnit = aEvaluatorUnit;
	mEvaluatorUnit->runningTasks.push_back(this);

	Scheme->execute(*this);

	mEvaluatorUnit->runningTasks.pop_back();

	// Сообщаем о готовности задания.
	Ready.store(1, std::memory_order_release);
}

const DataValue & SExecutionContext::getArg(int aIndex) const
{
	return stack.at(argPos + aIndex);
}

void SExecutionContext::push(const DataValue & aData)
{
	stack.push_back(aData);
	++arity;
}

void SExecutionContext::advance()
{
	argPos = stack.size() - arity;
	argNum = arity;
	arity = 0;
}

void SExecutionContext::unwind(int aArgPosOld, int aArity, int aPos)
{
	for (int i = 0; i < arity; ++i)
	{
		stack[aPos + i] = stack[stack.size() - arity + i];
	}

	argPos = aArgPosOld;
	stack.resize(aPos + arity);
	arity += aArity;
}

//-----------------------------------------------------------------------------
EvaluatorUnit::EvaluatorUnit(SchemeEvaluator * aSchemeEvaluator)
	: mEvaluator(aSchemeEvaluator),
	  mJobsCompleted(0),
	  mJobsCreated(0),
	  mJobsStealed(0),
	  mHeap(aSchemeEvaluator->garbageCollector())
{
}

void EvaluatorUnit::addJob(SExecutionContext * aContext)
{
	mJobsCreated++;
	mJobQueue.push(aContext);
}

SExecutionContext * EvaluatorUnit::stealJob()
{
	SExecutionContext * elem = 0;
	mJobQueue.steal(elem);

	if (elem)
	{
		mJobsStealed++;
	}

	return elem;
}

void EvaluatorUnit::evaluateScheme()
{
	while (true)
	{
        try
        {
            boost::this_thread::interruption_point();
        
            schedule();
        }
        catch (boost::thread_interrupted)
        {
            break;
        }
	}
    
    static boost::mutex outputMutex;
    boost::lock_guard<boost::mutex> guard(outputMutex);

	// Выводим статистику.
	std::stringstream ss;
	ss << "\nJobs processed by thread id = " << boost::this_thread::get_id() << " " << mJobsCompleted << " stealed " << mJobsStealed;
	std::cout << ss.str();
}

void EvaluatorUnit::fork(SExecutionContext * task)
{
	pendingTasks.push_back(task);
	addJob(task);
}

void EvaluatorUnit::join(SExecutionContext * task, SExecutionContext * joinTask)
{
	while (!joinTask->Ready.load(std::memory_order_acquire))
	{
		schedule();
	}

	pendingTasks.pop_back();
}

void EvaluatorUnit::schedule()
{
	// Берем задание из своей очереди.
	SExecutionContext * context = 0;
		
	if (mJobQueue.pop(context))
	{
		context->run(this);
		mJobsCompleted++;
		return;
	}

	// Если это не удалось, берем у кого-нибудь другого.
	context = mEvaluator->findJob(this);
	
	if (context)
	{
		context->run(this);
		mJobsCompleted++;
	}
	else
	{
		mEvaluator->safePoint();
	}
}

CollectedHeap & EvaluatorUnit::heap() const
{
	return mHeap;
}

void EvaluatorUnit::markDataRoots(GarbageCollector * collector)
{
	// Помечаем корни у всех заданий, взятых на выполнение текщим юнитом.
	for (SExecutionContext * ctx : runningTasks)
	{
		std::for_each(ctx->stack.begin(), ctx->stack.end(),
			[collector](DataValue & data) {
				data.getOps()->mark(data, collector);
			}
		);
	}

	// Помечаем корни у всех форкнутых заданий, которые были выполнены.
	for (SExecutionContext * ctx : pendingTasks)
	{
		if (ctx->isReady())
		{
			std::for_each(ctx->stack.begin(), ctx->stack.end(),
				[collector](DataValue & data) {
					data.getOps()->mark(data, collector);
				}
			);
		}
	}
}

//-----------------------------------------------------------------------------
SchemeEvaluator::SchemeEvaluator()
{
}

void SchemeEvaluator::stop()
{
    boost::lock_guard<boost::mutex> guard(mStopMutex);
    
	mThreadGroup.interrupt_all();
}

SExecutionContext * SchemeEvaluator::findJob(const EvaluatorUnit * aUnit)
{
	for (size_t i = 0; i < mEvaluatorUnits.size(); i++)
	{
		if (mEvaluatorUnits[i] != aUnit)
		{
			SExecutionContext * job = mEvaluatorUnits[i]->stealJob();
			if (job)
			{
				return job;
			}
		}
	}

	return 0;
}

void SchemeEvaluator::markRoots(GarbageCollector * collector)
{
	for (EvaluatorUnit * unit : mEvaluatorUnits)
	{
		unit->markDataRoots(collector);
	}
}

GarbageCollector * SchemeEvaluator::garbageCollector() const
{
	return mGarbageCollector.get();
}

void SchemeEvaluator::safePoint()
{
	mGarbageCollector->safePoint();
}

void SchemeEvaluator::runScheme(const FSchemeNode * aScheme, const FSchemeNode * aInput, int aNumEvaluators)
{
	if (aNumEvaluators <= 0 || aNumEvaluators >= 32)
	{
		std::cerr << "Number of evaluation units is incorrect. Using default value (1).\n";
		aNumEvaluators = 1;
	}

	int evaluatorUnits = aNumEvaluators;

	// Создаем юниты выполнения.
	for (int i = 0; i < evaluatorUnits; i++)
	{
		mEvaluatorUnits.push_back(new EvaluatorUnit(this));
	}

	// Создаем задание и назначем его первому вычислителю.
	GarbageCollector * collector = GarbageCollector::getCollector(evaluatorUnits, this);
	mGarbageCollector.reset(collector);

	SExecutionContext * context = new SExecutionContext();

	FFunctionNode startNode(
		[this, aScheme, aInput, context, collector](SExecutionContext & aCtx)
		{
			if (aInput)
			{
				aInput->execute(aCtx);
			}

			aCtx.advance();

			boost::timer::cpu_timer timer;

			aScheme->execute(aCtx);

			// TEST
			collector->runGc(&aCtx.evaluator()->heap());

			stop();

			std::cout << "\nTime : " << boost::timer::format(timer.elapsed()) << "\n";
		}
	);

	context->Scheme = &startNode;
	mEvaluatorUnits[0]->addJob(context);
    
    // Защита от случая, когда поток завершит вычисления раньше, чем другие будут созданы.
    mStopMutex.lock();

	// Создаем потоки.
	for (int i = 0; i < evaluatorUnits; i++)
	{
		mThreadGroup.create_thread(boost::bind(&EvaluatorUnit::evaluateScheme, mEvaluatorUnits[i]));
	}
    
    mStopMutex.unlock();

	// Ждем завершения вычислений.
	mThreadGroup.join_all();
}

//-----------------------------------------------------------------------------

}} // FPTL::Runtime