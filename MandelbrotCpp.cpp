// MandelbrotCpp.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "MandelbrotCpp.h"
#include "Point.h"
#include "colorConvert.h"

#include <vector>
#include <windowsx.h>
#include <thread>

#define MAX_LOADSTRING 100
void distributeCalculation(HWND hWnd);

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MANDELBROTCPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MANDELBROTCPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MANDELBROTCPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MANDELBROTCPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   distributeCalculation(hWnd);
   //calculateMandelbrot(xMin, xMax, yMin, yMax, maxIteration, widthScale, heightScale);
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

std::vector<Point> dataPoints;

double xMin = -3;
double xMax = 3;
double yMin = -3;
double yMax = 3;
int maxIteration = 1000;
double widthScale = 0;
double heightScale = 0;
int zoomLevel = 5;

double linearMap(double value, double low, double high, double newLow, double newHigh)
{
	return newLow + ((value - low) / (high - low)) * (newHigh - newLow);
}

void calculateMandelbrot(double xMin, double xMax, double yMin, double yMax, int maxIteration, double widthScale, double heightScale)
{
	for (double w = xMin; w < xMax; w += widthScale)
	{
		for (double h = yMin; h < yMax; h += heightScale)
		{
			double x = 0;
			double y = 0;
			int iteration = 0;

			while ((x * x + y * y <= 4) && (iteration < maxIteration))
			{
				double xtemp = (x * x - y * y) + w;
				y = ((2 * x) * y) + h;
				x = xtemp;
				iteration++;
			}

			if (iteration != maxIteration)
			{
				dataPoints.push_back({ w, h, iteration });
			}
			else
			{
				dataPoints.push_back({ w, h, -1 });
			}
		}
	}
}

void drawMandelbrot(HDC hdc, HWND hWnd)
{
	HPEN pen = CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
	//Get the size of the window
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int width = clientRect.right;
	int height = clientRect.bottom;

	//BITMAPV5HEADER bmh{ sizeof(BITMAPV5HEADER) , width, -height, 1, 32, BI_RGB, 0, 0, 0, 0, 0, };

	HBITMAP hbm = CreateCompatibleBitmap(hdc, width, height);

	if (!dataPoints.empty())
	{
		for (Point &p : dataPoints)
		{
			int newW = (int) linearMap(p.x, xMin, xMax, 0, width - 1);
			int newH = (int) linearMap(p.y, yMin, yMax, height - 1, 0);

			if (p.color != -1)
			{
				HsvColor hsvColor{ linearMap(p.color, 0, maxIteration, 0, 255), 255, 255 };
				RgbColor newColor = HsvToRgb(hsvColor);
				SetPixel(hdc, newW, newH, RGB(newColor.r, newColor.g, newColor.b));
			}
			else
			{
				SetPixel(hdc, newW, newH, RGB(0, 0, 0));
			}
		}
	}

	DeleteObject(pen);
}

int numberOfProcessors()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

void distributeCalculation(HWND hWnd)
{
	int numProcesses = 1;
	
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int width = clientRect.right;
	int height = clientRect.bottom;

	double widthScale = (xMax - xMin) / width;
	double heightScale = (yMax - yMin) / height;

	double totalLength = abs(xMin) + abs(xMax);
	double pieceLength = totalLength / numProcesses;

	for (int i = 0; i < numProcesses; i++)
	{
		double xMinNew = xMin + (pieceLength * i);
		double xMaxNew = xMin + (pieceLength * (i + 1));

		std::thread threadObject(calculateMandelbrot, xMinNew, xMaxNew, yMin, yMax, maxIteration, widthScale, heightScale);
		threadObject.join();

		//calculateMandelbrot(xMinNew, xMaxNew, yMin, yMax, maxIteration, widthScale, heightScale);
	}

	InvalidateRect(hWnd, nullptr, false);
}

void zoomIn(HWND hWnd, int x, int y, int zoomLevel)
{
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int width = clientRect.right;
	int height = clientRect.bottom;

	double xMouse = linearMap(x, 0, width, xMin, xMax);
	double yMouse = linearMap(y, 0, height, yMax, yMin);

	double xMinTemp = xMouse - ((xMax - xMin) / (zoomLevel * zoomLevel));
	double xMaxTemp = xMouse + ((xMax - xMin) / (zoomLevel * zoomLevel));
	double yMinTemp = yMouse - ((yMax - yMin) / (zoomLevel * zoomLevel));
	double yMaxTemp = yMouse + ((yMax - yMin) / (zoomLevel * zoomLevel));

	xMin = xMinTemp;
	xMax = xMaxTemp;
	yMin = yMinTemp;
	yMax = yMaxTemp;

	widthScale = (xMax - xMin) / width;
	heightScale = (yMax - yMin) / height;

	dataPoints.clear();
	calculateMandelbrot(xMin, xMax, yMin, yMax, maxIteration, widthScale, heightScale);
	InvalidateRect(hWnd, nullptr, true);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_LBUTTONDOWN:
		{
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);

			zoomIn(hWnd, xPos, yPos, zoomLevel);
		}
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
			//Draw Mandelbrot

			HBITMAP mandelbrotBitmap{ hdc, };
			drawMandelbrot(hdc, hWnd);
            
			EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
