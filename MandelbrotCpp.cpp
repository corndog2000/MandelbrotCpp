// MandelbrotCpp.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "MandelbrotCpp.h"
#include "Point.h"
#include "colorConvert.h"
#include "resource.h"

#include <vector>
#include <windowsx.h>
#include <thread>
#include <wingdi.h>
#include <WinUser.h>

#define MAX_LOADSTRING 100
void distributeCalculation(HWND hWnd);
void recalculate(HWND hWnd);
void resetZoom();

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

   //Run resetZoom to set the xMin/xMax and yMin/yMax for the correct fractal algorithm
   resetZoom();
	
   //recalculate will run the correct algorithm based on useMandelbrotMath
   recalculate(hWnd);

   //Run the code at the end from the windows gui handler to draw the bitmap on the screen.
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

BOOL IsWindowMode = TRUE;
WINDOWPLACEMENT wpc;
LONG HWNDStyle = 0;
LONG HWNDStyleEx = 0;

void FullScreenSwitch(HWND hWnd)
{
	if (IsWindowMode)
	{
		IsWindowMode = FALSE;
		GetWindowPlacement(hWnd, &wpc);
		if (HWNDStyle == 0)
			HWNDStyle = GetWindowLong(hWnd, GWL_STYLE);
		if (HWNDStyleEx == 0)
			HWNDStyleEx = GetWindowLong(hWnd, GWL_EXSTYLE);

		LONG NewHWNDStyle = HWNDStyle;
		NewHWNDStyle &= ~WS_BORDER;
		NewHWNDStyle &= ~WS_DLGFRAME;
		NewHWNDStyle &= ~WS_THICKFRAME;

		LONG NewHWNDStyleEx = HWNDStyleEx;
		NewHWNDStyleEx &= ~WS_EX_WINDOWEDGE;

		SetWindowLong(hWnd, GWL_STYLE, NewHWNDStyle | WS_POPUP);
		SetWindowLong(hWnd, GWL_EXSTYLE, NewHWNDStyleEx | WS_EX_TOPMOST);
		ShowWindow(hWnd, SW_SHOWMAXIMIZED);
	}
	else
	{
		IsWindowMode = TRUE;
		SetWindowLong(hWnd, GWL_STYLE, HWNDStyle);
		SetWindowLong(hWnd, GWL_EXSTYLE, HWNDStyleEx);
		ShowWindow(hWnd, SW_SHOWNORMAL);
		SetWindowPlacement(hWnd, &wpc);
	}
}

//Array to hold data points that need to be drawn - DEPRECATED
//std::vector<Point> dataPoints;
//The bitmap object that holds the pixel data that is shown on in the main window
HBITMAP fractalBitmap = nullptr;
unsigned char* fractalColorData = nullptr;

//xMin, xMax, yMin, yMax are used in the calculation to determine the range of values the algorithm is applied to
double xMin;
double xMax;
double yMin;
double yMax;

//How many times through the loop to determine if a data point fits into the mandelbrot data set
int maxIteration = 500;
double widthScale = 0;
double heightScale = 0;
//How far to zoom when clicking on the screen. This value is squared in the zoom code
int zoomLevel = 3;

//If this is true then the program will draw the mandelbot data set. If it is false then it will draw the julia fractal data set.
bool useMandelbrotMath = true;


void resetZoom()
{
	if (useMandelbrotMath)
	{
		xMin = -3;
		xMax = 3;
		yMin = -3;
		yMax = 3;
	}
	else
	{
		xMin = -200;
		xMax = 200;
		yMin = -200;
		yMax = 200;
	}
}

//Function used for calculating the bitmap memory allocation size
int getStride(int width)
{
	int bpp = 32;
	int stride = (width * (bpp / 8));
	return stride;
}

//Function to easily get the client window size
std::pair<int, int> getClientSize(HWND hWnd)
{
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int width = clientRect.right;
	int height = clientRect.bottom;

	return std::make_pair(width, height );
}

//Function to map a value over a range of other values
double linearMap(double value, double low, double high, double newLow, double newHigh)
{
	return newLow + ((value - low) / (high - low)) * (newHigh - newLow);
}

void calculateMandelbrot(HWND hWnd, double xMin, double xMax, double yMin, double yMax, int maxIteration, double widthScale, double heightScale)
{
	if (fractalColorData == nullptr)
		return;

	auto [width, height] = getClientSize(hWnd);
	int stride = getStride(width);

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

			int newW = (int)linearMap(w, xMin, xMax, 0, width - 1);
			int newH = (int)linearMap(h, yMin, yMax, height - 1, 0);

			unsigned char* pixData = fractalColorData + (stride * newH) + (newW * 4);

			if (iteration != maxIteration)
			{
				HsvColor hsvColor{ linearMap(iteration, 0, maxIteration, 0, 255), 255, 255 };
				RgbColor newColor = HsvToRgb(hsvColor);

				pixData[0] = newColor.b;
				pixData[1] = newColor.g;
				pixData[2] = newColor.r;
				pixData[3] = 255;
			}
			else
			{
				pixData[0] = 0;
				pixData[1] = 0;
				pixData[2] = 0;
				pixData[3] = 255;
			}
		}
	}
}

void calculateJulia(HWND hWnd, double xMin, double xMax, double yMin, double yMax, int maxIteration, double widthScale, double heightScale)
{
	if (fractalColorData == nullptr)
		return;

	auto [width, height] = getClientSize(hWnd);
	int stride = getStride(width);

	//Julia specific calculation variables
	double C = -0.8;
	double D = 0.156;

	for (double w = xMin; w < xMax; w += widthScale)
	{
		for (double h = yMin; h < yMax; h += heightScale)
		{
			double x = (h - 32) / 100;
			double y = (w - 32) / 100;
			int iteration = 0;

			while (iteration < maxIteration)
			{
				double R = ((x * x) - (y * y)) + C;
				double I = (2 * (x * y)) + D;

				if (R * R > maxIteration * 2)
				{
					break;
				}
				else
				{
					x = R;
					y = I;
				}

				iteration++;
			}

			int newW = (int)linearMap(w, xMin, xMax, 0, width - 1);
			int newH = (int)linearMap(h, yMin, yMax, height - 1, 0);

			unsigned char* pixData = fractalColorData + (stride * newH) + (newW * 4);

			if (iteration != maxIteration)
			{
				HsvColor hsvColor{ linearMap(iteration, 0, maxIteration, 0, 255), 255, 255 };
				RgbColor newColor = HsvToRgb(hsvColor);

				pixData[0] = newColor.b;
				pixData[1] = newColor.g;
				pixData[2] = newColor.r;
				pixData[3] = 255;
			}
			else
			{
				pixData[0] = 0;
				pixData[1] = 0;
				pixData[2] = 0;
				pixData[3] = 255;
			}
		}
	}
}

void recalculate(HWND hWnd)
{
	auto [width, height] = getClientSize(hWnd);

	widthScale = (xMax - xMin) / width;
	heightScale = (yMax - yMin) / height;

	//dataPoints.clear();
	if (useMandelbrotMath)
	{
		calculateMandelbrot(hWnd, xMin, xMax, yMin, yMax, maxIteration, widthScale, heightScale);
		//distributeCalculation(hWnd);
	}
	else
	{
		calculateJulia(hWnd, xMin, xMax, yMin, yMax, maxIteration, widthScale, heightScale);
		//distributeCalculation(hWnd);
	}
	InvalidateRect(hWnd, nullptr, true);
}

HBITMAP createMandelbrotBitmap(int width, int height, unsigned char*& bits)
{
	int stride = getStride(width);
	int byteCount = (stride * height);
	HANDLE hMem = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, byteCount, nullptr);

	BITMAPV5HEADER bmh = { sizeof(BITMAPV5HEADER), width, -height, 1, 32, BI_RGB, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF };
	HDC hdcMem = ::CreateCompatibleDC(nullptr);

	HBITMAP hbitmap = CreateDIBSection(hdcMem, reinterpret_cast<BITMAPINFO*>(&bmh), DIB_RGB_COLORS, (void**)&bits, NULL, 0);
	DeleteObject(hdcMem);
	return hbitmap;
}

void drawMandelbrot(HDC hdc, HWND hWnd)
{
	//Get the size of the window
	auto [width, height] = getClientSize(hWnd);

	/*
	int stride = getStride(width);
	if (!dataPoints.empty())
	{
		for (Point& p : dataPoints)
		{
			int newW = (int)linearMap(p.x, xMin, xMax, 0, width - 1);
			int newH = (int)linearMap(p.y, yMin, yMax, height - 1, 0);

			unsigned char* pixData = mandelbrotColorData + (stride * newH) + (newW * 4);

			if (p.color != -1)
			{
				HsvColor hsvColor{ linearMap(p.color, 0, maxIteration, 0, 255), 255, 255 };
				RgbColor newColor = HsvToRgb(hsvColor);

				pixData[0] = newColor.b;
				pixData[1] = newColor.g;
				pixData[2] = newColor.r;
				pixData[3] = 255;
			}
			else
			{
				pixData[0] = 0;
				pixData[1] = 0;
				pixData[2] = 0;
				pixData[3] = 255;
			}
		}
	}
	*/

	HDC hdcMem = ::CreateCompatibleDC(nullptr);

	auto oldBitmap = SelectObject(hdcMem, fractalBitmap);

	BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

	SelectObject(hdcMem, oldBitmap);

	DeleteObject(hdcMem);
}

int numberOfProcessors()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

//NOTICE - I'm not using the multiprocess code. It doesn't seem to be faster than using 1 thread.
void distributeCalculation(HWND hWnd)
{
	//How many processes of the algorithm to run.
	int numProcesses = 1;
	
	auto [width, height] = getClientSize(hWnd);

	double widthScale = (xMax - xMin) / width;
	double heightScale = (yMax - yMin) / height;

	double totalLength = abs(xMin) + abs(xMax);
	double pieceLength = totalLength / numProcesses;

	for (int i = 0; i < numProcesses; i++)
	{
		double xMinNew = xMin + (pieceLength * i);
		double xMaxNew = xMin + (pieceLength * (i + 1));

		if (useMandelbrotMath)
		{
			std::thread threadObject(calculateMandelbrot, hWnd, xMinNew, xMaxNew, yMin, yMax, maxIteration, widthScale, heightScale);
			threadObject.join();
		}
		else
		{
			std::thread threadObject(calculateJulia, hWnd, xMinNew, xMaxNew, yMin, yMax, maxIteration, widthScale, heightScale);
			threadObject.join();
		}
	}

	InvalidateRect(hWnd, nullptr, false);

}

void zoomIn(HWND hWnd, int x, int y, int zoomLevel)
{
	auto [width, height] = getClientSize(hWnd);

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

	recalculate(hWnd);
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
			case ID_ALGORITHM_MANDELBROT:
			{
				useMandelbrotMath = true;
				resetZoom();
				recalculate(hWnd);
			}
				break;
			case ID_ALGORITHM_JULIA:
			{
				useMandelbrotMath = false;
				resetZoom();
				recalculate(hWnd);
			}
				break;
			case ID_FILE_RESETZOOM:
				resetZoom();
				recalculate(hWnd);
				break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_KEYDOWN:
	{
		if (GetKeyState('M') & 0x8000)
		{
			bool calculating = false;

			if (useMandelbrotMath && !calculating)
			{
				calculating = true;
				useMandelbrotMath = false;
			}
			else if (!useMandelbrotMath && !calculating)
			{
				calculating = true;
				useMandelbrotMath = true;
			}

			resetZoom();
			recalculate(hWnd);

			calculating = false;
		}

		if (GetKeyState('R') & 0x8000)
		{
			resetZoom();
			recalculate(hWnd);
		}

		if (GetKeyState('F') & 0x8000 || GetKeyState(VK_F11) & 0x8000)
		{
			FullScreenSwitch(hWnd);
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
	case WM_SIZE:
		{
		auto [width, height] = getClientSize(hWnd);
		
		if (fractalBitmap != nullptr)
		{
			DeleteObject(fractalBitmap);
		}

		fractalBitmap = createMandelbrotBitmap(width, height, fractalColorData);
		recalculate(hWnd);
		}
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
			//Draw Mandelbrot
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
