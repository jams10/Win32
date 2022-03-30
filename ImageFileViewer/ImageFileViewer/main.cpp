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

// ���� ����:
HINSTANCE hInst;                                // ���� �ν��Ͻ��Դϴ�.
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
    // ������ Ŭ���� ���.
    MyRegisterClass(hInstance);

    // ������ ����.
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    // GDI+ ���̺귯�� �ʱ�ȭ.
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
    wcex.lpszClassName = L"������ Ŭ����";
    wcex.hIconSm = NULL; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // �ν��Ͻ� �ڵ��� ���� ������ �����մϴ�.

    HWND hWnd = CreateWindowW(L"������ Ŭ����", L"�̹��� ���� ���", WS_OVERLAPPEDWINDOW,
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
            int wmId = LOWORD(wParam); // �޴� ID ������.

            switch (wmId)
            {
#pragma region OpenFile
            case ID_40001: // ���� ���� �޴�
                InitOpenFileName(hWnd, ofn, strFile, FILE_MODE::OPEN); // OPENFILENAME ����ü �ʱ�ȭ.
                if (GetOpenFileName(&ofn) != 0)
                {
                    // ���� �̸����� ���� ��Ʈ�� ����.
                    pBitmap = new Gdiplus::Bitmap(strFile);
                    if (pBitmap->GetLastStatus() != Gdiplus::Status::Ok)
                    {
                        MessageBox(hWnd, L"��Ʈ�� ������ ������ �� �����ϴ�.", L"���� ���� ����", MB_OK);
                        break;
                    }
                    // ��Ʈ�� ũ�� ������.
                    nWidth = pBitmap->GetWidth();
                    nHeight = pBitmap->GetHeight();

                    // ��Ʈ�� �ڵ� ������.
                    pBitmap->GetHBITMAP(RGB(0, 0, 0), &hBitmap);

                    // �޸� DC ����.
                    hdc = GetDC(hWnd);
                    if (hMemDC != 0) DeleteDC(hMemDC); // �� ������ ������ ���, ������ �޸� DC�� ��������.
                    hMemDC = CreateCompatibleDC(hdc);

                    // �޸� DC�� ��Ʈ�� ��ü.
                    SelectObject(hMemDC, hBitmap);
                    
                    // ����� ���� GDI ������Ʈ ����.
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
                    MessageBox(hWnd, L"������ ��Ʈ�� ������ �����ϴ�. ���� �ҷ��;� �մϴ�.", L"���� ���� ����", MB_OK);
                    break;
                }
                InitOpenFileName(hWnd, ofn, strFile, FILE_MODE::SAVE); // OPENFILENAME ����ü �ʱ�ȭ.
                if (GetSaveFileName(&ofn) != 0)
                {
                    // ���� Ȯ���� �����صα�.
                    switch (ofn.nFilterIndex)
                    {
                    case (DWORD)FILE_EXTENSION::BMP:
                        StringCbPrintf(strFileExtension, sizeof(strFileExtension), L"%s", L"bmp"); // ���� Ȯ���� ���� ���ڿ�
                        if (ofn.nFileExtension == 0) // ����ڰ� Ȯ���ڸ� �Է����� ���� ���.
                        {
                            lstrcatW(strFile, L".bmp");
                        }
                        break;
                    case (DWORD)FILE_EXTENSION::PNG:
                        StringCbPrintf(strFileExtension, sizeof(strFileExtension), L"%s", L"png");
                        if (ofn.nFileExtension == 0) // ����ڰ� Ȯ���ڸ� �Է����� ���� ���.
                        {
                            lstrcatW(strFile, L".png");
                        }
                        break;
                    case (DWORD)FILE_EXTENSION::JPEG:
                        StringCbPrintf(strFileExtension, sizeof(strFileExtension), L"%s", L"jpeg");
                        if (ofn.nFileExtension == 0) // ����ڰ� Ȯ���ڸ� �Է����� ���� ���.
                        {
                            lstrcatW(strFile, L".jpeg");
                        }
                        break;
                    }

                    // ��Ʈ�� ���� -> �̹��� ���� ���� �Լ� ȣ��.
                    if (!SaveFile(strFile, strFileExtension, pBitmap))
                    {
                        MessageBox(hWnd, L"������ ������ �� �����ϴ�.", L"���� ���� ����", MB_OK);
                        break;
                    }
                    else
                    {
                        MessageBox(hWnd, L"������ ���������� �����߽��ϴ�!", L"���� ���� ����", MB_OK);
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
        if (pBitmap != nullptr) delete pBitmap; // ����� �������Ƿ�, �Ҵ� ���� Bitmap ��ü ����.
        if (hMemDC != 0) DeleteDC(hMemDC);      // ����� �������Ƿ�, ���� �� �޸� DC ����.
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
        fileNameStruct.lpstrTitle = L"������ ������ ������ �ּ���";
    }
    else fileNameStruct.lpstrTitle = L"������ ������ ������ �ּ���";

    fileNameStruct.lpstrFile = string;
    fileNameStruct.lpstrFilter = L"Bitmap(*.bmp)\0*.bmp\0png(*.png)\0*.png\0jpeg(*.jpeg)\0*.jpeg\0";
    fileNameStruct.nMaxFile = MAX_PATH;
    fileNameStruct.nMaxFileTitle = MAX_PATH;
}

// Gdiplus::Bitmap ��ü�� ���ϴ� �̹��� ���� ���Ϸ� �������ִ� �Լ�.
BOOL SaveFile(const WCHAR* filename, const WCHAR* fileExtension, Gdiplus::Bitmap* pBitmap)
{
    CLSID encoderID;        // �ش� �̹��� ���� encoder�� �ִ��� Ȯ���ϱ� ���� Ŭ���� ���̵�.

    UINT  num = 0;          // �̹��� encoder���� ����
    UINT  size = 0;         // �̹��� encoder �迭�� ����Ʈ ũ��

    WCHAR strFormat[32];
    StringCbPrintf(strFormat, sizeof(strFormat), L"image/%s", fileExtension);

    // ImageCodecInfo : ��ġ�� �̹��� encoder �� decoder(codec�̶� ��)�� ���� ������ �˻��ϴµ� �ʿ��� ��� �� �Լ����� ������.
    Gdiplus::ImageCodecInfo* pImageCodecInfo = nullptr;

    //	��� ������ image encoder���� ������ ImageCodecInfo ��ü �迭�� �� ũ�⸦ ����.
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
    {
        return FALSE;
    }

    // ImageCodecInfo �迭�� �� ũ�� ��ŭ ������ �Ҵ�.
    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == nullptr)
    {
        return FALSE;
    }

    // �̹��� encoder�� �迭�� ����.
    GetImageEncoders(num, size, pImageCodecInfo);

    // image encoder �迭�� ���鼭, ���̵� ���� ��Ʈ�� �񱳸� ���� ���� format�� 
    // image encoder�� ������, CLSID(COM ��ü �ĺ� ���� Ű)�� �Ѱ���. 
    for (UINT j = 0; j < num; ++j)
    {
        if (lstrcmpW(pImageCodecInfo[j].MimeType, strFormat) == 0) // ���� format�� encoder�� �߰��� ���
        {
            encoderID = pImageCodecInfo[j].Clsid; // pClsid�� Ŭ���� ���̵� �Ѱ���.
            free(pImageCodecInfo);                // �Ҵ� �޾Ҵ� ���� ����.
            break;
        }
        if (j == num - 1)                         // ���� ���� format�� image encoder�� ������,
        {
            free(pImageCodecInfo);                // �Ҵ� �޾Ҵ� ���� ����.
            return FALSE;
        }
    }

    // �츮�� ������ ���� ���� encoder Ŭ���� ���̵� �̿�, ���ϴ� ���� �������� �̹��� ������ ��������.
    if (pBitmap->Save(filename, &encoderID, nullptr) != Gdiplus::Status::Ok)
    {
        return FALSE;
    }

    return TRUE;
}