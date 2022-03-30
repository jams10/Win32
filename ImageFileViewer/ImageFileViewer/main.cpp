#include <windows.h>
#include <commdlg.h>
#include <strsafe.h>
#include "resource.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#define MAX_LOADSTRING 100

enum class FILE_MODE
{
    OPEN = 0,
    SAVE,
};

enum class FILE_EXTENSION : DWORD
{
    BMP = 1,
    PNG,
    JPEG
};

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
Gdiplus::Bitmap* pBitmap = nullptr;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

void InitOpenFileName(HWND hWnd, OPENFILENAME& fileNameStruct, wchar_t* string, FILE_MODE fileMode);
BOOL SaveFile(const WCHAR* filename, const WCHAR* fileExtension, Gdiplus::Bitmap* bitmap);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // 윈도우 클래스 등록.
    MyRegisterClass(hInstance);

    // 윈도우 생성.
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    // GDI+ 라이브러리 초기화.
    Gdiplus::GdiplusStartupInput tmp;
    ULONG_PTR token;
    Gdiplus::GdiplusStartup(&token, &tmp, NULL);

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Gdiplus::GdiplusShutdown(token);

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL; //LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32FILEOPEN));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU1);
    wcex.lpszClassName = L"윈도우 클래스";
    wcex.hIconSm = NULL; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    HWND hWnd = CreateWindowW(L"윈도우 클래스", L"이미지 파일 뷰어", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    OPENFILENAME ofn;

    static wchar_t strFile[512] = { 0, };
    static wchar_t strFileExtension[10] = { 0, };

    HBITMAP hBitmap = NULL;

    PAINTSTRUCT ps;

    static HDC hMemDC;
    static int nWidth, nHeight;

    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam); // 메뉴 ID 얻어오기.

            switch (wmId)
            {
#pragma region OpenFile
            case ID_40001: // 파일 열기 메뉴
                InitOpenFileName(hWnd, ofn, strFile, FILE_MODE::OPEN); // OPENFILENAME 구조체 초기화.
                if (GetOpenFileName(&ofn) != 0)
                {
                    // 파일 이름으로 부터 비트맵 생성.
                    pBitmap = new Gdiplus::Bitmap(strFile);
                    if (pBitmap->GetLastStatus() != Gdiplus::Status::Ok)
                    {
                        MessageBox(hWnd, L"비트맵 파일을 생성할 수 없습니다.", L"파일 열기 오류", MB_OK);
                        break;
                    }
                    // 비트맵 크기 얻어오기.
                    nWidth = pBitmap->GetWidth();
                    nHeight = pBitmap->GetHeight();

                    // 비트맵 핸들 얻어오기.
                    pBitmap->GetHBITMAP(RGB(0, 0, 0), &hBitmap);

                    // 메모리 DC 생성.
                    hdc = GetDC(hWnd);
                    if (hMemDC != 0) DeleteDC(hMemDC); // 새 파일을 열어준 경우, 기존의 메모리 DC는 삭제해줌.
                    hMemDC = CreateCompatibleDC(hdc);

                    // 메모리 DC의 비트맵 교체.
                    SelectObject(hMemDC, hBitmap);
                    
                    // 사용을 다한 GDI 오브젝트 해제.
                    ReleaseDC(hWnd, hdc);

                    SetWindowText(hWnd, strFile);

                    InvalidateRect(hWnd, NULL, TRUE);
                }
                break;
#pragma endregion
#pragma region SaveFile
            case ID_40002:
                if (pBitmap == nullptr)
                {
                    MessageBox(hWnd, L"저장할 비트맵 파일이 없습니다. 먼저 불러와야 합니다.", L"파일 저장 오류", MB_OK);
                    break;
                }
                InitOpenFileName(hWnd, ofn, strFile, FILE_MODE::SAVE); // OPENFILENAME 구조체 초기화.
                if (GetSaveFileName(&ofn) != 0)
                {
                    // 파일 확장자 저장해두기.
                    switch (ofn.nFilterIndex)
                    {
                    case (DWORD)FILE_EXTENSION::BMP:
                        StringCbPrintf(strFileExtension, sizeof(strFileExtension), L"%s", L"bmp"); // 파일 확장자 저장 문자열
                        if (ofn.nFileExtension == 0) // 사용자가 확장자를 입력하지 않은 경우.
                        {
                            lstrcatW(strFile, L".bmp");
                        }
                        break;
                    case (DWORD)FILE_EXTENSION::PNG:
                        StringCbPrintf(strFileExtension, sizeof(strFileExtension), L"%s", L"png");
                        if (ofn.nFileExtension == 0) // 사용자가 확장자를 입력하지 않은 경우.
                        {
                            lstrcatW(strFile, L".png");
                        }
                        break;
                    case (DWORD)FILE_EXTENSION::JPEG:
                        StringCbPrintf(strFileExtension, sizeof(strFileExtension), L"%s", L"jpeg");
                        if (ofn.nFileExtension == 0) // 사용자가 확장자를 입력하지 않은 경우.
                        {
                            lstrcatW(strFile, L".jpeg");
                        }
                        break;
                    }

                    // 비트맵 파일 -> 이미지 파일 저장 함수 호출.
                    if (!SaveFile(strFile, strFileExtension, pBitmap))
                    {
                        MessageBox(hWnd, L"파일을 저장할 수 없습니다.", L"파일 저장 오류", MB_OK);
                        break;
                    }
                    else
                    {
                        MessageBox(hWnd, L"파일을 성공적으로 저장했습니다!", L"파일 저장 성공", MB_OK);
                    }
                }
                break;
#pragma endregion
            }

        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        if (hMemDC != 0)
        {
            BitBlt(hdc, 0, 0, nWidth, nHeight, hMemDC, 0, 0, SRCCOPY);
        }

        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        if(hBitmap != NULL) DeleteObject(hBitmap);
        if (pBitmap != nullptr) delete pBitmap; // 사용을 다햇으므로, 할당 받은 Bitmap 객체 삭제.
        if (hMemDC != 0) DeleteDC(hMemDC);      // 사용을 다했으므로, 종료 전 메모리 DC 삭제.
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void InitOpenFileName(HWND hWnd, OPENFILENAME& fileNameStruct, wchar_t* string, FILE_MODE fileMode)
{
    memset(&fileNameStruct, 0, sizeof(OPENFILENAME));
    fileNameStruct.lStructSize = sizeof(OPENFILENAME);
    fileNameStruct.hwndOwner = hWnd;

    if (fileMode == FILE_MODE::OPEN)
    {
        fileNameStruct.lpstrTitle = L"열어줄 파일을 선택해 주세요";
    }
    else fileNameStruct.lpstrTitle = L"저장할 파일을 선택해 주세요";

    fileNameStruct.lpstrFile = string;
    fileNameStruct.lpstrFilter = L"Bitmap(*.bmp)\0*.bmp\0png(*.png)\0*.png\0jpeg(*.jpeg)\0*.jpeg\0";
    fileNameStruct.nMaxFile = MAX_PATH;
    fileNameStruct.nMaxFileTitle = MAX_PATH;
}

// Gdiplus::Bitmap 객체를 원하는 이미지 포맷 파일로 저장해주는 함수.
BOOL SaveFile(const WCHAR* filename, const WCHAR* fileExtension, Gdiplus::Bitmap* pBitmap)
{
    CLSID encoderID;        // 해당 이미지 포맷 encoder가 있는지 확인하기 위한 클래스 아이디.

    UINT  num = 0;          // 이미지 encoder들의 개수
    UINT  size = 0;         // 이미지 encoder 배열의 바이트 크기

    WCHAR strFormat[32];
    StringCbPrintf(strFormat, sizeof(strFormat), L"image/%s", fileExtension);

    // ImageCodecInfo : 설치된 이미지 encoder 및 decoder(codec이라 함)의 관련 정보를 검색하는데 필요한 멤버 및 함수들을 제공함.
    Gdiplus::ImageCodecInfo* pImageCodecInfo = nullptr;

    //	사용 가능한 image encoder들의 개수와 ImageCodecInfo 객체 배열의 총 크기를 얻어옴.
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
    {
        return FALSE;
    }

    // ImageCodecInfo 배열의 총 크기 만큼 공간을 할당.
    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == nullptr)
    {
        return FALSE;
    }

    // 이미지 encoder의 배열을 얻어옴.
    GetImageEncoders(num, size, pImageCodecInfo);

    // image encoder 배열을 돌면서, 와이드 문자 스트링 비교를 통해 같은 format의 
    // image encoder가 있으면, CLSID(COM 객체 식별 고유 키)를 넘겨줌. 
    for (UINT j = 0; j < num; ++j)
    {
        if (lstrcmpW(pImageCodecInfo[j].MimeType, strFormat) == 0) // 같은 format의 encoder를 발견한 경우
        {
            encoderID = pImageCodecInfo[j].Clsid; // pClsid에 클래스 아이디를 넘겨줌.
            free(pImageCodecInfo);                // 할당 받았던 공간 해제.
            break;
        }
        if (j == num - 1)                         // 만약 같은 format의 image encoder가 없으면,
        {
            free(pImageCodecInfo);                // 할당 받았던 공간 해제.
            return FALSE;
        }
    }

    // 우리가 위에서 얻어온 파일 encoder 클래스 아이디를 이용, 원하는 파일 포맷으로 이미지 파일을 저장해줌.
    if (pBitmap->Save(filename, &encoderID, nullptr) != Gdiplus::Status::Ok)
    {
        return FALSE;
    }

    return TRUE;
}