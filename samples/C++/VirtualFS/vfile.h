#pragma once
#include "defines.h"
#include "H5FDBlock.h"
#include "Ranges/IntervalTree.h"

using namespace XDX::Objects;
namespace XDX
{
	typedef hid_t  RawHandle;
	typedef HANDLE XHandle;
	struct LockInfo
	{
		LockInfo():
			isExclusive(false), UserHandle(INVALID_HANDLE_VALUE){}
		bool    isExclusive;
		XHandle UserHandle;
	};
	typedef IntervalTree<file_offset_t, LockInfo> Ranges;
	struct RealHandle
	{
		RealHandle():
			rawHandle(-1), isFile(false), uReaders(0), uWriters(0), fShareMode(0){}
		RawHandle    rawHandle;
		bool         isFile;
		uint32_t     uReaders;
		uint32_t     uWriters;
		uint32_t     fShareMode;
		std::wstring wsPath;
		Ranges       rangeLocks;
	};
	struct UserHandle
	{
		UserHandle():
			oCursor(0), uCreatedBy(0), fAccessMode(0), hRealHandle(-1){}
		file_offset_t oCursor;
		uint64_t      uCreatedBy;
		uint32_t      fAccessMode;
		RawHandle     hRealHandle;
	};
	typedef std::unordered_map<RawHandle, RealHandle> RealHandlesT;
	typedef std::unordered_map<XHandle, UserHandle> UserHandlesT;
	typedef std::unordered_map<std::wstring, RawHandle> NamedHandlesT;

}
