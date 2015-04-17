#pragma comment(lib, "detours.lib")
#pragma comment(lib, "D3D10.lib")
#pragma comment(lib, "D3DX10.lib")

#include <Windows.h>
#include <stdio.h>

//D3D10 includes for DirectX 10/11
#include <d3dx10.h>
#include <d3d10misc.h>

//Use Detours API for example hook
#include "detours.h"

static HRESULT (WINAPI *pD3D10CreateDeviceAndSwapChain)(IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software,
    UINT Flags, UINT SDKVersion, DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, IDXGISwapChain **ppSwapChain,
    ID3D10Device **ppDevice) = D3D10CreateDeviceAndSwapChain;

typedef HRESULT (WINAPI *pPresent)(IDXGISwapChain *thisPtr, UINT SyncInterval, UINT Flags);

pPresent PresentAddress = NULL;

IDXGISwapChain *pSwapChain;
ID3D10Device *pDevice;
ID3DX10Font *pFont = NULL;

SIZE_T OutputCounter = 0;
const D3DXCOLOR RED(1.0f, 0.0f, 0.0f, 1.0f);

void CreateDrawingFont(void)
{
    D3DX10_FONT_DESC fontDesc;
    fontDesc.Height          = 24;
    fontDesc.Width           = 0;
    fontDesc.Weight          = 0;
    fontDesc.MipLevels       = 1;
    fontDesc.Italic          = false;
    fontDesc.CharSet         = DEFAULT_CHARSET;
    fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
    fontDesc.Quality         = DEFAULT_QUALITY;
    fontDesc.PitchAndFamily  = DEFAULT_PITCH | FF_DONTCARE;
    wcscpy(fontDesc.FaceName, L"Times New Roman");

    if(pDevice == NULL) 
    {
        printf("Device to create font on is NULL.\n");
        return;
    }

    D3DX10CreateFontIndirect(pDevice, &fontDesc, &pFont);
}

HRESULT WINAPI PresentHook(IDXGISwapChain *thisAddr, UINT SyncInterval, UINT Flags) {
    
    //printf("In Present (%X)\n", PresentAddress);

    RECT Rect = { 100, 100, 200, 200 };
    pFont->DrawTextW(NULL, L"Hello, World!", -1, &Rect, DT_CENTER | DT_NOCLIP, RED);
    return PresentAddress(thisAddr, SyncInterval, Flags);
}

HRESULT WINAPI D3D10CreateDeviceAndSwapChainHook(IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software,
    UINT Flags, UINT SDKVersion, DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, IDXGISwapChain **ppSwapChain,
    ID3D10Device **ppDevice)
{

    printf("In D3D10CreateDeviceAndSwapChainHook\n");

    //Create the device and swap chain
    HRESULT hResult = pD3D10CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags, SDKVersion,
        pSwapChainDesc, ppSwapChain, ppDevice);

    //Save the device and swap chain interface.
    //These aren't used in this example but are generally nice to have addresses to
    if(ppSwapChain == NULL)
    {
        printf("Swap chain is NULL.\n");
        return hResult;
    }
    else
    {
        pSwapChain = *ppSwapChain;
    }
    if(ppDevice == NULL)
    { 
        printf("Device is NULL.\n");
        return hResult;
    }
    else
    {
        pDevice = *ppDevice;
    }

    //Get the vtable address of the swap chain's Present function and modify it with our own.
    //Save it to return to later in our Present hook
    if(pSwapChain != NULL)
    {
        DWORD_PTR *SwapChainVTable = (DWORD_PTR *)pSwapChain;
        SwapChainVTable = (DWORD_PTR *)SwapChainVTable[0];
        printf("Swap chain VTable: %X\n", SwapChainVTable);
        PresentAddress = (pPresent)SwapChainVTable[8];
        printf("Present address: %X\n", PresentAddress);

        DWORD OldProtections = 0;
        VirtualProtect(&SwapChainVTable[8], sizeof(DWORD_PTR), PAGE_EXECUTE_READWRITE, &OldProtections);
        SwapChainVTable[8] = (DWORD_PTR)PresentHook;
        VirtualProtect(&SwapChainVTable[8], sizeof(DWORD_PTR), OldProtections, &OldProtections);
    }

    //Create the font that we will be drawing with
    CreateDrawingFont();

    return hResult;
}

void CreateConsole(void)
{
    if(AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        SetConsoleTitle(L"Debug Console");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        printf("DLL loaded at %X\n", GetModuleHandle(NULL));
    }
}

void InstallHook(void)
{
    (void)DetourTransactionBegin();
    (void)DetourUpdateThread(GetCurrentThread());
    LONG lResult = DetourAttach(&(PVOID&)pD3D10CreateDeviceAndSwapChain, D3D10CreateDeviceAndSwapChainHook);
    if (lResult != NO_ERROR)
    {
        printf("DetourAttach failed with %X\n", lResult);
        return;
    }
    lResult = DetourTransactionCommit();
    if (lResult != NO_ERROR)
    {
        printf("DetourTransactionCommit failed with %X\n", lResult);
        return;
    }
}

int APIENTRY DllMain(HMODULE hModule, DWORD Reason, LPVOID Reserved)
{
    switch(Reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateConsole();
        InstallHook();
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}