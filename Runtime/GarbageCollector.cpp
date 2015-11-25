#include "GarbageCollector.h"
#include "CollectedHeap.h"
#include "BlockingQueue.h"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>

#include <boost/timer/timer.hpp>

namespace FPTL {
namespace Runtime {

typedef std::vector<const DataValue *> MarkList;

//-------------------------------------------------------------------------------
class RootMarker : public ObjectMarker
{
public:
	RootMarker(Collectable::Age maxAge)
		: mAliveSize(0),
		mMaxAge(maxAge)
	{}

	virtual bool markAlive(Collectable * object, size_t size)
	{
		if (ObjectMarker::checkAge(object, mMaxAge))
		{
			mAliveSize += size;
			mRoots.push_back(object);
			return true;
		}

		return false;
	}

	virtual void addChild(const DataValue * child)
	{
		mMarkStack.push_back(child);
	}

	std::vector<Collectable *> mRoots;
	MarkList mMarkStack;
	size_t mAliveSize;
	Collectable::Age mMaxAge;
};

//-------------------------------------------------------------------------------
class HeapMarker : public ObjectMarker
{
public:
	HeapMarker()
		: mAliveSize(0)
	{}

	HeapMarker(RootMarker & rootMarker)
		: mAliveSize(rootMarker.mAliveSize),
		mMaxAge(rootMarker.mMaxAge)
	{
		mMarkStack.swap(rootMarker.mMarkStack);
	}

	virtual bool markAlive(Collectable * object, size_t size)
	{
		if (!object->isMarked() && ObjectMarker::checkAge(object, mMaxAge))
		{
			ObjectMarker::setMarked(object, 1);
			mAliveSize += size;
			return true;
		}

		return false;
	}

	virtual void addChild(const DataValue * child)
	{
		mMarkStack.push_back(child);
	}

	size_t aliveSize() const
	{
		return mAliveSize;
	}

	int traceDataRecursively()
	{
		int count = 0;

		while (!mMarkStack.empty())
		{
			const DataValue * val = mMarkStack.back();
			mMarkStack.pop_back();

			val->getOps()->mark(*val, this);
			count++;
		}

		return count;
	}

private:
	size_t mAliveSize;
	MarkList mMarkStack;
	Collectable::Age mMaxAge;
};

//-------------------------------------------------------------------------------
class GarbageCollectorImpl : public GarbageCollector
{
private:
	struct GcJob
	{
		RootMarker marker;
		CollectedHeap::MemList allocated;
		size_t allocatedSize;
		bool fullGc;

		GcJob() :
			marker(Collectable::YOUNG),
			allocatedSize(0),
			fullGc(false)
		{}

		GcJob(bool fullGc)
			: marker(fullGc ? Collectable::OLD : Collectable::YOUNG),
			allocatedSize(0),
			fullGc(fullGc)
		{}
	};

public:
	GarbageCollectorImpl(int numMutatorThreads, DataRootExplorer * rootExplorer, const GcConfig & config)
		: mConfig(config),
		mStopped(0),
		mThreads(numMutatorThreads),
		mStop(0),
		mRootExplorer(rootExplorer),
		mQuit(false),
		mOldGenSize(0),
		mCollectOld(false)
	{
		mCollectorThread.swap(std::thread(std::bind(&GarbageCollectorImpl::collectorThreadLoop, this)));
	}

	virtual ~GarbageCollectorImpl()
	{
		mQuit = true;
		mQueue.quit();
		mCollectorThread.join();
		mAllocated.clear_and_dispose([](Collectable * obj) { delete obj; });
	}

	virtual void runGc()
	{
		// ���������, �� ����� �� ���-������ ������ ������ ������.
		while (!mGcMutex.try_lock())
		{
			safePoint();
		}

		boost::timer::cpu_timer pauseTimer;

		{
			std::unique_lock<std::mutex> lock(mRunMutex);

			// �������� ������� �� ��������� ������ �������.
			mStop.store(1, std::memory_order_release);
			mStopped++;

			// ����, ���� ��� �����������.
			mRunCond.wait(lock, [this] { return mStopped == mThreads; });
			mStopped--;
		}

		GcJob * job = new GcJob(mCollectOld);

		// ��������� �����.
		mRootExplorer->markRoots(&job->marker);

		// ���������� ������ ���������� ������ �� ���� �����.
		for (auto heap : mHeaps)
		{
			job->allocatedSize += heap->heapSize();
			job->allocated.splice(job->allocated.end(), heap->reset());
		}

		// ��������� ������� �� ������ ������.
		mQueue.push(job);

		{
			std::unique_lock<std::mutex> lock(mRunMutex);

			// ������������ ������ ������ �������.
			mStop.store(0, std::memory_order_relaxed);
			mRunCond.notify_all();
		}

		std::cout << "GC Pause. Time: " << boost::timer::format(pauseTimer.elapsed());

		mGcMutex.unlock();
	}

	// ���� ����� ����������� ������� �������� ��� �������� ������������ ������.
	virtual void safePoint()
	{
		if (mStop.load(std::memory_order_acquire) == 1)
		{
			std::unique_lock<std::mutex> lock(mRunMutex);

			mStopped++;
			mRunCond.notify_all();

			mRunCond.wait(lock, [this] { return mStop.load(std::memory_order_relaxed) == 0; });
			mStopped--;
		}
	}

	virtual void registerHeap(CollectedHeap * heap)
	{
		mHeaps.push_back(heap);
		heap->setLimit(mConfig.youngGenSize());
	}
	
private:
	void collectorThreadLoop()
	{
		while (!mQuit)
		{
			auto deqJob = mQueue.pop();

			if (!deqJob.is_initialized())
			{
				break;
			}

			GcJob * job = deqJob.get();

			boost::timer::cpu_timer gcTimer;

			// ��������� �������� �������.
			for (auto object : job->marker.mRoots)
			{
				ObjectMarker::setMarked(object, 1);
			}
			
			HeapMarker marker(job->marker);

			// �������� ��������� �������.
			int count = marker.traceDataRecursively();

			if (job->fullGc)
			{
				job->allocated.splice(job->allocated.end(), mAllocated);
				job->allocatedSize += mOldGenSize;
				mOldGenSize = 0;
				mCollectOld = false;
			}

			// ������� ������.
			job->allocated
				.remove_and_dispose_if([](const Collectable & obj) { 
											return !obj.isMarked();
										}, [](Collectable * obj) {
											delete obj;
										});

			// ���������� �����.
			for (auto & object : job->allocated)
			{
				ObjectMarker::setObjectAge(&object, Collectable::OLD);
				ObjectMarker::setMarked(&object, 0);
			}

			mOldGenSize += marker.aliveSize();
			mAllocated.splice(mAllocated.end(), job->allocated);

			std::cout << "GC time : " << boost::timer::format(gcTimer.elapsed())
				<< "Reclaimed: " << (job->allocatedSize - marker.aliveSize()) / (1024 * 1024) << " MiB. OldGen: "
				<< mOldGenSize / (1024 * 1024) << " MiB \n";

			if (mOldGenSize > 20 * 1024 * 1024)
				mCollectOld = true;

			delete job;
		}
	}

private:
	GcConfig mConfig;

	// ����, ��������������� � ������������� ���������.
	std::atomic<int> mStop;

	// ���������� �������������� �������.
	int mStopped;

	// ���������� ������� ��������.
	int mThreads;

	std::mutex mGcMutex;
	std::mutex mRunMutex;
	std::condition_variable mRunCond;

	DataRootExplorer * mRootExplorer;
	std::vector<CollectedHeap *> mHeaps;
	size_t mOldGenSize;

	// ����� ��������� ������.
	BlockingQueue<GcJob *> mQueue;
	bool mQuit;

	// ��������� ������ ������ ������ ������.
	std::thread mCollectorThread;
	CollectedHeap::MemList mAllocated;

	bool mCollectOld;
};

//-------------------------------------------------------------------------------
GarbageCollector * GarbageCollector::getCollector(int numThreads, DataRootExplorer * rootExplorer, const GcConfig & config)
{
	return new GarbageCollectorImpl(numThreads, rootExplorer, config);
}

}

//-------------------------------------------------------------------------------
void Runtime::ObjectMarker::setObjectAge(Collectable * object, int age)
{
	object->meta.age = static_cast<Collectable::Age>(age);
}

//-------------------------------------------------------------------------------
bool Runtime::ObjectMarker::checkAge(const Collectable * object, int age)
{
	return object->meta.age <= age;
}

void Runtime::ObjectMarker::setMarked(Collectable * object, unsigned int flag)
{
	object->meta.marked = flag;
}

}
