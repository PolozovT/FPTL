﻿#include <assert.h>

#include <iostream>
#include <unordered_set>
#include <stack>
#include <mutex>
#include <string>
#include <boost/timer/timer.hpp>
#include <boost/chrono.hpp>

#include "Run.h"
#include "FScheme.h"
#include "WorkStealingQueue.h"

#undef max

namespace FPTL { namespace Runtime {

//-----------------------------------------------------------------------------
SExecutionContext::SExecutionContext()
		: Scheme(nullptr),
		  Parent(nullptr),
		  //id(0),
		  Ready(0),
		  Working(0),
		  Anticipation(0),
		  NewAnticipationLevel(0),
		  Canceled(0),
	      endPtr(nullptr),
		  argPos(0),
		  arity(0),
		  argNum(0),
		  mEvaluatorUnit(nullptr)
	{
		stack.reserve(10);
		controlStack.reserve(10);
	}

bool SExecutionContext::isReady() const
{
	return Ready.load(std::memory_order_acquire) == 1;
}

// Метод старого eveluator'а.
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

// Метод старого eveluator'а.
void SExecutionContext::run(EvaluatorUnit * aEvaluatorUnit)
{
	assert(!Ready.load(std::memory_order_acquire));
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

void SExecutionContext::unwind(size_t aArgPosOld, int aArity, size_t aPos)
{;
	for (int i = 0; i < arity; ++i)
	{
		stack[aPos + i] = stack[stack.size() - arity + i];
	}

	argPos = aArgPosOld;
	stack.resize(aPos + arity);
	arity += aArity;
}

void SExecutionContext::join()
{
	auto joined = mEvaluatorUnit->join();
	if (!joined->Canceled)
		// Копируем результат.
		for (int i = 0; i < joined->arity; ++i)
		{
			push(joined->stack.at(joined->stack.size() - joined->arity + i));
		}

	delete joined;
}

//-----------------------------------------------------------------------------
EvaluatorUnit::EvaluatorUnit(SchemeEvaluator * aSchemeEvaluator, const bool disableAnt)
	: mJobsCompleted(0),
	  mAnticipationJobsCompleted(0),
	  mJobsCreated(0),
	  mAnticipationJobsCreated(0),
	  mJobsStealed(0),
	  mAnticipationJobsStealed(0),
	  mAnticipationJobsMoved(0),
	  mAnticipationJobsCanceled(0),
	  mEvaluator(aSchemeEvaluator),
	  mHeap(aSchemeEvaluator->garbageCollector()),
	  mCollector(aSchemeEvaluator->garbageCollector()),
	  disableAnticipatory(disableAnt)
{
}

void EvaluatorUnit::addJob(SExecutionContext * aContext)
{
	mJobsCreated++;
	mJobQueue.push(aContext);
}

SExecutionContext * EvaluatorUnit::stealJob()
{
	SExecutionContext * elem = nullptr;
	mJobQueue.steal(elem);
	return elem;
}

SExecutionContext * EvaluatorUnit::stealAnticipationJob()
{
	SExecutionContext * elem = nullptr;
	mAnticipationJobQueue.pop(elem);
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
	ss << "\n\nThread ID = " << boost::this_thread::get_id() << ". Jobs compleated: " << mJobsCompleted << ", stealed: " << mJobsStealed << ".";
	if (!disableAnticipatory)
		ss << "\nAnticipation jobs created : " << mAnticipationJobsCreated << ", compleated : " << mAnticipationJobsCompleted << ", stealed : " << mAnticipationJobsStealed << ", moved : " << mAnticipationJobsMoved << ", canceled : " << mAnticipationJobsCanceled << ".";
	std::cout << ss.str();
}

void EvaluatorUnit::addForkJob(SExecutionContext * task)
{
	pendingTasks.push_back(task);
	if (task->Anticipation.load(std::memory_order_acquire))
	{
		mAnticipationJobQueue.push(task);
		mAnticipationJobsCreated++;
		/*std::stringstream ss;
		ss << "Job ant " << task->id << " created.\n";
		std::cout << ss.str();*/
	}
	else
	{
		addJob(task);
		/*std::stringstream ss;
		ss << "Job " << task->id << " created.\n";
		std::cout << ss.str();*/
	}
}

SExecutionContext *EvaluatorUnit::join()
{
	// Перед возможным выполнением новой задачи проверяем, не запланированна ли сборка мусора.
	// Иначе может получаться ситуация, когда 1 поток пораждает задания и сразу же их берёт на выполнение,
	// а все остальные ожидают сборки мусора.
	safePoint();

	SExecutionContext * joinTask = pendingTasks.back();

	if (joinTask->Anticipation.load(std::memory_order_acquire) && !joinTask->Parent->Anticipation.load(std::memory_order_acquire))
		moveToMainOrder(joinTask);
	
	while (!joinTask->Ready.load(std::memory_order_acquire))
	{
		schedule();
	}

	joinTask->Parent->Childs.erase(joinTask);
	pendingTasks.pop_back();
	return joinTask;
}

void EvaluatorUnit::moveToMainOrder(SExecutionContext * movingTask)
{
	if (movingTask->Parent->Anticipation.load(std::memory_order_acquire))
	{
		movingTask->Anticipation.store(0, std::memory_order_release);
		if (!movingTask->Ready && !movingTask->Working)
		{
			mJobQueue.push(movingTask);
			mAnticipationJobsMoved++;
			//std::cout << "\nTask moved.\n";
		}
		for (SExecutionContext * child : movingTask->Childs)
		{
			if (!child->NewAnticipationLevel)
				moveToMainOrder(child);
		}
	}
	movingTask->NewAnticipationLevel.store(0, std::memory_order_release);
}

void EvaluatorUnit::cancelFromPendingEnd(const int backPos)
{
	static std::mutex outputMutex;
	SExecutionContext * cancelTask = pendingTasks.at(pendingTasks.size() - backPos);
	cancelTask->Parent->Childs.erase(cancelTask);
	outputMutex.lock();
	cancel(cancelTask);
	outputMutex.unlock();
	//Убираем из очереди ожидающих выполнения задач.
	pendingTasks.erase(pendingTasks.end() - backPos);
}

void EvaluatorUnit::cancel(SExecutionContext * cancelingTask)
{
	/*std::stringstream ss;
	ss << "Job " << cancelingTask->id << " start cancel.\n";
	std::cout << ss.str();*/
	cancelingTask->Parent->Childs.erase(cancelingTask);
	cancelingTask->Canceled = 1;
	if (!cancelingTask->Ready)
	{	// Если задание ещё не выполнено, выставляем флаг готовности, чтобы никто не начал выполнение.
		cancelingTask->Ready = 1;
		// Если уже выполняется - запускаем процесс остановки.
		if (cancelingTask->Working.load(std::memory_order_acquire))
		{
			cancelingTask->cancel();
		}
		mAnticipationJobsCanceled++;
		/*std::stringstream ss;
		ss << "Job " << cancelingTask->id << " canceled.\n";
		std::cout << ss.str();*/
	}
	for (SExecutionContext * child : cancelingTask->Childs)
	{
		cancel(child);
	}
}

void EvaluatorUnit::safePoint() const
{
	mCollector->safePoint();
}

void EvaluatorUnit::schedule()
{
	SExecutionContext * context = nullptr;

	// Берем задание из своей очереди.
	if (mJobQueue.pop(context))
	{
		context->run(this);
		mJobsCompleted++;
		// Выполнили задание - проверяем не запланированна ли сборка мусора.
		safePoint();
		return;
	}

	// Если это не удалось, ищем у кого-нибудь другого.
	context = mEvaluator->findJob(this);	
	if (context)
	{
		context->run(this);
		mJobsStealed++;
		mJobsCompleted++;
		// Выполнили задание - проверяем не запланированна ли сборка мусора.
		safePoint();
		return;
	}

	if (!disableAnticipatory)
	{
		// Если не нашли, берём задание из своей упреждающей очереди и выполняем, если оно не отменено.
		if (mAnticipationJobQueue.pop(context) && !context->Ready.load(std::memory_order_acquire) && context->Anticipation.load(std::memory_order_acquire))
		{
			context->Working.store(1, std::memory_order_release);
			/*std::stringstream ss;
			ss << "Job " << context->id << " started.\n";
			std::cout << ss.str();*/
			context->run(this);
			mAnticipationJobsCompleted++;
			/*std::stringstream s;
			s << "Job " << context->id << " compleated.\n";
			std::cout << s.str();*/
			// Выполнили задание - проверяем не запланированна ли сборка мусора.
			safePoint();
			return;
		}

		// Если не удалось, ищем упреждающую задачу у кого-нибудь другого и выполняем, если оно отменено.
		context = mEvaluator->findAnticipationJob(this);
		if (context && !context->Ready.load(std::memory_order_acquire) && context->Anticipation.load(std::memory_order_acquire))
		{
			context->Working.store(1, std::memory_order_release);
			/*std::stringstream ss;
			ss << "Job " << context->id << " started.\n";
			std::cout << ss.str();*/
			context->run(this);
			mAnticipationJobsStealed++;
			mAnticipationJobsCompleted++;
			/*std::stringstream s;
			s << "Job " << context->id << " compleated.\n";
			std::cout << s.str();*/
			// Выполнили задание - проверяем не запланированна ли сборка мусора.
			safePoint();
			return;
		}
	}

	// Если заданий нет - приостанавливаем поток.
	boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
	safePoint();
}

CollectedHeap & EvaluatorUnit::heap() const
{
	return mHeap;
}

void EvaluatorUnit::markDataRoots(ObjectMarker * marker)
{
	// Помечаем корни у всех заданий, взятых на выполнение текщим юнитом.
	for (SExecutionContext * ctx : runningTasks)
	{
		for (auto & data : ctx->stack)
		{
			data.getOps()->mark(data, marker);
		}
	}

	// Помечаем корни у всех форкнутых заданий, которые были выполнены.
	for (SExecutionContext * ctx : pendingTasks)
	{
		if (ctx->isReady())
		{
			for (auto & data : ctx->stack)
			{
				data.getOps()->mark(data, marker);
			}
		}
	}
}

void EvaluatorUnit::pushTask(SExecutionContext * task)
{
	runningTasks.push_back(task);
}

void EvaluatorUnit::popTask()
{
	runningTasks.pop_back();
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
	return nullptr;
}

SExecutionContext * SchemeEvaluator::findAnticipationJob(const EvaluatorUnit * aUnit)
{
	for (size_t i = 0; i < mEvaluatorUnits.size(); i++)
	{
		if (mEvaluatorUnits[i] != aUnit)
		{
			SExecutionContext * job = mEvaluatorUnits[i]->stealAnticipationJob();
			if (job)
			{
				return job;
			}
		}
	}
	return nullptr;
}

void SchemeEvaluator::markRoots(ObjectMarker * marker)
{
	for (EvaluatorUnit * unit : mEvaluatorUnits)
	{
		unit->markDataRoots(marker);
	}
}

GarbageCollector * SchemeEvaluator::garbageCollector() const
{
	return mGarbageCollector.get();
}

// Метод старого eveluator'а.
void SchemeEvaluator::runScheme(const FSchemeNode * aScheme, const FSchemeNode * aInput, int aNumEvaluators)
{
	if (aNumEvaluators >= 32)
	{
		aNumEvaluators = 32;
		std::cerr << "Too many evaluators. Using default " << aNumEvaluators << "\n";
	}

	const int evaluatorUnits = aNumEvaluators;

	GarbageCollector * collector = GarbageCollector::getCollector(evaluatorUnits, this, mGcConfig);
	mGarbageCollector.reset(collector);

	// Создаем юниты выполнения.
	for (int i = 0; i < evaluatorUnits; i++)
	{
		mEvaluatorUnits.push_back(new EvaluatorUnit(this, true));
	}

	// Создаем задание и назначем его первому вычислителю.
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

			collector->runGc();
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

struct ControlContext : SExecutionContext
{
	ControlContext(SExecutionContext * target, SchemeEvaluator * evaluator)
		: mTarget(target),
		mEvaluator(evaluator)
	{}

	void run(EvaluatorUnit * evaluatorUnit) override
	{
		boost::timer::cpu_timer timer;

		mTarget->run(evaluatorUnit);
		mEvaluator->stop();

		elapsed_times = timer.elapsed();
	}

	boost::timer::cpu_times getWorkTime() const
	{
		return elapsed_times;
	}

private:
	SExecutionContext * mTarget;
	SchemeEvaluator * mEvaluator;
	boost::timer::cpu_times elapsed_times;
};

void SchemeEvaluator::run(SExecutionContext & program, const int numEvaluators, const bool disableAnt)
{
	GarbageCollector * collector = GarbageCollector::getCollector(numEvaluators, this, mGcConfig);
	mGarbageCollector.reset(collector);

	// Создаем юниты выполнения.
	for (int i = 0; i < numEvaluators; i++)
	{
		mEvaluatorUnits.push_back(new EvaluatorUnit(this, disableAnt));
	}

	std::cout.precision(15);
	ControlContext controlContext(&program, this);

	// Добавляем задание в очередь к первому потоку.
	mEvaluatorUnits[0]->addJob(&controlContext);

	// Защита от случая, когда поток завершит вычисления раньше, чем другие будут созданы.
	mStopMutex.lock();

	// Создаем потоки.
	for (size_t i = 0; i < numEvaluators; ++i)
	{
		mThreadGroup.create_thread(boost::bind(&EvaluatorUnit::evaluateScheme, mEvaluatorUnits[i]));
	}
	mStopMutex.unlock();

	mThreadGroup.join_all();

	std::for_each(mEvaluatorUnits.begin(), mEvaluatorUnits.end(), [](auto unit) { delete unit; });
	mEvaluatorUnits.clear();

	std::cout << "\n\nTime : " << boost::timer::format(controlContext.getWorkTime(), 3, "%ws\n");
}

//-----------------------------------------------------------------------------

}} // FPTL::Runtime