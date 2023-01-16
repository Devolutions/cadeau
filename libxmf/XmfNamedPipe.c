
#include "XmfString.h"

#include "XmfNamedPipe.h"

#include <fcntl.h>
#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif

#define XMF_NAMED_PIPE_BUFFER_SIZE    8192

int XmfNamedPipe_Read(XmfNamedPipe* np_handle, uint8_t* data, size_t size)
{
#ifdef _WIN32
    DWORD cb_read = 0;
    
    if (!ReadFile((HANDLE) np_handle, data, size, &cb_read, NULL)) {
        return -1;
    }

    return (int) cb_read;
#else
    int status;
    int pipe_fd = (int) (size_t) np_handle;

    do
    {
        status = read(pipe_fd, data, size);
    } while ((status < 0) && (errno == EINTR));

    if (status < 0) {
        if (errno == EWOULDBLOCK) {
            status = 0;
        }
        return -1;
    }

    return status;
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
    int status;
    int pipe_fd = (int) (size_t) np_handle;

    do
    {
        status = write(pipe_fd, data, size);
    } while ((status < 0) && (errno == EINTR));

    if (status < 0) {
        if (errno == EWOULDBLOCK) {
            status = 0;
        }
        return -1;
    }

    return status;
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
    int status;
    int pipe_fd;
    char filename[1024];
    struct sockaddr_un addr = { 0 };

    if (!np_name)
        return NULL;

    sprintf_s(filename, sizeof(filename) - 1, "/tmp/.pipe-%s", np_name);

    pipe_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (pipe_fd == -1)
    {
        return NULL;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, filename, sizeof(addr.sun_path) - 1);

    status = connect(pipe_fd, (struct sockaddr*)&addr, sizeof(addr));

    if (status != 0)
    {
        close(pipe_fd);
        return NULL;
    }

    return (XmfNamedPipe*) (size_t) pipe_fd;
#endif
}

XmfNamedPipe* XmfNamedPipe_Create(const char* np_name, int max_clients)
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
        max_clients,
        XMF_NAMED_PIPE_BUFFER_SIZE,
        XMF_NAMED_PIPE_BUFFER_SIZE,
        0, NULL);

    if (np_handle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    return (XmfNamedPipe*) np_handle;
#else
    int status;
    int pipe_fd;
    char filename[1024];
    struct sockaddr_un addr = { 0 };

    if (!np_name)
        return NULL;

    sprintf_s(filename, sizeof(filename) - 1, "/tmp/.pipe-%s", np_name);

    pipe_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (pipe_fd == -1)
    {
        return NULL;
    }

    fcntl(pipe_fd, F_SETFL, O_NONBLOCK);

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, filename, sizeof(addr.sun_path) - 1);

    unlink(filename);

    status = bind(pipe_fd, (struct sockaddr*)&addr, sizeof(addr));

    if (status != 0)
    {
        close(pipe_fd);
        return NULL;
    }

    status = listen(pipe_fd, max_clients);

    if (status != 0)
    {
        close(pipe_fd);
        return NULL;
    }

    return (XmfNamedPipe*) (size_t) pipe_fd;
#endif
}

XmfNamedPipe* XmfNamedPipe_Accept(XmfNamedPipe* np_handle)
{
#ifdef _WIN32
    if (!ConnectNamedPipe((HANDLE) np_handle, NULL)) {
        return NULL;
    }
    return np_handle;
#else
    int status;
    int pipe_fd = (int) (size_t) np_handle;
    struct sockaddr_un addr = { 0 };
    socklen_t length = sizeof(struct sockaddr_un);

    status = accept(pipe_fd, (struct sockaddr*)&addr, &length);

    if (status < 0) {
        return NULL;
    }

    return (XmfNamedPipe*) (size_t) status;
#endif
}


void XmfNamedPipe_Close(XmfNamedPipe* np_handle)
{
#ifdef _WIN32
    CloseHandle((HANDLE) np_handle);
#else
    if (np_handle) {
        int pipe_fd = (int) (size_t) np_handle;
        close(pipe_fd);
    }
#endif
}
