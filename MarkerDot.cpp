#include <windows.h>
#pragma comment(lib, "msimg32.lib")

const int circle_radius = 20;
const int window_size = circle_radius * 2;
const double alpha_percent = 0.5;
const BYTE color_red = 255;
const BYTE color_green = 128;
const BYTE color_blue = 0;

bool is_dragging = false;
POINT drag_offset;

void update_window(HWND hwnd) {
    HDC screen_dc = GetDC(NULL);
    HDC mem_dc = CreateCompatibleDC(screen_dc);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = window_size;
    bmi.bmiHeader.biHeight = -window_size;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    BYTE* pixels;
    HBITMAP dib = CreateDIBSection(screen_dc, &bmi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
    HBITMAP old_bitmap = (HBITMAP)SelectObject(mem_dc, dib);

    // Clear to transparent
    memset(pixels, 0, window_size * window_size * 4);

    // Draw circle with pre-multiplied alpha
    BYTE alpha_value = (BYTE)(alpha_percent * 255);
    BYTE premult_red = (BYTE)(color_red * alpha_percent);
    BYTE premult_green = (BYTE)(color_green * alpha_percent);
    BYTE premult_blue = (BYTE)(color_blue * alpha_percent);

    for (int y = 0; y < window_size; y++) {
        for (int x = 0; x < window_size; x++) {
            int dx = x - circle_radius;
            int dy = y - circle_radius;
            if (dx * dx + dy * dy <= circle_radius * circle_radius) {
                int index = (y * window_size + x) * 4;
                pixels[index] = premult_blue;      // Blue * alpha
                pixels[index + 1] = premult_green; // Green * alpha  
                pixels[index + 2] = premult_red;   // Red * alpha
                pixels[index + 3] = alpha_value;   // Alpha
            }
        }
    }

    POINT pt_src = { 0, 0 };
    SIZE size = { window_size, window_size };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(hwnd, screen_dc, NULL, &size, mem_dc, &pt_src, 0, &blend, ULW_ALPHA);

    SelectObject(mem_dc, old_bitmap);
    DeleteObject(dib);
    DeleteDC(mem_dc);
    ReleaseDC(NULL, screen_dc);
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        update_window(hwnd);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);
        SetFocus(hwnd);
        is_dragging = true;

        POINT cursor_pos;
        GetCursorPos(&cursor_pos);
        RECT window_rect;
        GetWindowRect(hwnd, &window_rect);

        drag_offset.x = cursor_pos.x - window_rect.left;
        drag_offset.y = cursor_pos.y - window_rect.top;
        return 0;
    }

    case WM_LBUTTONUP:
        if (is_dragging) {
            ReleaseCapture();
            is_dragging = false;
        }
        return 0;

    case WM_MOUSEMOVE:
        if (is_dragging) {
            POINT cursor_pos;
            GetCursorPos(&cursor_pos);
            SetWindowPos(hwnd, NULL,
                cursor_pos.x - drag_offset.x,
                cursor_pos.y - drag_offset.y,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;

    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
            PostQuitMessage(0);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprev_instance, LPSTR cmd_line, int cmd_show) {
    const wchar_t* class_name = L"ScreenMarker";

    WNDCLASS wc = {};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = hinstance;
    wc.lpszClassName = class_name;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    RegisterClass(&wc);

    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    int start_x = (screen_width - window_size) / 2;
    int start_y = (screen_height - window_size) / 2;

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        class_name,
        L"Screen Marker",
        WS_POPUP,
        start_x, start_y, window_size, window_size,
        NULL, NULL, hinstance, NULL
    );

    ShowWindow(hwnd, cmd_show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}