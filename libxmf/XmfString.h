#ifndef XMF_STRING_H
#define XMF_STRING_H

#include <xmf/xmf.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32
#define _strdup strdup
#define sprintf_s snprintf
#endif

#ifdef _WIN32
int XmfConvertToUnicode(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte,
                     LPWSTR* lpWideCharStr, int cchWideChar);
                     
int XmfConvertFromUnicode(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar,
                       LPSTR* lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar,
                       LPBOOL lpUsedDefaultChar);
#endif

#ifdef __cplusplus
}
#endif

#endif /* XMF_STRING_H */
