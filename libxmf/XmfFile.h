#ifndef XMF_FILE_H
#define XMF_FILE_H

#include <xmf/xmf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XMF_MAX_PATH                    512

#define XMF_PATH_SLASH_CHR              '/'
#define XMF_PATH_SLASH_STR              "/"

#define XMF_PATH_BACKSLASH_CHR          '\\'
#define XMF_PATH_BACKSLASH_STR          "\\"

#ifdef _WIN32
#define XMF_PATH_SEPARATOR_CHR          XMF_PATH_BACKSLASH_CHR
#define XMF_PATH_SEPARATOR_STR          XMF_PATH_BACKSLASH_STR
#else
#define XMF_PATH_SEPARATOR_CHR          XMF_PATH_SLASH_CHR
#define XMF_PATH_SEPARATOR_STR          XMF_PATH_SLASH_STR
#endif

#define XMF_FILE_FLAG_TERMINATOR        0x00000001

int XmfFile_Seek(FILE* fp, uint64_t offset, int origin);
uint64_t XmfFile_Tell(FILE* fp);
uint64_t XmfFile_Size(const char* filename);
const char* XmfFile_Base(const char* filename);
char* XmfFile_Dir(const char* filename);
const char* XmfFile_Extension(const char* filename, bool dot);

FILE* XmfFile_Open(const char* path, const char* mode);
void XmfFile_Close(FILE* fp);

uint8_t* XmfFile_Load(const char* filename, size_t* size, uint32_t flags);
bool XmfFile_Save(const char* filename, uint8_t* data, size_t size, int mode);

bool XmfFile_Delete(const char* filename);

char* XmfFile_SanitizeName(const char* name, char defaultChar, uint32_t flags);

bool XmfPath_Append(char* pszPath, size_t cchPath, const char* pszMore);
bool XmfPath_Combine(char* pszPathOut, size_t cchPathOut, const char* pszPathIn, const char* pszMore);

#ifdef __cplusplus
}
#endif

#endif /* XMF_FILE_H */
