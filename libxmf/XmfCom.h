#ifndef XMF_COM_H
#define XMF_COM_H

#include <xmf/xmf.h>

#include <winpr/wtypes.h>
#include <winpr/rpc.h>

#ifndef STDCALL
#ifdef _WIN32
#define STDCALL __stdcall
#else
#define STDCALL 
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _IUnknown IUnknown;

typedef struct IUnknownVtbl
{
    HRESULT(STDCALL* QueryInterface)(IUnknown* This, REFIID riid, void** ppvObject);
    uint32_t(STDCALL* AddRef)(IUnknown* This);
    uint32_t(STDCALL* Release)(IUnknown* This);
} IUnknownVtbl;

struct _IUnknown
{
    IUnknownVtbl* vtbl;
};

typedef struct _IClassFactory IClassFactory;

typedef struct IClassFactoryVtbl
{
    HRESULT(STDCALL* QueryInterface)(IClassFactory* This, REFIID riid, void** ppvObject);
    uint32_t(STDCALL* AddRef)(IClassFactory* This);
    uint32_t(STDCALL* Release)(IClassFactory* This);
    HRESULT(STDCALL* CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
    HRESULT(STDCALL* LockServer)(IClassFactory* This, BOOL fLock);
} IClassFactoryVtbl;

struct _IClassFactory
{
    IClassFactoryVtbl* vtbl;
};

typedef struct _IXmfRecorder IXmfRecorder;

typedef struct IXmfRecorderVtbl
{
    HRESULT(STDCALL* QueryInterface)(IXmfRecorder* This, REFIID riid, void** ppvObject);
    uint32_t(STDCALL* AddRef)(IXmfRecorder* This);
    uint32_t(STDCALL* Release)(IXmfRecorder* This);
    bool(STDCALL* Init)(IXmfRecorder* This);
    void(STDCALL* Uninit)(IXmfRecorder* This);
    void(STDCALL* SetFilename)(IXmfRecorder* This, const char* filename);
    void(STDCALL* SetFrameSize)(IXmfRecorder* This, uint32_t frameWidth, uint32_t frameHeight);
    uint32_t(STDCALL* GetFrameRate)(IXmfRecorder* This);
    void(STDCALL* SetFrameRate)(IXmfRecorder* This, uint32_t frameRate);
    void(STDCALL* SetVideoQuality)(IXmfRecorder* This, uint32_t videoQuality);
    uint32_t(STDCALL* GetTimeout)(IXmfRecorder* This);
    void(STDCALL* Timeout)(IXmfRecorder* This);
    void(STDCALL* UpdateFrame)(IXmfRecorder* This,
        uint8_t* buffer, uint32_t updateX, uint32_t updateY,
        uint32_t updateWidth, uint32_t updateHeight, uint32_t surfaceStep);
} IXmfRecorderVtbl;

struct _IXmfRecorder
{
    IXmfRecorderVtbl* vtbl;
    uint32_t refCount;
};

extern const GUID IID_IUnknown;
extern const GUID IID_IClassFactory;
extern const GUID IID_IXmfRecorder;

bool XmfGuid_IsEqual(const GUID* guid1, const GUID* guid2);

XMF_EXPORT HRESULT STDCALL XmfClassFactory_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);

#ifdef __cplusplus
}
#endif

#endif /* XMF_COM_H */