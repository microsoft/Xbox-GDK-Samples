//--------------------------------------------------------------------------------------
// FontSizeTest.cpp
// 
// Render text with different font sizes and styles to test how it looks on different
// displays and with different Windows settings.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <Windows.h>
#include <wrl.h>
#include <d2d1.h>
#include <dwrite_2.h>
#include <tchar.h>
#include <winrt/Windows.UI.ViewManagement.h>

#pragma comment(lib,"windowsapp.lib")

using namespace Microsoft::WRL;

// Text to display at different sizes
static const wchar_t* g_Text = L"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor";

// Fonts to use
constexpr int g_NumFonts = 6;
const wchar_t* g_FontName[g_NumFonts] = { L"Segoe UI", L"OCR A", L"Harrington", L"Kristen ITC", L"GOST Common", L"Departure Mono" };
int g_CurrentFontIndex = 0;

// Font sizes to use
constexpr int g_NumFontSizes = 8;
const int g_FontSize[g_NumFontSizes] = { 6, 8, 9, 11, 12, 14, 16, 24 };
ComPtr<IDWriteTextFormat> g_TextFormat[g_NumFontSizes];

// Settings
bool g_bDarkMode = false;
bool g_bUseBold = false;
bool g_bUseItalic = false;
bool g_bUseWindowsTextSize = false;

// Font scaling based on display DPI and Windows Text Size setting
float g_dpiScale = 1.0f;
float g_WindowsTextScaleFactor = 1.0f;

// Get the Windows Text Size setting
float GetTextScaleFactor()
{
    if (g_bUseWindowsTextSize)
    {
        winrt::Windows::UI::ViewManagement::UISettings uiSettings;
        return float(uiSettings.TextScaleFactor());
    }

    return 1.0f;
}

// Create the DirectWrite font formats
void CreateFontFormats(IDWriteFactory* dwFactory)
{
    for (int i = 0; i < g_NumFontSizes; i++)
    {
        dwFactory->CreateTextFormat(g_FontName[g_CurrentFontIndex], nullptr, g_bUseBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                                    g_bUseItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                    g_FontSize[i] * g_dpiScale * GetTextScaleFactor(), L"en-us", g_TextFormat[i].ReleaseAndGetAddressOf());
    }
}

// Class to render text using DirectWrite
class CDWriteTextRenderer final
{
private:
    IDWriteFactory* m_factory;
    ID2D1RenderTarget* m_renderTarget;
    IDWriteTextFormat* m_textFormat;
    ID2D1Brush* m_brush;

public:
    CDWriteTextRenderer(IDWriteFactory* factory, ID2D1RenderTarget* renderTarget, IDWriteTextFormat* textFormat, ID2D1Brush* brush) :
        m_factory(factory),
        m_renderTarget(renderTarget),
        m_textFormat(textFormat),
        m_brush(brush) {}

    void Draw(PCWSTR text, float posX, float posY)
    {
        auto size = m_renderTarget->GetSize();
        m_renderTarget->DrawText(text, static_cast<UINT>(wcslen(text)), m_textFormat, D2D1::RectF(posX, posY, size.width - 10.0f, size.height - 10.0f), m_brush);
    }
};

static constexpr LPCTSTR WINDOW_CLASS_NAME = _T("MyWindow");

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Main entry point for a Windows application
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,    _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int  nCmdShow)
{
    // Get screen width and height as scaled by Windows settings "Scale"
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Get screen width and height as Windows settings for "Display Resolution", which is normally what a game would render to in full screen mode
    DEVMODE dm = { 0 };
    dm.dmSize = sizeof(DEVMODE);
    if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm))
    {
        screenWidth = dm.dmPelsWidth;
        screenHeight = dm.dmPelsHeight;
    }

    WNDCLASS wndClass{ 0 };
    wndClass.lpszClassName = WINDOW_CLASS_NAME;
    wndClass.hInstance = hInstance;
    wndClass.lpfnWndProc = WndProc;
    RegisterClass(&wndClass);

    HWND hWnd = CreateWindow(WINDOW_CLASS_NAME,    _T("Font Size Test"), WS_POPUP | WS_VISIBLE,
                                0, 0, screenWidth, screenHeight, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    ShowCursor(FALSE);

    MSG msg;
    while (GetMessage(&msg, hWnd, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static ID2D1Factory* d2dFactory;
    static ID2D1HwndRenderTarget* renderTarget;
    static IDWriteFactory* dwFactory;
    static IDWriteRenderingParams* defaultParams;

    switch (msg)
    {
        case WM_CREATE:
        {
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory);
            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2), reinterpret_cast<IUnknown**>(&dwFactory));

#if _DEBUG
            IDWriteFontFallback* ff;
            reinterpret_cast<IDWriteFactory2*>(dwFactory)->GetSystemFontFallback(&ff);
#endif

            RECT rect;
            GetClientRect(hWnd, &rect);
            d2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, D2D1::SizeU(rect.right, rect.bottom)), &renderTarget);
            dwFactory->CreateRenderingParams(&defaultParams);

            g_WindowsTextScaleFactor = GetTextScaleFactor();
    
            CreateFontFormats(dwFactory);
            [[fallthrough]];
        }

        case WM_SIZE:
        {
            UINT32 width = LOWORD(lParam);
            UINT32 height = HIWORD(lParam);
            renderTarget->Resize(D2D1::SizeU(width, height));
            InvalidateRect(hWnd, nullptr, TRUE);
            return 0;
        }

        case WM_DPICHANGED:
        {
            UINT dpi = HIWORD(wParam);
            g_dpiScale = (float)dpi / USER_DEFAULT_SCREEN_DPI;
            CreateFontFormats(dwFactory);
            InvalidateRect(hWnd, nullptr, TRUE);
            return 0;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            renderTarget->BeginDraw();
            renderTarget->Clear(D2D1::ColorF(g_bDarkMode ? D2D1::ColorF::Black : D2D1::ColorF::White, 1.0f));

            ComPtr<ID2D1SolidColorBrush> brush;
            renderTarget->CreateSolidColorBrush(D2D1::ColorF(g_bDarkMode ? D2D1::ColorF::White : D2D1::ColorF::Black, 1.0f), brush.ReleaseAndGetAddressOf());

            ComPtr<IDWriteRenderingParams> paramsForClearType;
            dwFactory->CreateCustomRenderingParams(defaultParams->GetGamma(), defaultParams->GetEnhancedContrast(),    defaultParams->GetClearTypeLevel(),
                                                    defaultParams->GetPixelGeometry(), DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, paramsForClearType.ReleaseAndGetAddressOf());

            renderTarget->SetTextRenderingParams(paramsForClearType.Get());
            renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

            // Check if Windows settings for Text Size changed
            auto windowsTextScaleFactor = GetTextScaleFactor();
            
            if (windowsTextScaleFactor != g_WindowsTextScaleFactor)
            {
                g_WindowsTextScaleFactor = windowsTextScaleFactor;
                CreateFontFormats(dwFactory);
            }

            float posX = 10.0f;
            float posY = 10.0f;
            wchar_t buf[1024];

            // Show interesting info and instructions
            int fontIndex = g_NumFontSizes - 2;
            if (g_TextFormat[fontIndex].Get())
            {
                CDWriteTextRenderer renderer(dwFactory, renderTarget, g_TextFormat[fontIndex].Get(), brush.Get());

                // Show instructions
                {
                    swprintf_s(buf, L"Esc - Exit      S - Use Windows Text Size      D - Dark Mode      F - Font      B - Bold      I - Italic");

                    auto scaledWindowHeight = GetSystemMetrics(SM_CYSCREEN);
                    renderer.Draw(buf, posX, float(scaledWindowHeight - g_FontSize[fontIndex] * 2));
                }

                // Show interesting info
                {
                    RECT windowRect;
                    GetWindowRect(hWnd, &windowRect);

                    int windowWidth = windowRect.right - windowRect.left;
                    int windowHeight = windowRect.bottom - windowRect.top;

                    if (g_bUseWindowsTextSize)
                    {
                        swprintf_s(buf, L"DPI = %d      DIP Font Scale = %2.2f      Windows 'Text size' Scale = %2.2f      Resolution = %d x %d",
                            GetDpiForWindow(hWnd), g_dpiScale, GetTextScaleFactor(), windowWidth, windowHeight);
                    }
                    else
                    {
                        swprintf_s(buf, L"DPI = %d      DIP Font Scale = %2.2f      Windows 'Text size' Scale NOT Applied      Resolution = %d x %d",
                            GetDpiForWindow(hWnd), g_dpiScale, windowWidth, windowHeight);
                    }

                    renderer.Draw(buf, posX, posY);
                    posY += 50.0f;
                }
            }

            // Show test text
            for (int i = 0; i < g_NumFontSizes; i++)
            {
                if (g_TextFormat[i])
                {
                    CDWriteTextRenderer renderer(dwFactory, renderTarget, g_TextFormat[i].Get(), brush.Get());

                    swprintf_s(buf, L"%s %d DIP\n%s", g_FontName[g_CurrentFontIndex], g_FontSize[i], g_Text);
                    renderer.Draw(buf, posX, posY);
                    posY += g_FontSize[i] * 4.0f;
                }
            }

            renderTarget->EndDraw();
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE)
            {
                DestroyWindow(hWnd);
            }
            else if (wParam == 'S' || wParam == 's')
            {
                g_bUseWindowsTextSize = !g_bUseWindowsTextSize;
                InvalidateRect(hWnd, nullptr, TRUE);
            }
            else if (wParam == 'D' || wParam == 'd')
            {
                g_bDarkMode = !g_bDarkMode;
                InvalidateRect(hWnd, nullptr, TRUE);
            }
            else if (wParam == 'B' || wParam == 'b')
            {
                g_bUseBold = !g_bUseBold;
                CreateFontFormats(dwFactory);
                InvalidateRect(hWnd, nullptr, TRUE);
            }
            else if (wParam == 'I' || wParam == 'i')
            {
                g_bUseItalic = !g_bUseItalic;
                CreateFontFormats(dwFactory);
                InvalidateRect(hWnd, nullptr, TRUE);
            }
            else if (wParam == 'F' || wParam == 'f')
            {
                g_CurrentFontIndex = (g_CurrentFontIndex + 1) % g_NumFonts;
                CreateFontFormats(dwFactory);
                InvalidateRect(hWnd, nullptr, TRUE);
            }
            return 0;
        }

        case WM_QUIT:
            return 0;

        case WM_DESTROY:
        {
            for (int i = 0; i < g_NumFontSizes; i++)
            {
                g_TextFormat[i].Reset();
            }

            defaultParams->Release();
            renderTarget->Release();
            dwFactory->Release();
            d2dFactory->Release();
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}
