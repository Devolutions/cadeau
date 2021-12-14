
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <shlwapi.h>
#include <shlobj.h>
#pragma comment(lib, "shlwapi.lib")
#endif

#include <winpr/file.h>
#include <winpr/path.h>

#include "XmfFile.h"

int XmfFile_Seek(FILE* fp, uint64_t offset, int origin)
{
#ifdef _WIN32
	return (int) _fseeki64(fp, offset, origin);
#elif defined(__APPLE__)
	return (int) fseeko(fp, offset, origin);
#else
	return (int) fseeko(fp, offset, origin);
#endif
}

uint64_t XmfFile_Tell(FILE* fp)
{
#ifdef _WIN32
	return (uint64_t) _ftelli64(fp);
#elif defined(__APPLE__)
	return (uint64_t) ftello(fp);
#else
	return (uint64_t) ftello(fp);
#endif
}

uint64_t XmfFile_Size(const char* filename)
{
	FILE* fp = NULL;
	uint64_t fileSize;

	fp = XmfFile_Open(filename, "rb");

	if (!fp)
		return 0;

	XmfFile_Seek(fp, 0, SEEK_END);
	fileSize = XmfFile_Tell(fp);
	fclose(fp);

	return fileSize;
}

const char* XmfFile_Base(const char* filename)
{
	size_t length;
	char* separator;

	if (!filename)
		return NULL;

	separator = strrchr(filename, '\\');

	if (!separator)
		separator = strrchr(filename, '/');

	if (!separator)
		return filename;

	length = strlen(filename);

	if ((length - (separator - filename)) > 1)
		return separator + 1;

	return filename;
}

char* XmfFile_Dir(const char* filename)
{
	char* dir;
	char* end;
	char* base;
	size_t length;

	base = (char*) XmfFile_Base(filename);

	if (!base)
		return NULL;

	end = base - 1;

	if (end < filename)
		return NULL;

	length = end - filename;
	dir = malloc(length + 1);

	if (!dir)
		return NULL;

	CopyMemory(dir, filename, length);
	dir[length] = '\0';

	return dir;
}

const char* XmfFile_Extension(const char* filename, bool dot)
{
	char* p;
	size_t length;

	if (!filename)
		return NULL;

	p = strrchr(filename, '.');

	if (!p)
		return NULL;

	if (dot)
		return p;

	length = strlen(filename);

	if ((length - (p - filename)) > 1)
		return p + 1;

	return NULL;
}

FILE* XmfFile_Open(const char* path, const char* mode)
{
#ifndef _WIN32
	return fopen(path, mode);
#else
	LPWSTR lpPathW = NULL;
	LPWSTR lpModeW = NULL;
	FILE* result = NULL;

	if (!path || !mode)
		return NULL;

	if (ConvertToUnicode(CP_UTF8, 0, path, -1, &lpPathW, 0) < 1)
		goto cleanup;

	if (ConvertToUnicode(CP_UTF8, 0, mode, -1, &lpModeW, 0) < 1)
		goto cleanup;

	result = _wfopen(lpPathW, lpModeW);

cleanup:
	free(lpPathW);
	free(lpModeW);
	return result;
#endif
}

uint8_t* XmfFile_Load(const char* filename, size_t* size, uint32_t flags)
{
	FILE* fp = NULL;
	uint8_t* data = NULL;

	if (!filename || !size)
		return NULL;

	*size = 0;

	fp = XmfFile_Open(filename, "rb");

	if (!fp)
		return NULL;

	XmfFile_Seek(fp, 0, SEEK_END);
	*size = XmfFile_Tell(fp);
	XmfFile_Seek(fp, 0, SEEK_SET);

	data = malloc(*size + 1);

	if (!data)
		goto exit;

	data[*size] = '\0';

	if (fread(data, 1, *size, fp) != *size)
	{
		free(data);
		data = NULL;
		*size = 0;
	}

	if (flags & XMF_FILE_FLAG_TERMINATOR)
		*size = *size + 1;

exit:
	fclose(fp);
	return data;
}

bool XmfFile_Save(const char* filename, uint8_t* data, size_t size, int mode)
{
	FILE* fp = NULL;
	bool success = true;

	if (!filename || !data)
		return false;

	fp = XmfFile_Open(filename, "wb");

	if (!fp)
		return false;

	if (fwrite(data, 1, size, fp) != size)
	{
		success = false;
	}

	fclose(fp);
	return success;
}

/**
 * Naming Files, Paths, and Namespaces:
 * https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247
 */

char* XmfFile_SanitizeName(const char* name, char defaultChar, uint32_t flags)
{
	int length;
	char* sane;
	char* p = NULL;
	static char reservedChars[] = { '<', '>', ':', '"', '/', '\\', '|', '?', '*', '%', '\0' };

	if (!name)
		return NULL;

	sane = _strdup(name);

	if (!sane)
		return NULL;

	if (!defaultChar)
		defaultChar = '_';

	/* replace reserved characters */

	p = sane;

	do
	{
		p = strpbrk(p, reservedChars);

		if (p)
		{
			*p = defaultChar;
			p++;
		}
	}
	while (p);

	/* trim spaces and periods at the end */

	length = (int) strlen(sane);

	if (length > 1)
	{
		p = &sane[length - 1];

		while (p > sane)
		{
			if ((*p == ' ') || (*p == '.'))
			{
				*p = '\0';
				p--;
			}
			else
			{
				break;
			}
		}
	}

	return sane;
}

static BOOL MoveFileX(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD flags)
{
#ifndef _WIN32
	return MoveFileExA(lpExistingFileName, lpNewFileName, flags);
#else
	BOOL result = FALSE;
	LPWSTR lpExistingFileNameW = NULL;
	LPWSTR lpNewFileNameW = NULL;

	if (!lpExistingFileName)
		goto cleanup;

	if (ConvertToUnicode(CP_UTF8, 0, lpExistingFileName, -1, &lpExistingFileNameW, 0) < 1)
		goto cleanup;

	if (lpNewFileName)
	{
		if (ConvertToUnicode(CP_UTF8, 0, lpNewFileName, -1, &lpNewFileNameW, 0) < 1)
			goto cleanup;
	}

	result = MoveFileExW(lpExistingFileNameW, lpNewFileNameW, flags);

cleanup:
	free(lpExistingFileNameW);
	free(lpNewFileNameW);
	return result;
#endif
}

bool XmfFile_Move(const char* filename, const char* newFilename, bool overwrite)
{
	return MoveFileX(filename, newFilename, overwrite ? MOVEFILE_REPLACE_EXISTING : 0);
}

static BOOL DeleteFileX(const char* lpFileName)
{
#ifndef _WIN32
	return DeleteFileA(lpFileName);
#else
	LPWSTR lpFileNameW = NULL;
	BOOL result = FALSE;

	if (lpFileName)
	{
		if (ConvertToUnicode(CP_UTF8, 0, lpFileName, -1, &lpFileNameW, 0) < 1)
			goto cleanup;
	}

	result = DeleteFileW(lpFileNameW);

cleanup:
	free(lpFileNameW);
	return result;
#endif
}

bool XmfFile_Delete(const char* filename)
{
	return DeleteFileX(filename);
}

static BOOL RemoveDirectoryX(LPCSTR lpPathName)
{
#ifndef _WIN32
	return RemoveDirectoryA(lpPathName);
#else
	LPWSTR lpPathNameW = NULL;
	BOOL result = FALSE;

	if (lpPathName)
	{
		if (ConvertToUnicode(CP_UTF8, 0, lpPathName, -1, &lpPathNameW, 0) < 1)
			goto cleanup;
	}

	result = RemoveDirectoryW(lpPathNameW);

cleanup:
	free(lpPathNameW);
	return result;
#endif
}

static BOOL PathFileExistsX(const char* pszPath)
{
#ifndef _WIN32
	return PathFileExistsA(pszPath);
#else
	WCHAR* pszPathW = NULL;
	BOOL result = FALSE;

	if (ConvertToUnicode(CP_UTF8, 0, pszPath, -1, &pszPathW, 0) < 1)
		return FALSE;

	result = PathFileExistsW(pszPathW);
	free(pszPathW);

	return result;
#endif
}

bool XmfFile_Exists(const char* filename)
{
	return PathFileExistsX(filename);
}

static BOOL PathMakePathX(const char* path, LPSECURITY_ATTRIBUTES lpAttributes)
{
#ifndef _WIN32
	return PathMakePathA(path, lpAttributes);
#else
	WCHAR* pathW = NULL;
	BOOL result = FALSE;
	
	if (ConvertToUnicode(CP_UTF8, 0, path, -1, &pathW, 0) < 1)
		return FALSE;

	result = SHCreateDirectoryExW(NULL, pathW, lpAttributes) == ERROR_SUCCESS;
	free(pathW);

	return result;
#endif
}

bool XmfFile_MakePath(const char* path, int mode, bool check)
{
	if (check && XmfFile_Exists(path))
		return true;

	return PathMakePathX(path, NULL);
}

bool XmfPath_Append(char* pszPath, size_t cchPath, const char* pszMore)
{
	BOOL pathBackslash;
	BOOL moreBackslash;
	size_t pszMoreLength;
	size_t pszPathLength;

	if (!pszPath || !pszMore)
		return false;

	if ((cchPath == 0) || (cchPath > XMF_MAX_PATH))
		return false;

	pszMoreLength = strlen(pszMore);
	pszPathLength = strlen(pszPath);

	pathBackslash = (pszPath[pszPathLength - 1] == XMF_PATH_SEPARATOR_CHR) ? TRUE : FALSE;
	moreBackslash = (pszMore[0] == XMF_PATH_SEPARATOR_CHR) ? TRUE : FALSE;

	if (pathBackslash && moreBackslash)
	{
		if ((pszPathLength + pszMoreLength - 1) < cchPath)
		{
			sprintf_s(&pszPath[pszPathLength], cchPath - pszPathLength, "%s", &pszMore[1]);
			return true;
		}
	}
	else if ((pathBackslash && !moreBackslash) || (!pathBackslash && moreBackslash))
	{
		if ((pszPathLength + pszMoreLength) < cchPath)
		{
			sprintf_s(&pszPath[pszPathLength], cchPath - pszPathLength, "%s", pszMore);
			return true;
		}
	}
	else if (!pathBackslash && !moreBackslash)
	{
		if ((pszPathLength + pszMoreLength + 1) < cchPath)
		{
			sprintf_s(&pszPath[pszPathLength], cchPath - pszPathLength, XMF_PATH_SEPARATOR_STR "%s", pszMore);
			return true;
		}
	}

	return false;
}

bool XmfPath_Combine(char* pszPathOut, size_t cchPathOut, const char* pszPathIn, const char* pszMore)
{
	if (!pszPathOut || !pszPathIn)
		return false;

	pszPathOut[0] = '\0';
	strncpy(pszPathOut, pszPathIn, cchPathOut);

	if (!pszMore)
		return true;

	return XmfPath_Append(pszPathOut, cchPathOut, pszMore);
}
