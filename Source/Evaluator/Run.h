﻿#pragma once

#include <vector>

#include <boost/thread.hpp>
#include <boost/lockfree/stack.hpp>
#include <boost/timer/timer.hpp>


#include "LockFreeWorkStealingQueue.h"
#include "GC/CollectedHeap.h"
#include "GC/GarbageCollector.h"
#include "Utils/FormattedOutput.h"

namespace FPTL
{
	namespace Runtime
	{
		struct SExecutionContext;
		class FSchemeNode;
		class SchemeEvaluator;
		class CollectedHeap;

		//----------------------------------------------------------------------------

		class EvalConfig
		{
		public:
			EvalConfig() :
				mOutput(Utils::FormattedOutput()),
				mNumCores(1),
				mInfo(false),
				mTime(false),
				mProactive(false)
			{}

			void SetOutput(const Utils::FormattedOutput &fo) { mOutput = fo; }

			void SetInfo(const bool state) { mInfo = state; }

			void SetTime(const bool state) { mTime = state; }

			void SetProactive(const bool state) { mProactive = state; }

			void SetNumCores(const size_t numCores) { mNumCores = numCores; }

			const Utils::FormattedOutput& Output() const { return mOutput; }

			bool Info() const { return mInfo; }
			
			bool Time() const { return mTime; }

			bool Proactive() const { return mProactive; }

			size_t NumCores() const { return mNumCores; }

		private:
			Utils::FormattedOutput mOutput;
			size_t mNumCores;
			bool mInfo;
			bool mTime;
			bool mProactive;
		};

		//----------------------------------------------------------------------------------

		// Вычислитель схемы, привязан к конкретному потоку.
		class EvaluatorUnit
		{
		public:
			friend struct SExecutionContext;

			explicit EvaluatorUnit(SchemeEvaluator * aSchemeEvaluator);

			// Основная процедура, в которой производятся вычисления.
			void evaluateScheme();

			// Запуск независимого процесса выполнения задания.
			void addForkJob(SExecutionContext * task);

			// Переместить задачу и все подзадачи в основную очередь.
			void moveToMainOrder(SExecutionContext * movingTask);

			// Ожидание завершения процесса выполнения последнего в очереди задания.
			SExecutionContext* join();

			//Отмена задания, стоящего в списке ожидающих выполнения на позиции pos с конца.
			void cancelFromPendingEnd(size_t pos = 1);

			//Отмена задания и всех его дочерних заданий.
			void cancel(SExecutionContext *cancelingTask);

			// Проверка на необходимость выполнения системного действия (сборка мусора и т.п.).
			void safePoint();

			// Добавление нового задания в очередь.
			// Вызывается только из потока, к которому привязан или до его создания.
			void addJob(SExecutionContext * aContext);

			// Получение работы для другого потока.
			// Вызывается из любого потока.
			SExecutionContext * stealJob();

			// Получение работы из очереди упреждающих задач для другого потока.
			// Вызывается из любого потока.
			SExecutionContext * stealProactiveJob();

			// Поиск и выполнение задания.
			void schedule();

			CollectedHeap& heap() const;

			// Трассировка корней стека.
			void markDataRoots(ObjectMarker * marker);

			void pushTask(SExecutionContext * task);
			void popTask();

			boost::timer::cpu_times GetWorkTime() const
			{
				return mWorkTimer.elapsed();
			}

		private:
			// Задачи, данные из которых ожидаются.
			// Они могут как всё ещё находиться в очереди или быть на выполнении, так и быть на 
			std::vector<SExecutionContext *> pendingTasks;
			// Выполняемые задачи, в том числе ожидающие данных. 
			std::vector<SExecutionContext *> runningTasks;

			size_t mJobsCompleted;
			size_t mProactiveJobsCompleted;
			size_t mJobsCreated;
			size_t mProactiveJobsCreated;
			size_t mJobsStealed;
			size_t mProactiveJobsStealed;
			size_t mProactiveJobsMoved;
			size_t mProactiveJobsCanceled;
			boost::timer::cpu_timer mWorkTimer;
			// ToDo: Попробовать заменить на boost::lockfree::stack
			LockFreeWorkStealingQueue<SExecutionContext *> mJobQueue;
			boost::lockfree::stack<SExecutionContext *> mProactiveJobQueue;
			SchemeEvaluator * mEvaluator;
			mutable CollectedHeap mHeap;
			GarbageCollector * mCollector;
		};

		//----------------------------------------------------------------------------------

		// Производит вычисления программы, заданной функциональной схемой.
		// Порождает, владеет и управляет всеми потоками.
		class SchemeEvaluator : public DataRootExplorer
		{
		public:
			SchemeEvaluator();

			void setGcConfig(const GcConfig & config) { mGcConfig = config; }

			void setEvalConfig(const EvalConfig & config) { mEvalConfig = config; }

			EvalConfig getEvalConfig() const { return mEvalConfig; }

			void run(SExecutionContext & program);

			// Прервать вычисления.
			void abort();

			// Вычисления окончены.
			void stop();

			// Взять задание у других вычислителей. Возвращает нулевой указатель, если не получилось.
			SExecutionContext * findJob(const EvaluatorUnit * aUnit);

			// Взять упреждающую задачу у других вычислителей. Возвращает 0, если не получилось.
			SExecutionContext * findProactiveJob(const EvaluatorUnit * aUnit);

			void markRoots(ObjectMarker * marker) override;

			GarbageCollector * garbageCollector() const;

			bool WasErrors() const
			{
				return mWasErrors;
			}

			void StopRunTime()
			{
				mRunTimer.stop();
			}

			boost::timer::cpu_times GetRunTime() const
			{
				return mRunTimer.elapsed();
			}

		private:
			std::vector<EvaluatorUnit *> mEvaluatorUnits;
			boost::thread_group mThreadGroup;
			boost::mutex mStopMutex;
			std::unique_ptr<GarbageCollector> mGarbageCollector;
			GcConfig mGcConfig;
			EvalConfig mEvalConfig;
			boost::timer::cpu_timer mRunTimer;
			bool mWasErrors = false;
		};

	}
}
