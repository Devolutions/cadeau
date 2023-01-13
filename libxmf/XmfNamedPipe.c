
#include "XmfString.h"

#include "XmfNamedPipe.h"

#define XMF_NAMED_PIPE_BUFFER_SIZE	8192

int XmfNamedPipe_Read(XmfNamedPipe* np_handle, uint8_t* data, size_t size)
{
#ifdef _WIN32
	DWORD cb_read = 0;
	if (!ReadFile((HANDLE) np_handle, data, size, &cb_read, NULL)) {
		return -1;
	}
	return (int) cb_read;
#else
	return -1;
#endif
}

int XmfNamedPipe_Write(XmfNamedPipe* np_handle, const uint8_t* data, size_t size)
{
#ifdef _WIN32
	DWORD cb_write = 0;

	if (!WriteFile((HANDLE)np_handle, (void*)data, (DWORD)size, &cb_write, NULL)) {
		return -1;
	}
	return (int) cb_write;
#else
	return -1;
#endif
}

XmfNamedPipe* XmfNamedPipe_Open(const char* np_name)
{
#ifdef _WIN32
	HANDLE np_handle;
	char filename[1024];

	if (!np_name)
		return NULL;

	sprintf_s(filename, sizeof(filename) - 1, "\\\\.\\pipe\\%s", np_name);

	np_handle = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (np_handle == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	return (XmfNamedPipe*) np_handle;
#else
	return NULL;
#endif
}

XmfNamedPipe* XmfNamedPipe_Create(const char* np_name)
{
#ifdef _WIN32
	HANDLE np_handle;
	char filename[1024];

	if (!np_name)
		return NULL;

	sprintf_s(filename, sizeof(filename) - 1, "\\\\.\\pipe\\%s", np_name);

	np_handle = CreateNamedPipeA(filename,
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		XMF_NAMED_PIPE_BUFFER_SIZE,
		XMF_NAMED_PIPE_BUFFER_SIZE,
		0, NULL);

	if (np_handle == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	return (XmfNamedPipe*) np_handle;
#else
	return NULL;
#endif
}

void XmfNamedPipe_Close(XmfNamedPipe* np_handle)
{
#ifdef _WIN32
	CloseHandle((HANDLE) np_handle);
#else
	
#endif
}
