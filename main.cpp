#include <directsr.h>
#include <d3dx12.h>
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN // �� Windows ͷ���ų�����ʹ�õ�����
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <wrl.h>		//���WTL֧�� ����ʹ��COM
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3d12.h>       //for d3d12
#include <d3dcompiler.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#include <wincodec.h>   //for WIC
#include <atlbase.h>
#include <map>

using namespace Microsoft;
using namespace Microsoft::WRL;
using namespace DirectX;

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 715; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }
//linker
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define GRS_WND_CLASS_NAME _T("GRS Game Window Class")
#define GRS_WND_TITLE	_T("GRS DirectX12 Texture Sample")
#define GRS_THROW_IF_FAILED(hr) {HRESULT _hr = (hr);if (FAILED(_hr)){ throw CGRSCOMException(_hr); }}

class CGRSCOMException
{
public:
    CGRSCOMException(HRESULT hr) : m_hrError(hr)
    {
    }
    HRESULT Error() const
    {
        return m_hrError;
    }
private:
    const HRESULT m_hrError;
};

struct WICTranslate
{
    GUID wic;
    DXGI_FORMAT format;
};

static WICTranslate g_WICFormats[] =
{//WIC��ʽ��DXGI���ظ�ʽ�Ķ�Ӧ���ñ��еĸ�ʽΪ��֧�ֵĸ�ʽ
    { GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },

    { GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
    { GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },

    { GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
    { GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
    { GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1

    { GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
    { GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },

    { GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
    { GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },

    { GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
    { GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
    { GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
    { GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },

    { GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM },
};

// WIC ���ظ�ʽת����.
struct WICConvert
{
    GUID source;
    GUID target;
};

static WICConvert g_WICConvert[] =
{
    // Ŀ���ʽһ������ӽ��ı�֧�ֵĸ�ʽ
    { GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

    { GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

    { GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM
    { GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

    { GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT
    { GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT

    { GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM

    { GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM

    { GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

    { GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

    { GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
    { GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT

    { GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
    { GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
    { GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
    { GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
    { GUID_WICPixelFormat32bppRGBE,             GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT

    { GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

    { GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
};

// ���ȷ�����ݵ���ӽ���ʽ���ĸ�
bool GetTargetPixelFormat(const GUID* pSourceFormat, GUID* pTargetFormat)
{
    *pTargetFormat = *pSourceFormat;
    for (size_t i = 0; i < _countof(g_WICConvert); ++i)
    {
        if (InlineIsEqualGUID(g_WICConvert[i].source, *pSourceFormat))
        {
            *pTargetFormat = g_WICConvert[i].target;
            return true;
        }
    }
    return false;
}

// ���ȷ�����ն�Ӧ��DXGI��ʽ����һ��
DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID* pPixelFormat)
{
    for (size_t i = 0; i < _countof(g_WICFormats); ++i)
    {
        if (InlineIsEqualGUID(g_WICFormats[i].wic, *pPixelFormat))
        {
            return g_WICFormats[i].format;
        }
    }
    return DXGI_FORMAT_UNKNOWN;
}

struct GRS_VERTEX
{
    XMFLOAT4 m_v4Position;		//Position
    XMFLOAT2 m_vTxc;		    //Texcoord
};

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    ::CoInitialize(nullptr);  //for WIC & COM

    const UINT nFrameBackBufCount = 3u;
    UINT                                iTime = 20;
    UINT								sWidth = 100;
    UINT								sHeight = 89;
    UINT								iWidth = sWidth * iTime;
    UINT								iHeight = sHeight * iTime;
    UINT								tWidth = sWidth * iTime;
    UINT								tHeight = sHeight * iTime;
    UINT								nFrameIndex = 0;
    DXGI_FORMAT				            emRenderTarget = DXGI_FORMAT_R8G8B8A8_UNORM;
    const float						    faClearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    UINT								nDXGIFactoryFlags = 0U;
    UINT								nRTVDescriptorSize = 0U;
    HWND								hWnd = nullptr;
    MSG									msg = {};
    TCHAR								pszAppPath[MAX_PATH] = {};
    float								fAspectRatio = 3.0f;
    D3D12_VERTEX_BUFFER_VIEW			stVertexBufferView = {};
    UINT64								n64FenceValue = 0ui64;
    HANDLE								hEventFence = nullptr;
    UINT								nTextureW = 0u;
    UINT								nTextureH = 0u;
    UINT								nBPP = 0u;
    DXGI_FORMAT							stTextureFormat = DXGI_FORMAT_UNKNOWN;
    D3D12_VIEWPORT						stViewPort = { 0.0f, 0.0f, static_cast<float>(iWidth), static_cast<float>(iHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
    D3D12_RECT							stScissorRect = { 0, 0, static_cast<LONG>(iWidth), static_cast<LONG>(iHeight) };

    ComPtr<IDXGIFactory5>				pIDXGIFactory5;
    ComPtr<IDXGIAdapter1>				pIAdapter1;
    ComPtr<ID3D12Device>				pID3D12Device4;
    ComPtr<ID3D12CommandQueue>			pICMDQueue;
    ComPtr<IDXGISwapChain1>				pISwapChain1;
    ComPtr<IDXGISwapChain3>				pISwapChain3;
    ComPtr<ID3D12DescriptorHeap>		pIRTVHeap;
    ComPtr<ID3D12DescriptorHeap>		pISRVHeap;
    ComPtr<ID3D12DescriptorHeap>		pDsvHeap;
    ComPtr<ID3D12DescriptorHeap>		pMvHeap;
    ComPtr<ID3D12Resource>				pIARenderTargets[nFrameBackBufCount];
    ComPtr<ID3D12Resource>				pITexture;
    ComPtr<ID3D12Resource>				pITargetTexture;
    ComPtr<ID3D12Resource>				pITextureUpload;
    ComPtr<ID3D12CommandAllocator>		pICMDAlloc;
    ComPtr<ID3D12GraphicsCommandList>	pICMDList;
    ComPtr<ID3D12RootSignature>			pIRootSignature;
    ComPtr<ID3D12PipelineState>			pIPipelineState;
    ComPtr<ID3D12Resource>				pIVertexBuffer;
    ComPtr<ID3D12Fence>					pIFence;
    ComPtr<IWICImagingFactory>			pIWICFactory;
    ComPtr<IWICBitmapDecoder>			pIWICDecoder;
    ComPtr<IWICBitmapFrameDecode>		pIWICFrame;
    ComPtr<ID3D12Resource>              mDepthStencilBuffer;
    ComPtr<ID3D12Resource>              motionVectorTexture;

    ATL::CComPtr<ID3D12SDKConfiguration> configuration;
    ATL::CComPtr<ID3D12DSRDeviceFactory> pDSRDeviceFactory;
    ATL::CComPtr<IDSRDevice> pDSRDevice;
    std::map<UINT, DSR_SUPERRES_VARIANT_DESC> g_myDsrDescMap;
    ATL::CComPtr<IDSRSuperResEngine> pSREngine;
    ATL::CComPtr<IDSRSuperResUpscaler> pSRUpscaler;

    HRESULT hsr;

    D3D12_RESOURCE_DESC					stTextureDesc = {};
    D3D12_RESOURCE_DESC					stTargetTextureDesc = {};

    UINT								nVertexCnt = 0;
    try
    {
        // ָ�� D3D12 SDK �汾
        {
            hsr = D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(&configuration));
            hsr = configuration->SetSDKVersion(D3D12SDKVersion, D3D12SDKPath);
        }

        // �õ���ǰ�Ĺ���Ŀ¼����������ʹ�����·�������ʸ�����Դ�ļ�
        {
            if (0 == ::GetModuleFileName(nullptr, pszAppPath, MAX_PATH))
            {
                GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
            }

            WCHAR* lastSlash = _tcsrchr(pszAppPath, _T('\\'));
            if (lastSlash)
            {//ɾ��Exe�ļ���
                *(lastSlash) = _T('\0');
            }

            lastSlash = _tcsrchr(pszAppPath, _T('\\'));
            if (lastSlash)
            {//ɾ��x64·��
                *(lastSlash) = _T('\0');
            }

            lastSlash = _tcsrchr(pszAppPath, _T('\\'));
            if (lastSlash)
            {//ɾ��Debug �� Release·��
                *(lastSlash + 1) = _T('\0');
            }
        }

        // ��������
        {
            WNDCLASSEX wcex = {};
            wcex.cbSize = sizeof(WNDCLASSEX);
            wcex.style = CS_GLOBALCLASS;
            wcex.lpfnWndProc = WndProc;
            wcex.cbClsExtra = 0;
            wcex.cbWndExtra = 0;
            wcex.hInstance = hInstance;
            wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);		//��ֹ���ĵı����ػ�
            wcex.lpszClassName = GRS_WND_CLASS_NAME;
            RegisterClassEx(&wcex);

            DWORD dwWndStyle = WS_OVERLAPPED | WS_SYSMENU;
            RECT rtWnd = { 0, 0, iWidth, iHeight };
            AdjustWindowRect(&rtWnd, dwWndStyle, FALSE);

            // ���㴰�ھ��е���Ļ����
            INT posX = (GetSystemMetrics(SM_CXSCREEN) - rtWnd.right - rtWnd.left) / 2;
            INT posY = (GetSystemMetrics(SM_CYSCREEN) - rtWnd.bottom - rtWnd.top) / 2;

            hWnd = CreateWindowW(GRS_WND_CLASS_NAME
                , GRS_WND_TITLE
                , dwWndStyle
                , posX
                , posY
                , rtWnd.right - rtWnd.left
                , rtWnd.bottom - rtWnd.top
                , nullptr
                , nullptr
                , hInstance
                , nullptr);

            if (!hWnd)
            {
                return FALSE;
            }
        }

        // ����ʾ��ϵͳ�ĵ���֧��
        {
#if defined(_DEBUG)
            {
                ComPtr<ID3D12Debug> debugController;
                if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
                {
                    debugController->EnableDebugLayer();
                    // �򿪸��ӵĵ���֧��
                    nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
                }
            }
#endif
        //---------------------------------------------------------------------------------------------
        }

        // ����DXGI Factory����
        {
            GRS_THROW_IF_FAILED(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(&pIDXGIFactory5)));
        }

        // ����DXGI DirectSR Factory����
        {
            hsr = D3D12GetInterface(CLSID_D3D12DSRDeviceFactory, IID_PPV_ARGS(&pDSRDeviceFactory));
        }

        // ö������������ѡ����ʵ�������������3D�豸����
        {
            DXGI_ADAPTER_DESC1 stAdapterDesc = {};
            for (UINT nAdapterIndex = 0; DXGI_ERROR_NOT_FOUND != pIDXGIFactory5->EnumAdapters1(nAdapterIndex, &pIAdapter1); ++nAdapterIndex)
            {
                pIAdapter1->GetDesc1(&stAdapterDesc);

                if (stAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {//������������������豸
                    continue;
                }
                //�����������D3D֧�ֵļ��ݼ�������ֱ��Ҫ��֧��12.1��������ע�ⷵ�ؽӿڵ��Ǹ���������Ϊ��nullptr������
                //�Ͳ���ʵ�ʴ���һ���豸�ˣ�Ҳ�������ǆ��µ��ٵ���release���ͷŽӿڡ���Ҳ��һ����Ҫ�ļ��ɣ����ס��
                if (SUCCEEDED(D3D12CreateDevice(pIAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
            // ����D3D12.1���豸
            GRS_THROW_IF_FAILED(D3D12CreateDevice(pIAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pID3D12Device4)));

            // ���ڱ�ǩ
            TCHAR pszWndTitle[MAX_PATH] = {};
            GRS_THROW_IF_FAILED(pIAdapter1->GetDesc1(&stAdapterDesc));
            ::GetWindowText(hWnd, pszWndTitle, MAX_PATH);
            StringCchPrintf(pszWndTitle
                , MAX_PATH
                , _T("%s (GPU:%s)")
                , pszWndTitle
                , stAdapterDesc.Description);
            ::SetWindowText(hWnd, pszWndTitle);
        }

        // ����ֱ���������
        {
            D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
            stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&pICMDQueue)));
        }

        // ���� DirectSR SRDevice�豸���������ֱ�������ʱҪ��ǰ��������Դ������SRV+UAV
        {
            hsr = pDSRDeviceFactory->CreateDSRDevice(pID3D12Device4.Get(), 1, IID_PPV_ARGS(&pDSRDevice));

            // Enumerate all super resolution variants available on the device.
            const UINT dsrVariantCount = pDSRDevice->GetNumSuperResVariants();
            for (UINT currentVariantIndex = 0; currentVariantIndex < dsrVariantCount; currentVariantIndex++)
            {
                DSR_SUPERRES_VARIANT_DESC variantDesc;
                // AMD FidelityFX Super Resolution 3.1
                hsr = pDSRDevice->GetSuperResVariantDesc(currentVariantIndex, &variantDesc);
                g_myDsrDescMap.emplace(currentVariantIndex, variantDesc);
            }
        }

        // ����������
        {
            DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
            stSwapChainDesc.BufferCount = nFrameBackBufCount;
            stSwapChainDesc.Width = iWidth;
            stSwapChainDesc.Height = iHeight;
            stSwapChainDesc.Format = emRenderTarget;
            stSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            stSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            stSwapChainDesc.SampleDesc.Count = 1;

            GRS_THROW_IF_FAILED(pIDXGIFactory5->CreateSwapChainForHwnd(
                pICMDQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
                hWnd,
                &stSwapChainDesc,
                nullptr,
                nullptr,
                &pISwapChain1
            ));

            //�õ���ǰ�󻺳�������ţ�Ҳ������һ����Ҫ������ʾ�Ļ����������
            //ע��˴�ʹ���˸߰汾��SwapChain�ӿڵĺ���
            GRS_THROW_IF_FAILED(pISwapChain1.As(&pISwapChain3));
            nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

            //����RTV(��ȾĿ����ͼ)��������(����ѵĺ���Ӧ�����Ϊ������߹̶���СԪ�صĹ̶���С�Դ��)
            D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
            stRTVHeapDesc.NumDescriptors = nFrameBackBufCount;
            stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            GRS_THROW_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&pIRTVHeap)));
            //�õ�ÿ��������Ԫ�صĴ�С
            nRTVDescriptorSize = pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            //����RTV��������
            D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
            for (UINT i = 0; i < nFrameBackBufCount; i++)
            {
                GRS_THROW_IF_FAILED(pISwapChain3->GetBuffer(i, IID_PPV_ARGS(&pIARenderTargets[i])));
                pID3D12Device4->CreateRenderTargetView(pIARenderTargets[i].Get(), nullptr, stRTVHandle);
                stRTVHandle.ptr += nRTVDescriptorSize;
            }
            // �ر�ALT+ENTER���л�ȫ���Ĺ��ܣ���Ϊ����û��ʵ��OnSize���������ȹر�
            GRS_THROW_IF_FAILED(pIDXGIFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
        }

        // ����SRV�� (Shader Resource View Heap)
        {
            D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
            stSRVHeapDesc.NumDescriptors = 1;
            stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            GRS_THROW_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stSRVHeapDesc, IID_PPV_ARGS(&pISRVHeap)));
        }

        // ������������
        {
            D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
            // ����Ƿ�֧��V1.1�汾�ĸ�ǩ��
            stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
            if (FAILED(pID3D12Device4->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
            {
                stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }
            // ��GPU��ִ��SetGraphicsRootDescriptorTable�����ǲ��޸������б��е�SRV��������ǿ���ʹ��Ĭ��Rang��Ϊ:
            // D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
            D3D12_DESCRIPTOR_RANGE1 stDSPRanges1[1] = {};
            stDSPRanges1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            stDSPRanges1[0].NumDescriptors = 1;
            stDSPRanges1[0].BaseShaderRegister = 0;
            stDSPRanges1[0].RegisterSpace = 0;
            stDSPRanges1[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
            stDSPRanges1[0].OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 stRootParameters1[1] = {};
            stRootParameters1[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            stRootParameters1[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            stRootParameters1[0].DescriptorTable.NumDescriptorRanges = _countof(stDSPRanges1);
            stRootParameters1[0].DescriptorTable.pDescriptorRanges = stDSPRanges1;

            D3D12_STATIC_SAMPLER_DESC stSamplerDesc[1] = {};
            stSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            stSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            stSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            stSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            stSamplerDesc[0].MipLODBias = 0;
            stSamplerDesc[0].MaxAnisotropy = 0;
            stSamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            stSamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            stSamplerDesc[0].MinLOD = 0.0f;
            stSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
            stSamplerDesc[0].ShaderRegister = 0;
            stSamplerDesc[0].RegisterSpace = 0;
            stSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};

            if (D3D_ROOT_SIGNATURE_VERSION_1_1 == stFeatureData.HighestVersion)
            {
                stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
                stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
                stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters1);
                stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters1;
                stRootSignatureDesc.Desc_1_1.NumStaticSamplers = _countof(stSamplerDesc);
                stRootSignatureDesc.Desc_1_1.pStaticSamplers = stSamplerDesc;
            }
            else
            {
                D3D12_DESCRIPTOR_RANGE stDSPRanges[1] = {};
                stDSPRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                stDSPRanges[0].NumDescriptors = 1;
                stDSPRanges[0].BaseShaderRegister = 0;
                stDSPRanges[0].RegisterSpace = 0;
                stDSPRanges[0].OffsetInDescriptorsFromTableStart = 0;

                D3D12_ROOT_PARAMETER stRootParameters[1] = {};
                stRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                stRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                stRootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(stDSPRanges);
                stRootParameters[0].DescriptorTable.pDescriptorRanges = stDSPRanges;

                stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
                stRootSignatureDesc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
                stRootSignatureDesc.Desc_1_0.NumParameters = _countof(stRootParameters);
                stRootSignatureDesc.Desc_1_0.pParameters = stRootParameters;
                stRootSignatureDesc.Desc_1_0.NumStaticSamplers = _countof(stSamplerDesc);
                stRootSignatureDesc.Desc_1_0.pStaticSamplers = stSamplerDesc;
            }

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;
            GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
                , &pISignatureBlob
                , &pIErrorBlob));

            GRS_THROW_IF_FAILED(pID3D12Device4->CreateRootSignature(0
                , pISignatureBlob->GetBufferPointer()
                , pISignatureBlob->GetBufferSize()
                , IID_PPV_ARGS(&pIRootSignature)));
        }

        // ���������б������
        {
            GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pICMDAlloc)));
            // ����ͼ�������б�
            GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pICMDAlloc.Get(), pIPipelineState.Get(), IID_PPV_ARGS(&pICMDList)));
        }

        // 1. ���������Դ
        // �������/ģ�建����������ͼ
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;
        hsr = pID3D12Device4->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(pDsvHeap.GetAddressOf()));

        D3D12_RESOURCE_DESC depthStencilDesc;
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = iWidth;
        depthStencilDesc.Height = iHeight;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT; //DXGI_FORMAT_R32_FLOAT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE optClear;
        optClear.Format = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_R32_FLOAT;
        optClear.DepthStencil.Depth = 1.0f;
        optClear.DepthStencil.Stencil = 0;
        hsr = pID3D12Device4->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_COMMON,
            &optClear,
            IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())
        );

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_R32_FLOAT;
        dsvDesc.Texture2D.MipSlice = 0;
        // ���ô���Դ�ĸ�ʽ��Ϊ������Դ�ĵ�0 mip�㴴��������
        pID3D12Device4->CreateDepthStencilView(
            mDepthStencilBuffer.Get(),
            &dsvDesc,
            pDsvHeap.Get()->GetCPUDescriptorHandleForHeapStart()
        );

        // ����Դ�ӳ�ʼ״̬ת��Ϊ��Ȼ�����
        pICMDList->ResourceBarrier(
            1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                mDepthStencilBuffer.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_DEPTH_WRITE
            )
        );

        // ����Shader������Ⱦ����״̬����
        {
            ComPtr<ID3DBlob> pIBlobVertexShader;
            ComPtr<ID3DBlob> pIBlobPixelShader;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            UINT compileFlags = 0;
#endif
            TCHAR pszShaderFileName[MAX_PATH] = {};
            StringCchPrintf(pszShaderFileName, MAX_PATH, _T("%sShader\\Texture.hlsl"), pszAppPath);

            GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
                , "VSMain", "vs_5_0", compileFlags, 0, &pIBlobVertexShader, nullptr));
            GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
                , "PSMain", "ps_5_0", compileFlags, 0, &pIBlobPixelShader, nullptr));

            // Define the vertex input layout.
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            // ���� graphics pipeline state object (PSO)����
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };
            stPSODesc.pRootSignature = pIRootSignature.Get();

            stPSODesc.VS.pShaderBytecode = pIBlobVertexShader->GetBufferPointer();
            stPSODesc.VS.BytecodeLength = pIBlobVertexShader->GetBufferSize();

            stPSODesc.PS.pShaderBytecode = pIBlobPixelShader->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIBlobPixelShader->GetBufferSize();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;
            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            //stPSODesc.DepthStencilState.DepthEnable = TRUE;
            stPSODesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
            stPSODesc.DepthStencilState.DepthEnable = FALSE;
            stPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;
            //stPSODesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
            stPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = emRenderTarget;
            stPSODesc.SampleDesc.Count = 1;

            GRS_THROW_IF_FAILED(pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc, IID_PPV_ARGS(&pIPipelineState)));
        }

        // �������㻺��
        {
            // ���������ε�3D���ݽṹ
            GRS_VERTEX stTriangleVertices[] =
            {
                { { -1, -1, 0.0f, 1.0f}, { 0.0f, 1.0f } },	// Bottom left.
                { { -1, 1, 0.0f, 1.0f}, { 0.0f, 0.0f }  },	// Top left.
                { { 1, -1, 0.0f, 1.0f }, { 1.0f, 1.0f } },	// Bottom right.
                { { 1, 1, 0.0f, 1.0f},  { 1.0f, 0.0f }   },		// Top right.
            };
            const UINT nVertexBufferSize = sizeof(stTriangleVertices);

            nVertexCnt = _countof(stTriangleVertices);

            D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_UPLOAD };

            D3D12_RESOURCE_DESC stResSesc = {};
            stResSesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            stResSesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            stResSesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            stResSesc.Format = DXGI_FORMAT_UNKNOWN;
            stResSesc.Width = nVertexBufferSize;
            stResSesc.Height = 1;
            stResSesc.DepthOrArraySize = 1;
            stResSesc.MipLevels = 1;
            stResSesc.SampleDesc.Count = 1;
            stResSesc.SampleDesc.Quality = 0;

            GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
                &stHeapProp,
                D3D12_HEAP_FLAG_NONE,
                &stResSesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&pIVertexBuffer)));

            UINT8* pVertexDataBegin = nullptr;
            D3D12_RANGE stReadRange = { 0, 0 };		// We do not intend to read from this resource on the CPU.
            GRS_THROW_IF_FAILED(pIVertexBuffer->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, stTriangleVertices, sizeof(stTriangleVertices));
            pIVertexBuffer->Unmap(0, nullptr);

            stVertexBufferView.BufferLocation = pIVertexBuffer->GetGPUVirtualAddress();
            stVertexBufferView.StrideInBytes = sizeof(GRS_VERTEX);
            stVertexBufferView.SizeInBytes = nVertexBufferSize;
        }

        // ʹ��WIC����������һ��2D����
        {
            //ʹ�ô�COM��ʽ����WIC�೧����Ҳ�ǵ���WIC��һ��Ҫ��������
            GRS_THROW_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIWICFactory)));

            //ʹ��WIC�೧����ӿڼ�������ͼƬ�����õ�һ��WIC����������ӿڣ�ͼƬ��Ϣ��������ӿڴ���Ķ�������
            WCHAR pszTexcuteFileName[MAX_PATH] = {};
            StringCchPrintfW(pszTexcuteFileName, MAX_PATH, _T("%sImages\\test1.jpg"), pszAppPath);

            GRS_THROW_IF_FAILED(pIWICFactory->CreateDecoderFromFilename(
                pszTexcuteFileName,              // �ļ���
                NULL,                            // ��ָ����������ʹ��Ĭ��
                GENERIC_READ,                    // ����Ȩ��
                WICDecodeMetadataCacheOnDemand,  // ����Ҫ�ͻ������� 
                &pIWICDecoder                    // ����������
            ));

            // ��ȡ��һ֡ͼƬ(��ΪGIF�ȸ�ʽ�ļ����ܻ��ж�֡ͼƬ�������ĸ�ʽһ��ֻ��һ֡ͼƬ)
            // ʵ�ʽ���������������λͼ��ʽ����
            GRS_THROW_IF_FAILED(pIWICDecoder->GetFrame(0, &pIWICFrame));

            WICPixelFormatGUID wpf = {};
            //��ȡWICͼƬ��ʽ
            GRS_THROW_IF_FAILED(pIWICFrame->GetPixelFormat(&wpf));
            GUID tgFormat = {};

            //ͨ����һ��ת��֮���ȡDXGI�ĵȼ۸�ʽ
            if (GetTargetPixelFormat(&wpf, &tgFormat))
            {
                stTextureFormat = GetDXGIFormatFromPixelFormat(&tgFormat);
            }

            if (DXGI_FORMAT_UNKNOWN == stTextureFormat)
            {// ��֧�ֵ�ͼƬ��ʽ Ŀǰ�˳����� 
             // һ�� ��ʵ�ʵ����浱�ж����ṩ�����ʽת�����ߣ�
             // ͼƬ����Ҫ��ǰת���ã����Բ�����ֲ�֧�ֵ�����
                throw CGRSCOMException(S_FALSE);
            }

            // ����һ��λͼ��ʽ��ͼƬ���ݶ���ӿ�
            ComPtr<IWICBitmapSource>pIBMP;

            if (!InlineIsEqualGUID(wpf, tgFormat))
            {// ����жϺ���Ҫ�����ԭWIC��ʽ����ֱ����ת��ΪDXGI��ʽ��ͼƬʱ
             // ������Ҫ���ľ���ת��ͼƬ��ʽΪ�ܹ�ֱ�Ӷ�ӦDXGI��ʽ����ʽ
                //����ͼƬ��ʽת����
                ComPtr<IWICFormatConverter> pIConverter;
                GRS_THROW_IF_FAILED(pIWICFactory->CreateFormatConverter(&pIConverter));

                //��ʼ��һ��ͼƬת������ʵ��Ҳ���ǽ�ͼƬ���ݽ����˸�ʽת��
                GRS_THROW_IF_FAILED(pIConverter->Initialize(
                    pIWICFrame.Get(),                // ����ԭͼƬ����
                    tgFormat,						 // ָ����ת����Ŀ���ʽ
                    WICBitmapDitherTypeNone,         // ָ��λͼ�Ƿ��е�ɫ�壬�ִ��������λͼ�����õ�ɫ�壬����ΪNone
                    NULL,                            // ָ����ɫ��ָ��
                    0.f,                             // ָ��Alpha��ֵ
                    WICBitmapPaletteTypeCustom       // ��ɫ�����ͣ�ʵ��û��ʹ�ã�����ָ��ΪCustom
                ));
                // ����QueryInterface������ö����λͼ����Դ�ӿ�
                GRS_THROW_IF_FAILED(pIConverter.As(&pIBMP));
            }
            else
            {
                //ͼƬ���ݸ�ʽ����Ҫת����ֱ�ӻ�ȡ��λͼ����Դ�ӿ�
                GRS_THROW_IF_FAILED(pIWICFrame.As(&pIBMP));
            }
            //���ͼƬ��С����λ�����أ�
            GRS_THROW_IF_FAILED(pIBMP->GetSize(&nTextureW, &nTextureH));

            //��ȡͼƬ���ص�λ��С��BPP��Bits Per Pixel����Ϣ�����Լ���ͼƬ�����ݵ���ʵ��С����λ���ֽڣ�
            ComPtr<IWICComponentInfo> pIWICmntinfo;
            GRS_THROW_IF_FAILED(pIWICFactory->CreateComponentInfo(tgFormat, pIWICmntinfo.GetAddressOf()));

            WICComponentType type;
            GRS_THROW_IF_FAILED(pIWICmntinfo->GetComponentType(&type));

            if (type != WICPixelFormat)
            {
                throw CGRSCOMException(S_FALSE);
            }

            ComPtr<IWICPixelFormatInfo> pIWICPixelinfo;
            GRS_THROW_IF_FAILED(pIWICmntinfo.As(&pIWICPixelinfo));

            // ���������ڿ��Եõ�BPP�ˣ���Ҳ���ҿ��ıȽ���Ѫ�ĵط���Ϊ��BPP��Ȼ������ô�໷��
            GRS_THROW_IF_FAILED(pIWICPixelinfo->GetBitsPerPixel(&nBPP));

            // ����ͼƬʵ�ʵ��д�С����λ���ֽڣ�������ʹ����һ����ȡ����������A+B-1��/B ��
            // ����������˵��΢���������,ϣ�����Ѿ���������ָ��
            UINT nPicRowPitch = (uint64_t(nTextureW) * uint64_t(nBPP) + 7u) / 8u;

            // 0. �����˶�ʸ������
            D3D12_DESCRIPTOR_HEAP_DESC mvHeapDesc;
            mvHeapDesc.NumDescriptors = 1;
            mvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            mvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            mvHeapDesc.NodeMask = 0;
            hsr = pID3D12Device4->CreateDescriptorHeap(&mvHeapDesc, IID_PPV_ARGS(pMvHeap.GetAddressOf()));

            D3D12_RESOURCE_DESC motionVectorTextureDesc = {};
            motionVectorTextureDesc.MipLevels = 1;
            motionVectorTextureDesc.Format = DXGI_FORMAT_R16G16_FLOAT; // �˶�ʸ���ĸ�ʽ
            motionVectorTextureDesc.Width = nTextureW; // ������
            motionVectorTextureDesc.Height = nTextureH; // ����߶�
            motionVectorTextureDesc.DepthOrArraySize = 1;
            motionVectorTextureDesc.SampleDesc.Count = 1;
            motionVectorTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            motionVectorTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // �����������
            motionVectorTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            hsr = pID3D12Device4->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &motionVectorTextureDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // ��ʼ״̬
                nullptr,
                IID_PPV_ARGS(motionVectorTexture.GetAddressOf())
            );

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Format = motionVectorTextureDesc.Format;
            uavDesc.Texture2D.MipSlice = 0;

            pID3D12Device4->CreateUnorderedAccessView(
                motionVectorTexture.Get(),
                nullptr,
                &uavDesc,
                pMvHeap.Get()->GetCPUDescriptorHandleForHeapStart()
            );

            // 2.����Դ������Դ
            // ����ͼƬ��Ϣ�����2D������Դ����Ϣ�ṹ��
            stTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            stTextureDesc.MipLevels = 1;
            stTextureDesc.Format = stTextureFormat;
            stTextureDesc.Width = nTextureW;
            stTextureDesc.Height = nTextureH;
            stTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            stTextureDesc.DepthOrArraySize = 1;
            stTextureDesc.SampleDesc.Count = 1;
            stTextureDesc.SampleDesc.Quality = 0;

            // ��������Ŀ������
            stTargetTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            stTargetTextureDesc.MipLevels = 1;
            stTargetTextureDesc.Format = stTextureFormat; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            stTargetTextureDesc.Width = tWidth;
            stTargetTextureDesc.Height = tHeight;
            stTargetTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            stTargetTextureDesc.DepthOrArraySize = 1;
            stTargetTextureDesc.SampleDesc.Count = 1;
            stTargetTextureDesc.SampleDesc.Quality = 0;

            D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_DEFAULT };

            //����Ĭ�϶��ϵ���Դ��������Texture2D��GPU��Ĭ�϶���Դ�ķ����ٶ�������
            //��Ϊ������Դһ���ǲ��ױ����Դ����������ͨ��ʹ���ϴ��Ѹ��Ƶ�Ĭ�϶���
            //�ڴ�ͳ��D3D11����ǰ��D3D�ӿ��У���Щ���̶�����װ�ˣ�����ֻ��ָ������ʱ������ΪĬ�϶� 
            hsr = pID3D12Device4->CreateCommittedResource(
                &stHeapProp
                , D3D12_HEAP_FLAG_NONE
                , &stTextureDesc				//����ʹ��CD3DX12_RESOURCE_DESC::Tex2D���򻯽ṹ��ĳ�ʼ��
                , D3D12_RESOURCE_STATE_COPY_DEST
                , nullptr
                , IID_PPV_ARGS(&pITexture));

            hsr = pID3D12Device4->CreateCommittedResource(
                &stHeapProp
                , D3D12_HEAP_FLAG_NONE
                , &stTargetTextureDesc				//����ʹ��CD3DX12_RESOURCE_DESC::Tex2D���򻯽ṹ��ĳ�ʼ��
                , D3D12_RESOURCE_STATE_COMMON
                , nullptr
                , IID_PPV_ARGS(&pITargetTexture));

            pICMDList->ResourceBarrier(
                1,
                &CD3DX12_RESOURCE_BARRIER::Transition(
                    pITargetTexture.Get(),
                    D3D12_RESOURCE_STATE_COMMON,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                )
            );


            //��ȡ�ϴ�����Դ����Ĵ�С������ߴ�ͨ������ʵ��ͼƬ�ĳߴ�
            UINT64 n64UploadBufferSize = 0;
            D3D12_RESOURCE_DESC stDestDesc = pITexture->GetDesc();
            pID3D12Device4->GetCopyableFootprints(&stDestDesc, 0, 1, 0, nullptr, nullptr, nullptr, &n64UploadBufferSize);

            // ���������ϴ��������Դ,ע����������Buffer
            // �ϴ��Ѷ���GPU������˵�����Ǻܲ�ģ�
            // ���Զ��ڼ������������������������
            // ͨ�������ϴ���GPU���ʸ���Ч��Ĭ�϶���
            stHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC stBufDesc = {};
            stBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            stBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            stBufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            stBufDesc.Format = DXGI_FORMAT_UNKNOWN;
            stBufDesc.Width = n64UploadBufferSize;
            stBufDesc.Height = 1;
            stBufDesc.DepthOrArraySize = 1;
            stBufDesc.MipLevels = 1;
            stBufDesc.SampleDesc.Count = 1;
            stBufDesc.SampleDesc.Quality = 0;

            GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
                &stHeapProp,
                D3D12_HEAP_FLAG_NONE,
                &stBufDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&pITextureUpload)));

            //������Դ�����С������ʵ��ͼƬ���ݴ洢���ڴ��С
            void* pbPicData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, n64UploadBufferSize);
            if (nullptr == pbPicData)
            {
                throw CGRSCOMException(HRESULT_FROM_WIN32(GetLastError()));
            }

            //��ͼƬ�ж�ȡ������
            GRS_THROW_IF_FAILED(pIBMP->CopyPixels(nullptr
                , nPicRowPitch
                , static_cast<UINT>(nPicRowPitch * nTextureH)   //ע���������ͼƬ������ʵ�Ĵ�С�����ֵͨ��С�ڻ���Ĵ�С
                , reinterpret_cast<BYTE*>(pbPicData)));

            //��ȡ���ϴ��ѿ����������ݵ�һЩ����ת���ߴ���Ϣ
            //���ڸ��ӵ�DDS�������Ƿǳ���Ҫ�Ĺ���
            UINT64 n64RequiredSize = 0u;
            UINT   nNumSubresources = 1u;  //����ֻ��һ��ͼƬ��������Դ����Ϊ1
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT stTxtLayouts = {};
            UINT64 n64TextureRowSizes = 0u;
            UINT   nTextureRowNum = 0u;

            stDestDesc = pITexture->GetDesc();

            pID3D12Device4->GetCopyableFootprints(&stDestDesc
                , 0
                , nNumSubresources
                , 0
                , &stTxtLayouts
                , &nTextureRowNum
                , &n64TextureRowSizes
                , &n64RequiredSize);

            //��Ϊ�ϴ���ʵ�ʾ���CPU�������ݵ�GPU���н�
            //�������ǿ���ʹ����Ϥ��Map����������ӳ�䵽CPU�ڴ��ַ��
            //Ȼ�����ǰ��н����ݸ��Ƶ��ϴ�����
            //��Ҫע�����֮���԰��п�������ΪGPU��Դ���д�С
            //��ʵ��ͼƬ���д�С���в����,���ߵ��ڴ�߽����Ҫ���ǲ�һ����
            BYTE* pData = nullptr;
            GRS_THROW_IF_FAILED(pITextureUpload->Map(0, NULL, reinterpret_cast<void**>(&pData)));

            BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + stTxtLayouts.Offset;
            const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pbPicData);
            for (UINT y = 0; y < nTextureRowNum; ++y)
            {
                memcpy(pDestSlice + static_cast<SIZE_T>(stTxtLayouts.Footprint.RowPitch) * y
                    , pSrcSlice + static_cast<SIZE_T>(nPicRowPitch) * y
                    , nPicRowPitch);
            }
            //ȡ��ӳ�� �����ױ��������ÿ֡�ı任��������ݣ�������������Unmap�ˣ�
            //������פ�ڴ�,������������ܣ���Ϊÿ��Map��Unmap�Ǻܺ�ʱ�Ĳ���
            //��Ϊ�������붼��64λϵͳ��Ӧ���ˣ���ַ�ռ����㹻�ģ�������ռ�ò���Ӱ��ʲô
            pITextureUpload->Unmap(0, NULL);

            //�ͷ�ͼƬ���ݣ���һ���ɾ��ĳ���Ա
            ::HeapFree(::GetProcessHeap(), 0, pbPicData);

            //��������з������ϴ��Ѹ����������ݵ�Ĭ�϶ѵ�����
            D3D12_TEXTURE_COPY_LOCATION stDst = {};
            stDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            stDst.pResource = pITexture.Get();
            stDst.SubresourceIndex = 0;

            D3D12_TEXTURE_COPY_LOCATION stSrc = {};
            stSrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            stSrc.pResource = pITextureUpload.Get();
            stSrc.PlacedFootprint = stTxtLayouts;

            pICMDList->CopyTextureRegion(&stDst, 0, 0, 0, &stSrc, nullptr);

            //����һ����Դ���ϣ�ͬ����ȷ�ϸ��Ʋ������
            //ֱ��ʹ�ýṹ��Ȼ����õ���ʽ
            D3D12_RESOURCE_BARRIER stResBar = {};
            stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            stResBar.Transition.pResource = pITexture.Get();
            stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            // ����!!!!!!!!!!!!
            pICMDList->ResourceBarrier(1, &stResBar);
        }

        // ���մ���SRV������
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
            stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            stSRVDesc.Format = stTextureDesc.Format;
            stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            stSRVDesc.Texture2D.MipLevels = 1;
            pID3D12Device4->CreateShaderResourceView(pITargetTexture.Get(), &stSRVDesc, pISRVHeap->GetCPUDescriptorHandleForHeapStart());
        }

        // ִ�������б��ȴ�������Դ�ϴ���ɣ���һ���Ǳ����
        {
            GRS_THROW_IF_FAILED(pICMDList->Close());
            ID3D12CommandList* ppCommandLists[] = { pICMDList.Get() };
            pICMDQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
        }

        // ����һ��ͬ�����󡪡�Χ�������ڵȴ���Ⱦ��ɣ���Ϊ����Draw Call���첽����
        {
            GRS_THROW_IF_FAILED(pID3D12Device4->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pIFence)));
            n64FenceValue = 1;
            // ����һ��Eventͬ���������ڵȴ�Χ���¼�֪ͨ
            hEventFence = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (hEventFence == nullptr)
            {
                GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        // �ȴ�������Դ��ʽ���������
        {
            const UINT64 n64CurrentFenceValue = n64FenceValue;
            GRS_THROW_IF_FAILED(pICMDQueue->Signal(pIFence.Get(), n64CurrentFenceValue));
            n64FenceValue++;
            GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(n64CurrentFenceValue, hEventFence));
            //������Ϣѭ����Msg Wait������ֱ�ӵȵ�����¼���� ����ʼ��Ⱦ
        }

        // �����Դ���Ͻṹ
        D3D12_RESOURCE_BARRIER stBeginResBarrier = {};
        stBeginResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        stBeginResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        stBeginResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
        stBeginResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        stBeginResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        stBeginResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        D3D12_RESOURCE_BARRIER stEndResBarrier = {};
        stEndResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        stEndResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        stEndResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
        stEndResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        stEndResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
        stEndResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;


        D3D12_RESOURCE_BARRIER stSRBeginResBarrier = {};
        stSRBeginResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        stSRBeginResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        stSRBeginResBarrier.Transition.pResource = pITargetTexture.Get();
        stSRBeginResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        stSRBeginResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        stSRBeginResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        D3D12_RESOURCE_BARRIER stSREndResBarrier = {};
        stSREndResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        stSREndResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        stSREndResBarrier.Transition.pResource = pITargetTexture.Get();
        stSREndResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        stSREndResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        stSREndResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
        DWORD dwRet = 0;
        BOOL bExit = FALSE;

        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        // Create a DirectSR engine for the desired variant.
        DSR_SUPERRES_CREATE_ENGINE_PARAMETERS createParams = {};
        createParams.VariantId = g_myDsrDescMap[0].VariantId;
        createParams.SourceColorFormat = stTextureFormat;
        createParams.SourceDepthFormat = DXGI_FORMAT_R32_FLOAT; // DXGI_FORMAT_R32_FLOAT
        createParams.TargetFormat = stTextureFormat;
        createParams.Flags = DSR_SUPERRES_CREATE_ENGINE_FLAG_AUTO_EXPOSURE;
        createParams.TargetSize = DSR_SIZE{ tWidth, tHeight };
        createParams.MaxSourceSize = DSR_SIZE{ sWidth, sHeight };
        // Create the super resolution engine
        hsr = pDSRDevice->CreateSuperResEngine(&createParams, IID_PPV_ARGS(&pSREngine));
        // Create super resolution upscaler
        hsr = pSREngine->CreateUpscaler(pICMDQueue.Get(), IID_PPV_ARGS(&pSRUpscaler));

        pSRUpscaler->MakeResident();

        // ��ʼ��Ϣѭ�����������в�����Ⱦ
        while (!bExit)
        {
            dwRet = ::MsgWaitForMultipleObjects(1, &hEventFence, FALSE, INFINITE, QS_ALLINPUT);
            switch (dwRet - WAIT_OBJECT_0)
            {
            case 0:
            {
                //��ȡ�µĺ󻺳���ţ���ΪPresent�������ʱ�󻺳����ž͸�����
                nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

                //�����������Resetһ��
                GRS_THROW_IF_FAILED(pICMDAlloc->Reset());
                //Reset�����б�������ָ�������������PSO����
                GRS_THROW_IF_FAILED(pICMDList->Reset(pICMDAlloc.Get(), pIPipelineState.Get()));

                //��ʼ��¼����
                pICMDList->SetGraphicsRootSignature(pIRootSignature.Get());
                ID3D12DescriptorHeap* ppHeaps[] = { pISRVHeap.Get() };
                pICMDList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
                pICMDList->SetGraphicsRootDescriptorTable(0, pISRVHeap->GetGPUDescriptorHandleForHeapStart());
                pICMDList->RSSetViewports(1, &stViewPort);
                pICMDList->RSSetScissorRects(1, &stScissorRect);

                // ִ�в���
                DSR_SUPERRES_UPSCALER_EXECUTE_PARAMETERS executeParams;
                executeParams.pTargetTexture = pITargetTexture.Get();
                executeParams.TargetRegion = D3D12_RECT{ 0, 0, (long)tWidth, (long)tHeight };
                executeParams.pSourceColorTexture = pITexture.Get(); //pITexture.Get();
                executeParams.SourceColorRegion = D3D12_RECT{ 0, 0, (long)nTextureW, (long)nTextureH };
                executeParams.pSourceDepthTexture = mDepthStencilBuffer.Get();
                executeParams.SourceDepthRegion = D3D12_RECT{ 0, 0, (long)nTextureW, (long)nTextureH };
                executeParams.pMotionVectorsTexture = motionVectorTexture.Get();
                executeParams.MotionVectorsRegion = D3D12_RECT{ 0, 0, (long)nTextureW, (long)nTextureH };
                executeParams.MotionVectorScale = DSR_FLOAT2{ 1.0f, 1.0f };
                executeParams.CameraJitter = DSR_FLOAT2{ 0, 0 };
                //executeParams.ExposureScale = 1.0f;
                //executeParams.PreExposure = 1.0f;
                //executeParams.Sharpness = 1.0f;
                executeParams.CameraNear = 0.f;
                executeParams.CameraFar = INFINITY;
                //executeParams.CameraFovAngleVert = 1.0f;
                executeParams.pExposureScaleTexture = nullptr;
                executeParams.pIgnoreHistoryMaskTexture = nullptr;
                //executeParams.IgnoreHistoryMaskRegion = D3D12_RECT{ 0, 0, (long)nTextureW, (long)nTextureH };
                executeParams.pReactiveMaskTexture = nullptr;
                //executeParams.ReactiveMaskRegion = D3D12_RECT{ 0, 0, (long)nTextureW, (long)nTextureH };

                // ִ�б�־
                DSR_SUPERRES_UPSCALER_EXECUTE_FLAGS executeFlags = DSR_SUPERRES_UPSCALER_EXECUTE_FLAG_NONE;

                float deltaTimeInSeconds = 1.0f;
                hsr = pSRUpscaler->Execute(&executeParams, deltaTimeInSeconds, executeFlags);

                // ͨ����Դ�����ж��󻺳��Ѿ��л���Ͽ��Կ�ʼ��Ⱦ��
                stBeginResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
                pICMDList->ResourceBarrier(1, &stBeginResBarrier);

                stRTVHandle = pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
                stRTVHandle.ptr += nFrameIndex * nRTVDescriptorSize;
                // ������ȾĿ��
                //pICMDList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);
                pICMDList->OMSetRenderTargets(1, &stRTVHandle, FALSE, &pDsvHeap->GetCPUDescriptorHandleForHeapStart());

                // ������¼�����������ʼ��һ֡����Ⱦ
                pICMDList->ClearRenderTargetView(stRTVHandle, faClearColor, 0, nullptr);
                pICMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                pICMDList->IASetVertexBuffers(0, 1, &stVertexBufferView);

                //Draw Call������
                pICMDList->DrawInstanced(nVertexCnt, 1, 0, 0);

                //��һ����Դ���ϣ�����ȷ����Ⱦ�Ѿ����������ύ����ȥ��ʾ��
                stEndResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
                pICMDList->ResourceBarrier(1, &stEndResBarrier);

                //�ر������б�����ȥִ����
                GRS_THROW_IF_FAILED(pICMDList->Close());

                //ִ�������б�
                ID3D12CommandList* ppCommandLists[] = { pICMDList.Get() };
                pICMDQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

                //�ύ����
                GRS_THROW_IF_FAILED(pISwapChain3->Present(1, 0));

                //��ʼͬ��GPU��CPU��ִ�У��ȼ�¼Χ�����ֵ
                const UINT64 n64CurrentFenceValue = n64FenceValue;
                GRS_THROW_IF_FAILED(pICMDQueue->Signal(pIFence.Get(), n64CurrentFenceValue));
                n64FenceValue++;
                GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(n64CurrentFenceValue, hEventFence));

            }
            break;
            case 1:
            {//������Ϣ
                while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (WM_QUIT != msg.message)
                    {
                        ::TranslateMessage(&msg);
                        ::DispatchMessage(&msg);
                    }
                    else
                    {
                        bExit = TRUE;
                    }
                }
            }
            break;
            case WAIT_TIMEOUT:
            {

            }
            break;
            default:
                break;
            }
        }
        //::CoUninitialize();
    }
    catch (CGRSCOMException& e)
    {//������COM�쳣
        e;
    }


    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

