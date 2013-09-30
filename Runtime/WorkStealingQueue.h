#ifndef WORK_STEALING_QUEUE_H
#define WORK_STEALING_QUEUE_H

#include <vector>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>

namespace FPTL {
namespace Runtime {

// ������� ��� �������� �������.
// ������� � ������� ������. ����������/�������� ��� ������ ������ - ������������� ��������.
// ������ �� ������ ������ � �����������.
template <typename T>
class WorkStealingQueue
{
public:

	WorkStealingQueue() : mHeadPos(0), mTailPos(0), mSize(32), mQueue(mSize)
	{}

	void push(const T & aElem)
	{
		int tailPos = mTailPos;
		if (tailPos < mSize)
		{
			mQueue[tailPos] = aElem;
			mTailPos = tailPos + 1;
		}
		else
		{
			/// ����������� ������ �������.
			boost::lock_guard<boost::mutex> lock(mAccessMutex);

			int numJobs = mTailPos - mHeadPos;

			if (mHeadPos > 0)
			{
				for (int i = 0; i < numJobs; i++)
				{
					mQueue[i] = mQueue[i + mHeadPos];
				}

				mTailPos = numJobs;
				mHeadPos = 0;
			}

			// ����������� ������ �������.
			mSize *= 2;
			mQueue.resize(mSize);

			// ��������� �������.
			mQueue[mTailPos] = aElem;
			mTailPos++;
		}
	}

	bool pop(T & aElem)
	{
		int tailPos = mTailPos;
		if (tailPos <= mHeadPos)
		{
			return false;
		}

		tailPos--;
		mTailPos = tailPos;

		boost::atomic_thread_fence(boost::memory_order_seq_cst);

		if (tailPos >= mHeadPos)
		{
			aElem = mQueue[tailPos];
			mQueue[tailPos] = nullptr;
			return true;
		}
		else
		{
			boost::lock_guard<boost::mutex> lock(mAccessMutex);

			if (tailPos >= mHeadPos)
			{
				// �����, �������� �������, ����� ���� ��������� � ������ ������� �������.
				aElem = mQueue[tailPos];
				mQueue[tailPos] = nullptr;
				return true;
			}
			else
			{
				// �� ������
				tailPos++;
				mTailPos = tailPos;

				return false;
			}
		}
	}

	bool steal(T & aElem)
	{
		bool result = false;

		if (mAccessMutex.try_lock())
		{
			int headPos = mHeadPos;

			mHeadPos++;

			boost::atomic_thread_fence(boost::memory_order_seq_cst);

			// ���� ������� �� ���� ����� ����� �������, �� ����� ���.
			if (headPos < mTailPos)
			{
				//assert(headPos < mTailPos);
				aElem = mQueue[headPos];
				mQueue[headPos] = nullptr;
				result = true;
			}
			else
			{
				// ���������� �������.
				mHeadPos = headPos;
				result = false;
			}

			mAccessMutex.unlock();
		}

		return result;
	}

private:

	volatile int mHeadPos;
	volatile int mTailPos;
	volatile int mSize;
	std::vector<T> mQueue;
	boost::mutex mAccessMutex;
};

}}

#endif