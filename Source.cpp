#include <cstdlib>
#include <algorithm>
#include <random>

#include <iostream>
#include <memory>

#define NOMINMAX
#include <Windows.h>

#define bassert(test) ( !!(test) ? (void)0 : ((void)printf("-- Assertion failed at line %d: %s\n", __LINE__, ##test), __debugbreak()) )

const int NOT_FOUND = -1;
const int NOT_SEARCHED = -2;

// Utility functions not needed for solution
namespace {
	/// Get current "time" in nano seconds
	uint64_t timer_nsec() {
		static LARGE_INTEGER freq;
		static BOOL once = QueryPerformanceFrequency(&freq);

		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);

		return 1000000000ULL * t.QuadPart / freq.QuadPart;
	}

	/// Verify if previous search produced correct results
	/// @param hayStack - the input data that will be searched in
	/// @param size - the size of @hayStack
	/// @param needles - the values that will be searched
	/// @param indices - the indices of the needles (or -1 if the needle is not found)
	/// @param needleCount - number of items in needles and indices
	/// Return the first index @c where find(@hayStack, @needles[@c]) != @indices[@c], or -1 if all indices are correct
	int verify(const int* hayStack, const int size, int* const needles, int* const indices, const int needleCount) {
		const int* end = hayStack + size;
		for (int c = 0; c < needleCount; c++) {
			const int value = needles[c];
			const int* pos = std::lower_bound(hayStack, end, value);
			const int idx = std::distance(hayStack, pos);

			if (idx == size || hayStack[idx] != value) {
				bassert(indices[c] == NOT_FOUND);
				if (indices[c] != NOT_FOUND) {
					return c;
				}
			}
			else {
				bassert(indices[c] == idx);
				if (indices[c] != idx) {
					return c;
				}
			}
		}
		return -1;
	}

	/// Allocate aligned for @count objects of type T, does not perform initialization
	/// @param count - the number of objects
	/// @param unaligned [out] - stores the un-aligned pointer, used to call free
	/// @return pointer to the memory or nullptr
	template <typename T>
	T* alignedAlloc(size_t count, void*& unaligned) {
		const size_t bytes = count * sizeof(T);
		unaligned = malloc(bytes + 63);
		if (!unaligned) {
			return nullptr;
		}
		T* const aligned = reinterpret_cast<T*>(uintptr_t(unaligned) + 63 & -64);
		return aligned;
	}

	/// Initialize the haystack and the needles
	/// @param hayStack - the input data that will be searched in, can have duplicate numbers
	/// @param count - the size of @hayStack
	/// @param needles - the values that will be searched
	/// @param needleCount - number of items in needles and indices, can have duplicate numbers
	void initData(int* hayStack, int count, int* needles, int needleCount) {
		std::mt19937 rng(42);
		std::uniform_int<int> dataDist(0, count << 1);
		std::uniform_int<int> queryDist(0, count << 2);

		for (int c = 0; c < count; c++) {
			hayStack[c] = dataDist(rng);
		}

		for (int r = 0; r < needleCount; r++) {
			needles[r] = queryDist(rng);
		}
	}

	/// Reset all indices to NOT_SEARCHED
	void clearIndices(int* indices, int needleCount) {
		memset(indices, NOT_SEARCHED, needleCount);
	}

	struct FreeDeleter {
		FreeDeleter() = default;
		void operator()(void* ptr) const {
			free(ptr);
		}
	};
}

/// Stack allocator with predefined max size
/// The total memory is 64 byte aligned, all but the first allocation are not guaranteed to be algigned
/// Can only free all the allocations at once
struct StackAllocator {
	StackAllocator(char* ptr, int bytes)
		: totalBytes(bytes)
		, data(ptr)
	{}

	/// Allocate memory for @count T objects
	/// Does *NOT* call constructors
	/// @param count - the number of objects needed
	/// @return pointer to the allocated memory or nullptr
	template <typename T>
	T* alloc(int count) {
		const int size = count * sizeof(T);
		if (idx + size > totalBytes) {
			return nullptr;
		}
		char* start = data + idx;
		idx += size;
		return reinterpret_cast<T*>(start);
	}

	/// De-allocate all the memory previously allocated with @alloc
	void freeAll() {
		idx = 0;
	}

	/// Get the max number of bytes that can be allocated by the allocator
	int maxBytes() const {
		return totalBytes;
	}

	/// Get the free space that can still be allocated, same as maxBytes before any allocations
	int freeBytes() const {
		return totalBytes - idx;
	}

	StackAllocator(const StackAllocator&) = delete;
	StackAllocator& operator=(const StackAllocator&) = delete;
private:
	const int totalBytes;
	int idx = 0;
	char* data = nullptr;
	void* toFree = nullptr;
};

/// Binary search implemented to return same result as std::lower_bound
/// When there are multiple values of the searched, it will return index of the first one
/// When the searched value is not found, it will return -1
/// @param hayStack - the input data that will be searched in
/// @param size - the size of @hayStack
/// @param needles - the values that will be searched
/// @param indices - the indices of the needles (or -1 if the needle is not found)
/// @param needleCount - number of items in needles and indices
static void binarySearch(const int* hayStack, const int size, const int* const needles, int* indices, const int needleCount) {
	for (int c = 0; c < needleCount; c++) {
		const int value = needles[c];

		int left = 0;
		int count = size;

		while (count > 0) {
			const int half = count / 2;

			if (hayStack[left + half] < value) {
				left = left + half + 1;
				count -= half + 1;
			}
			else {
				count = half;
			}
		}

		if (hayStack[left] == value) {
			indices[c] = left;
		}
		else {
			indices[c] = -1;
		}
	}
}


/// Implement some search algorithm and optimize it so it is faster than @binarySearch above
/// When the searched value is not found, it will return -1
/// @param hayStack - the input data that will be searched in
/// @param size - the size of @hayStack
/// @param needles - the values that will be searched
/// @param indices - the indices of the needles (or -1 if the needle is not found)
/// @param needleCount - number of items in needles and indices
/// @param allocator - all allocations must be done trough this allocator, it can be queried for max allowed allocation size
static void betterSearch(const int* hayStack, const int size, const int* const needles, int* indices, const int needleCount, StackAllocator& allocator) {
	for (int q = 0; q < needleCount; q++) {
		const int value = needles[q];

		if (hayStack[0] == value)
		{
			indices[q] = 0;
		}
		else {
			int i = 1;
			while (i < size && hayStack[i] <= value)
			{
				i = i << 1;
			}

			int left = i >> 1;
			int	count = std::min(size - 1, i) - left + 1;

			while (count > 0) {
				int half = count >> 1;
				int midValue = hayStack[left + half];

				if (midValue < value) {
					left = left + half + 1;
					count -= half + 1;
				}
				else
				{
					count = half;
				}
			}
			indices[q] = (hayStack[left] == value) ? left : -1;
		}
	}
}

static void betterSearch2(const int* hayStack, const int size, const int* const needles, int* indices, const int needleCount, StackAllocator& allocator) {
	for (int q = 0; q < needleCount; q++) {
		const int value = needles[q];

		int left = 0;
		int right = size - 1;
		int mid = -1;
		if ((value < hayStack[left]) || (value > hayStack[right])) {
			indices[q] = -1;
		}
		else {
			while (hayStack[left] < hayStack[right]) {
				mid = left + ((value - hayStack[left]) * (right - left) / (hayStack[right] - hayStack[left]));

				if (hayStack[mid] < value)
				{
					left = mid + 1;
				}
				else
				{
					right = mid;
				}
			}
			indices[q] = (hayStack[left] == value) ? left : -1;
		}
	}
}

int main(int argc, char* argv[]) {
	printf("Initialization ... ");
	const int count = 1 << 24;
	const int queryCount = 1 << 14;

	void* toFree = nullptr;
	int* hayStack = alignedAlloc<int>(count, toFree);
	std::unique_ptr<void, FreeDeleter> hayStackFree(toFree, FreeDeleter());

	toFree = nullptr;
	int* needles = alignedAlloc<int>(queryCount, toFree);
	std::unique_ptr<void, FreeDeleter> needlesFree(toFree, FreeDeleter());

	toFree = nullptr;
	int* indices = alignedAlloc<int>(queryCount, toFree);
	std::unique_ptr<void, FreeDeleter> indicesFree(toFree, FreeDeleter());

	const int heapSize = 1 << 13;
	char* heap = alignedAlloc<char>(heapSize, toFree);
	std::unique_ptr<void, FreeDeleter> allocatorData(toFree, FreeDeleter());

	if (!hayStack || !needles || !indices || !heap) {
		printf("Failed to allocate memory for test\n");
		return -1;
	}

	StackAllocator allocator(heap, heapSize);
	initData(hayStack, count, needles, queryCount);
	std::sort(hayStack, hayStack + count);

	printf("Done\n");
	printf("Correctness tests... ");
	{
		clearIndices(indices, queryCount);
		betterSearch(hayStack, count, needles, indices, queryCount, allocator);
		if (verify(hayStack, count, needles, indices, queryCount) != -1) {
			printf("Failed to verify base betterSearch!\n");
			return -1;
		}

		clearIndices(indices, queryCount);
		binarySearch(hayStack, count, needles, indices, queryCount);
		if (verify(hayStack, count, needles, indices, queryCount) != -1) {
			printf("Failed to verify base binarySearch!\n");
			return -1;
		}

		clearIndices(indices, queryCount);
	}
	printf("Done\n");

	printf("Speed tests...\n");
	const int testRepeat = 1 << 10;

	uint64_t t0;
	uint64_t t1;

	// Time the binary search and take average of the runs
	{
		t0 = timer_nsec();
		for (int test = 0; test < testRepeat; ++test) {
			binarySearch(hayStack, count, needles, indices, queryCount);
		}
		t1 = timer_nsec();
	}

	const double totalBinary = (double(t1 - t0) * 1e-9) / testRepeat;
	printf("binarySearch time %f\n", totalBinary);

	// Time the better search and take average of the runs
	{
		t0 = timer_nsec();
		for (int test = 0; test < testRepeat; ++test) {
			betterSearch(hayStack, count, needles, indices, queryCount, allocator);
		}
		t1 = timer_nsec();
	}

	const double totalBetter = (double(t1 - t0) * 1e-9) / testRepeat;
	printf("betterSearch time %f\n", totalBetter);
	if (totalBetter < totalBinary) {
		printf("Great success!");
	}

	return 0;
}