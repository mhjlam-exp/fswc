#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// if (0) = TRUE
// if (!0) = FALSE

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *surface = NULL;
SDL_Texture *texture = NULL;

char* capture(int w, int h)
{
    char *filename = "capture.bmp";

    // create compatible device context and bitmap from screen device context
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMemory = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen, w, h);

    // select compatible bitmap into compatible memory device context
    SelectObject(hdcMemory, hbmScreen);

    // bit blit screen device context to compatible memory device context
    BitBlt(hdcMemory, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);

    // get BITMAP from HBITMAP
    BITMAP bmpScreen;
    GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

    // bitmap info header
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

    HANDLE hDIB = GlobalAlloc(GHND,dwBmpSize);
    char *lpbitmap = (char*)GlobalLock(hDIB);

    // copy bits from the bitmap into a buffer which is pointed to by lpbitmap
    GetDIBits(hdcScreen, hbmScreen, 0, (UINT)bmpScreen.bmHeight, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    // create file for screen capture
    HANDLE file = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // bitmap file header
    BITMAPFILEHEADER bmfHeader;
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfType = 0x4D42; //BM

    // write bitmap to file
    WriteFile(file, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), NULL, NULL);
    WriteFile(file, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), NULL, NULL);
    WriteFile(file, (LPSTR)lpbitmap, dwBmpSize, NULL, NULL);

    // clean up
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);
    CloseHandle(file);
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemory);
    ReleaseDC(NULL, hdcScreen);

    return filename;
}

void terminate()
{
    // unload any resources
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char **argv)
{
    SDL_Event event;

    // register termination function
    atexit(terminate);

    // get screen dimensions
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    // save screen capture to file
    char* capfile = capture(w, h);

    // initialize SDL renderer and create texture with screen capture
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("", 0, 0, w, h, SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS);
    surface = SDL_LoadBMP(capfile);
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTextureFromSurface(renderer, surface);

    // show texture in full screen
    SDL_ShowWindow(window);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // get mouse position
    int mx, my, mc = 0;
    SDL_GetGlobalMouseState(&mx, &my);
    int msq = -sqrt(mx*mx + my*my);

    // exit prerequisites
    int motion = 0;
    int clicks = 0;

    while (1)
    {
        SDL_WaitEvent(&event);

        if (!motion && event.type == SDL_MOUSEMOTION)
        {
            // keep track of total traveled mouse motion distance
            msq += sqrt((event.motion.xrel * event.motion.xrel) + (event.motion.yrel * event.motion.yrel));
            if (msq > 4*w) motion = 1;
        }
        else if (!clicks && event.type == SDL_MOUSEBUTTONDOWN)
        {
            // keep track of total amount of mouse button clicks
            if (++mc > 4) clicks = 1;
        }

        // allow exit when requirements are met
        if (motion && clicks) return 0;
    }

    return 0;
}
