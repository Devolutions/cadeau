
#include "XmfCom.h"

#include "XmfRecorder.h"

const GUID IID_IUnknown = // "00000000-0000-0000-C000-000000000046"
	{ 0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
const GUID IID_IClassFactory = // "00000001-0000-0000-C000-000000000046"
	{ 0x00000001,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

const GUID IID_IXmfRecorder = // "900598B4-0844-4DDE-8169-4AFEBB78FD77"
	{0x900598B4,0x0844,0x4DDE,{0x81,0x69,0x4A,0xFE,0xBB,0x78,0xFD,0x77}};

static GUID GUID_NIL =
	{ 0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} };

int XmfGuid_Compare(const GUID* guid1, const GUID* guid2)
{
	int index;

	if (!guid1)
		guid1 = &GUID_NIL;

	if (!guid2)
		guid2 = &GUID_NIL;

	if (guid1->Data1 != guid2->Data1)
		return (guid1->Data1 < guid2->Data1) ? -1 : 1;

	if (guid1->Data2 != guid2->Data2)
		return (guid1->Data2 < guid2->Data2) ? -1 : 1;

	if (guid1->Data3 != guid2->Data3)
		return (guid1->Data3 < guid2->Data3) ? -1 : 1;

	for (index = 0; index < 8; index++)
	{
		if (guid1->Data4[index] != guid2->Data4[index])
			return (guid1->Data4[index] < guid2->Data4[index]) ? -1 : 1;
	}

	return 0;
}

bool XmfGuid_IsEqual(const GUID* guid1, const GUID* guid2)
{
	return ((XmfGuid_Compare(guid1, guid2) == 0) ? true : false);
}

static IClassFactory g_IClassFactory;

HRESULT STDCALL XmfClassFactory_QueryInterface(IClassFactory* This, REFIID riid, void** ppvObject)
{
	HRESULT hr = E_NOINTERFACE;

	if (XmfGuid_IsEqual(riid, &IID_IUnknown))
	{
		*ppvObject = (void*)((IUnknown*)This);
		hr = S_OK;
	}
	else if (XmfGuid_IsEqual(riid, &IID_IClassFactory))
	{
		*ppvObject = (void*)((IClassFactory*)This);
		hr = S_OK;
	}

	return hr;
}

ULONG STDCALL XmfClassFactory_AddRef(IClassFactory* This)
{
	return 0;
}

ULONG STDCALL XmfClassFactory_Release(IClassFactory* This)
{
	return 0;
}

HRESULT STDCALL XmfClassFactory_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
	HRESULT hr = E_NOINTERFACE;

	//if (!pUnkOuter)
		pUnkOuter = (IUnknown*) &g_IClassFactory;

	if (XmfGuid_IsEqual(riid, &IID_IUnknown))
	{
		*ppvObject = (void*)((IUnknown*)pUnkOuter);
		hr = S_OK;
	}
	else if (XmfGuid_IsEqual(riid, &IID_IClassFactory))
	{
		*ppvObject = (void*)((IClassFactory*)pUnkOuter);
		hr = S_OK;
	}
	else if (XmfGuid_IsEqual(riid, &IID_IXmfRecorder))
	{
		IUnknown* pvObject = (IUnknown*) XmfRecorder_New();
		hr = pvObject->vtbl->QueryInterface(pvObject, riid, ppvObject);
	}

	return hr;
}

HRESULT STDCALL XmfClassFactory_LockServer(IClassFactory* This, BOOL fLock)
{
	return S_OK;
}

static IClassFactoryVtbl g_IClassFactoryVtbl = {
	XmfClassFactory_QueryInterface,
	XmfClassFactory_AddRef,
	XmfClassFactory_Release,
	XmfClassFactory_CreateInstance,
	XmfClassFactory_LockServer
};

static IClassFactory g_IClassFactory = {
	&g_IClassFactoryVtbl
};
