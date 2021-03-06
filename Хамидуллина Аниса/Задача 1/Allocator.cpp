#include "pch.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include <tchar.h>
#include <map>
#include <ctime>
#include <vector>

using namespace std;

struct Buffer {
	size_t size;
	size_t offset;
	void* startPointer;
	Buffer* next;
	int ptrCount;
};

template<class T> class MyAllocator {
public:
	typedef size_t size_type;
	typedef T* pointer;
	typedef T value_type;
	template <class U> struct rebind {
		typedef MyAllocator<U> other;
	};
	Buffer* headBuffer;
	Buffer* currentBuffer;
	size_t bufferSize;

	MyAllocator() 
	{
		headBuffer = nullptr;
		currentBuffer = nullptr;
		bufferSize = 10 * 1024;
	};
	~MyAllocator() {
		if (headBuffer->ptrCount == 0) {
			Buffer* buf = headBuffer;
			while (buf != nullptr) {
				Buffer* oldBuf = buf;
				buf = buf->next;
				free(oldBuf);
			}
		}
		headBuffer->ptrCount--;
	}
	template<class U>
	MyAllocator(MyAllocator<U>& other) noexcept 
	{
		headBuffer = other.headBuffer;
		currentBuffer = other.currentBuffer;
		if (headBuffer != nullptr) {
			headBuffer->ptrCount++;
		}
	};
	pointer allocate(size_type n) 
	{
		if (headBuffer == nullptr) {
			headBuffer = createBuffer();
			currentBuffer = headBuffer;
		}
		if (currentBuffer->size - currentBuffer->offset < n * sizeof(value_type)) {
			currentBuffer->next = createBuffer();
			currentBuffer = currentBuffer->next;
		}
		pointer ptr = reinterpret_cast<pointer>
			(reinterpret_cast<char*>(currentBuffer->startPointer) + currentBuffer->offset) ;
		currentBuffer->offset += n * sizeof(T);
		return ptr;
	};

	Buffer* createBuffer() {
		void* ptr = malloc(bufferSize + sizeof(Buffer));
		Buffer* buf = reinterpret_cast<Buffer*>(ptr);
		buf->size = bufferSize;
		buf->next = nullptr;
		buf->offset = 0;
		buf->ptrCount = 0;
		buf->startPointer = reinterpret_cast<void*>(reinterpret_cast<char*>(ptr) + sizeof(Buffer));
		return buf;
	}
	void deallocate(pointer p, size_type n) 
	{ 
	};
};

struct Comparator : public binary_function<char*, char*, bool>
{
	bool operator()(const char* p1, const char* p2) const
	{
		while (true) {
			if (isalpha(*p1) and isalpha(*p2)) {
				auto a = tolower(*p1);
				auto b = tolower(*p2);
				if (a == b) {
					p1++;
					p2++;
				}
				else
					return a < b;
			}
			else
				return isalpha(*p2);
		}
	}
};

int main(int argc, char* argv[])
{
	HANDLE bFile;
	const wchar_t* fName = L"input.txt";
	bFile = CreateFile(fName, 
		GENERIC_READ, 
		FILE_SHARE_READ, 
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);
	LARGE_INTEGER len_li;
	int result = GetFileSizeEx(bFile, &len_li);
	LONGLONG size = len_li.QuadPart;
	cout << "Size of file: " << size << "\n";
	char* Text;
	DWORD numberOfBytesRead;
	Text = new char[size + 1];
	ReadFile(bFile, Text, size, &numberOfBytesRead, NULL);
	Text[size] = 0;
	CloseHandle(bFile);

	map<char*, size_t, Comparator, MyAllocator<pair<const char*, size_t>>> firstMap;
	unsigned int startTime = clock();
	char* start = nullptr;
	for (char* p = Text - 1; *p;) {
		p++;
		if (*p && isalpha(*p)) {
			if (start == nullptr) {
				start = p;
			}
		}
		else {
			if (start != nullptr) {
				firstMap[start]++;
				start = nullptr;
			}
		}
	}
	unsigned int endTime = clock();
	cout << "Time with MyAllocator " << endTime - startTime << "\n";

	startTime = clock();
	map<string, size_t> secondMap;
	vector<char> vect;

	for (char* p = Text - 1; *p;) {
		p++;
		if (*p && isalpha(*p)) {
			vect.push_back(tolower(*p));
		}
		else {
			if (!vect.empty()) {
				string word(vect.begin(), vect.end());
				secondMap[word]++;
				vect.clear();
			}
		}
	}

	endTime = clock();
	cout << "Time with std::allocator: " << endTime - startTime << "\n";

	multimap<size_t, char*> m;

	for (auto it1 = firstMap.begin(); it1 != firstMap.end(); ++it1) {
		m.insert(make_pair(it1->second, it1->first));
	}
	cout << "Result:" << "\n\n";

	for (auto it2 = m.rbegin(); it2 != m.rend(); ++it2) {
		cout << it2->first << "\t";
		auto p = it2->second;
		while (*p && isalpha(*p)) {
			char a = tolower(*p);
			cout << a;
			p++;
		}
		cout << "\n";
	}
}