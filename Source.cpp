#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "Rpcrt4")
#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "wininet")
#include <windows.h>
#include <Shlwapi.h>
#include <wininet.h>
#include <vector>
#include <string>

TCHAR szClassName[] = TEXT("Window");

LPBYTE DownloadToMemory(IN LPCWSTR lpszURL)
{
	LPBYTE lpszReturn = 0;
	DWORD dwSize = 0;
	const HINTERNET hSession = InternetOpenW(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, INTERNET_FLAG_NO_COOKIES);
	if (hSession)
	{
		URL_COMPONENTSW uc = { 0 };
		WCHAR HostName[MAX_PATH];
		WCHAR UrlPath[MAX_PATH];
		uc.dwStructSize = sizeof(uc);
		uc.lpszHostName = HostName;
		uc.lpszUrlPath = UrlPath;
		uc.dwHostNameLength = MAX_PATH;
		uc.dwUrlPathLength = MAX_PATH;
		InternetCrackUrlW(lpszURL, 0, 0, &uc);
		const HINTERNET hConnection = InternetConnectW(hSession, HostName, INTERNET_DEFAULT_HTTP_PORT, 0, 0, INTERNET_SERVICE_HTTP, 0, 0);
		if (hConnection)
		{
			const HINTERNET hRequest = HttpOpenRequestW(hConnection, L"GET", UrlPath, 0, 0, 0, 0, 0);
			if (hRequest)
			{
				ZeroMemory(&uc, sizeof(URL_COMPONENTS));
				WCHAR Scheme[16];
				uc.dwStructSize = sizeof(uc);
				uc.lpszScheme = Scheme;
				uc.lpszHostName = HostName;
				uc.dwSchemeLength = 16;
				uc.dwHostNameLength = MAX_PATH;
				InternetCrackUrlW(lpszURL, 0, 0, &uc);
				WCHAR szReferer[1024];
				lstrcpyW(szReferer, L"Referer: ");
				lstrcatW(szReferer, Scheme);
				lstrcatW(szReferer, L"://");
				lstrcatW(szReferer, HostName);
				HttpAddRequestHeadersW(hRequest, szReferer, lstrlenW(szReferer), HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
				HttpSendRequestW(hRequest, 0, 0, 0, 0);
				lpszReturn = (LPBYTE)GlobalAlloc(GMEM_FIXED, 1);
				DWORD dwRead;
				static BYTE szBuf[1024 * 4];
				LPBYTE lpTmp;
				for (;;)
				{
					if (!InternetReadFile(hRequest, szBuf, (DWORD)sizeof(szBuf), &dwRead) || !dwRead) break;
					lpTmp = (LPBYTE)GlobalReAlloc(lpszReturn, (SIZE_T)(dwSize + dwRead), GMEM_MOVEABLE);
					if (lpTmp == NULL) break;
					lpszReturn = lpTmp;
					CopyMemory(lpszReturn + dwSize, szBuf, dwRead);
					dwSize += dwRead;
				}
				InternetCloseHandle(hRequest);
			}
			InternetCloseHandle(hConnection);
		}
		InternetCloseHandle(hSession);
	}
	return lpszReturn;
}

BOOL DownloadToFile(IN LPCWSTR lpszURL, IN LPCWSTR lpszFilePath)
{
	BOOL bReturn = FALSE;
	LPBYTE lpByte = DownloadToMemory(lpszURL);
	if (lpByte)
	{
		const DWORD dwSize = (DWORD)GlobalSize(lpByte);
		static BYTE szBuf[1024 * 4];
		const HANDLE hFile = CreateFileW(lpszFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwWritten;
			WriteFile(hFile, lpByte, dwSize, &dwWritten, NULL);
			CloseHandle(hFile);
			bReturn = TRUE;
		}
		GlobalFree(lpByte);
	}
	return bReturn;
}

BOOL CreateTempDirectory(LPWSTR pszDir)
{
	DWORD dwSize = GetTempPath(0, 0);
	if (dwSize == 0 || dwSize > MAX_PATH - 14) { goto END0; }
	LPWSTR pTmpPath;
	pTmpPath = (LPWSTR)GlobalAlloc(GPTR, sizeof(WCHAR)*(dwSize + 1));
	GetTempPathW(dwSize + 1, pTmpPath);
	dwSize = GetTempFileNameW(pTmpPath, L"", 0, pszDir);
	GlobalFree(pTmpPath);
	if (dwSize == 0) { goto END0; }
	DeleteFileW(pszDir);
	if (CreateDirectoryW(pszDir, 0) == 0) { goto END0; }
	return TRUE;
END0:
	return FALSE;
}

std::wstring Replace(std::wstring String1, std::wstring String2, std::wstring String3)
{
	std::wstring::size_type Pos(String1.find(String2));
	while (Pos != std::wstring::npos)
	{
		String1.replace(Pos, String2.length(), String3);
		Pos = String1.find(String2, Pos + String3.length());
	}
	return String1;
}

LPWSTR Download2WChar(LPCWSTR lpszPostURL)
{
	LPWSTR lpszReturn = 0;
	LPBYTE lpByte = DownloadToMemory(lpszPostURL);
	if (lpByte)
	{
		const DWORD len = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)lpByte, -1, 0, 0);
		lpszReturn = (LPWSTR)GlobalAlloc(0, (len + 1) * sizeof(WCHAR));
		if (lpszReturn)
		{
			MultiByteToWideChar(CP_UTF8, 0, (LPCCH)lpByte, -1, lpszReturn, len);
		}
		GlobalFree(lpByte);
	}
	return lpszReturn;
}

BOOL URLToFileName(LPCWSTR lpszURL, LPWSTR lpszFileName, int nFileNameSize)
{
	if (lpszURL != NULL)
	{
		LPCWSTR p = lpszURL + lstrlenW(lpszURL);
		for (; p != lpszURL; --p)
		{
			if (*p == L'/')
			{
				break;
			}
		}
		if (lstrlenW(p) >= 2)
		{
			lstrcpynW(lpszFileName, p + (p == lpszURL ? 0 : 1), nFileNameSize);
			for (int i = 0; lpszFileName[i] != 0; ++i)
			{
				if (lpszFileName[i] == L'\"' ||
					lpszFileName[i] == L'<' ||
					lpszFileName[i] == L'>' ||
					lpszFileName[i] == L'|' ||
					lpszFileName[i] == L':' ||
					lpszFileName[i] == L'*' ||
					lpszFileName[i] == L'?' ||
					lpszFileName[i] == L'\\' ||
					lpszFileName[i] == L'/')
					lpszFileName[i] = 0;
			}
			if (lstrlenW(lpszFileName) == 0)
				goto SETDEFAULT;
			return TRUE;
		}
	}
SETDEFAULT:
#define DEFAULT_FILE_NAME L"video.mp4"
	lstrcpynW(lpszFileName, DEFAULT_FILE_NAME, nFileNameSize);
#undef DEFAULT_FILE_NAME
	return TRUE;
}

BOOL DownloadFacebookVideo(LPCWSTR lpszPostURL, LPCWSTR lpszOutputFolder = 0)
{
	BOOL bReturnValue = FALSE;
	LPWSTR lpszWeb = Download2WChar(lpszPostURL);
	if (!lpszWeb) return 0;
	std::wstring srcW(lpszWeb);
	GlobalFree(lpszWeb);

	size_t posStart = 0, posEnd = 0;

	LPWSTR lpszQualityTag[] = { L"hd_src_no_ratelimit", L"hd_src", L"sd_src_no_ratelimit", L"sd_src" };

	for (auto v : lpszQualityTag)
	{
		posStart = srcW.find(std::wstring(v) + std::wstring(L":\""), posEnd);
		if (posStart != std::wstring::npos)
		{
			posStart += lstrlenW(v) + 2;
			posEnd = srcW.find(L'\"', posStart);
			std::wstring url(srcW, posStart, posEnd - posStart);
			WCHAR szFileName[MAX_PATH];
			URLToFileName(url.c_str(), szFileName, _countof(szFileName));
			WCHAR szFullFileName[MAX_PATH] = { 0 };
			if (lpszOutputFolder)
			{
				lstrcpyW(szFullFileName, lpszOutputFolder);
				PathAppendW(szFullFileName, szFileName);
			}
			else
			{
				GetModuleFileNameW(0, szFullFileName, _countof(szFullFileName));
				PathRemoveFileSpecW(szFullFileName);
				PathAppendW(szFullFileName, szFileName);
			}
			if (DownloadToFile(url.c_str(), szFullFileName))
			{
				break;
			}
		}
	}

	return bReturnValue;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hEdit, hButton;
	switch (msg)
	{
	case WM_CREATE:
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT("https://www.facebook.com/BBCEntsNews/videos/1229594863791391/"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 10, 10, 1024, 32, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("ダウンロード開始"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 10, 50, 256, 32, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		break;
	case WM_SIZE:
		MoveWindow(hEdit, 10, 10, lParam & 0xFFFF - 20, 32, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			EnableWindow(hEdit, FALSE);
			EnableWindow(hButton, FALSE);
			DWORD dwSize = GetWindowTextLengthW(hEdit);
			if (dwSize)
			{
				LPWSTR lpszInputUrl = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * (dwSize + 1));
				GetWindowTextW(hEdit, lpszInputUrl, dwSize + 1);
				DownloadFacebookVideo(lpszInputUrl);
				GlobalFree(lpszInputUrl);
			}
			EnableWindow(hEdit, TRUE);
			EnableWindow(hButton, TRUE);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefDlgProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	int n;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &n);
	if (n == 1)
	{
		MSG msg;
		WNDCLASS wndclass = {
			CS_HREDRAW | CS_VREDRAW,
			WndProc,
			0,
			DLGWINDOWEXTRA,
			hInstance,
			0,
			LoadCursor(0,IDC_ARROW),
			0,
			0,
			szClassName
		};
		RegisterClass(&wndclass);
		HWND hWnd = CreateWindow(
			szClassName,
			TEXT("Facebookの投稿に含まれる動画をMP4形式でダウンロードする"),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			0,
			CW_USEDEFAULT,
			0,
			0,
			0,
			hInstance,
			0
		);
		ShowWindow(hWnd, SW_SHOWDEFAULT);
		UpdateWindow(hWnd);
		while (GetMessage(&msg, 0, 0, 0))
		{
			if (!IsDialogMessage(hWnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	else if (n == 2)
	{
		DownloadFacebookVideo(argv[1]);
	}
	else if (n >= 3)
	{
		DownloadFacebookVideo(argv[1], argv[2]);
	}
	if (argv) LocalFree(argv);
	return 0;
}
