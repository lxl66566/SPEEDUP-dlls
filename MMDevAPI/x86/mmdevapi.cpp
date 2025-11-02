#include <windows.h>

#include <audioclient.h>
#include <cstdio>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <vector>

#include <soundtouch/SoundTouch.h>

// --- 全局变量和定义 ---
HINSTANCE hRealDll = NULL;
FARPROC p[37];
typedef HRESULT(WINAPI *DllGetClassObject_t)(REFCLSID, REFIID, LPVOID *);
DllGetClassObject_t pDllGetClassObject = NULL;

// --- COM 接口的 GUIDs ---
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D,
            0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46,
            0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2,
            0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IMMDevice, 0xD666063F, 0x1587, 0x4E43, 0x81, 0xF1, 0xB9, 0x48,
            0xE8, 0x07, 0x36, 0x3F);
DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF,
            0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);

// --- 伪造的 IAudioRenderClient (数据处理核心) ---
class FakeAudioRenderClient : public IAudioRenderClient {
private:
  IAudioRenderClient *m_pRealRenderClient;
  LONG m_cRef;
  soundtouch::SoundTouch *m_pSoundTouch;
  WAVEFORMATEX *m_pFormat;
  BYTE *m_pInputBuffer; // 我们提供给应用程序的缓冲区
  UINT32 m_inputBufferSizeInFrames;

public:
  FakeAudioRenderClient(IAudioRenderClient *pReal, const WAVEFORMATEX *pFormat,
                        UINT32 bufferSizeInFrames)
      : m_pRealRenderClient(pReal), m_cRef(1), m_pSoundTouch(nullptr),
        m_pFormat(nullptr), m_pInputBuffer(nullptr) {
    m_pRealRenderClient->AddRef();

    // 复制音频格式
    size_t fmt_size = sizeof(WAVEFORMATEX) + pFormat->cbSize;
    m_pFormat = (WAVEFORMATEX *)malloc(fmt_size);
    if (m_pFormat) {
      memcpy(m_pFormat, pFormat, fmt_size);
    }

    // 初始化 SoundTouch
    m_pSoundTouch = new soundtouch::SoundTouch();
    m_pSoundTouch->setSampleRate(m_pFormat->nSamplesPerSec);
    m_pSoundTouch->setChannels(m_pFormat->nChannels);
    m_pSoundTouch->setTempo(SPEEDUP);

    // 创建我们自己的输入缓冲区
    m_inputBufferSizeInFrames = bufferSizeInFrames;
    m_pInputBuffer =
        new BYTE[m_inputBufferSizeInFrames * m_pFormat->nBlockAlign];
  }

  virtual ~FakeAudioRenderClient() {
    if (m_pRealRenderClient)
      m_pRealRenderClient->Release();
    if (m_pSoundTouch)
      delete m_pSoundTouch;
    if (m_pFormat)
      free(m_pFormat);
    if (m_pInputBuffer)
      delete[] m_pInputBuffer;
  }

  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_IAudioRenderClient) {
      *ppv = static_cast<IAudioRenderClient *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = NULL;
    return m_pRealRenderClient->QueryInterface(riid, ppv);
  }

  STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }

  STDMETHODIMP_(ULONG) Release() {
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef)
      delete this;
    return ulRef;
  }

  STDMETHODIMP GetBuffer(UINT32 NumFramesRequested, BYTE **ppData) {
    if (!ppData)
      return E_POINTER;
    if (NumFramesRequested > m_inputBufferSizeInFrames) {
      *ppData = NULL;
      return AUDCLNT_E_BUFFER_TOO_LARGE;
    }
    *ppData = m_pInputBuffer;
    return S_OK;
  }

  STDMETHODIMP ReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags) {
    if (NumFramesWritten == 0) {
      return m_pRealRenderClient->ReleaseBuffer(0, dwFlags);
    }

    // SoundTouch 最适合处理 32-bit 浮点数音频
    // 如果是其他格式 (如16-bit PCM), 需要在此处添加转换代码
    if (m_pFormat->wFormatTag != WAVE_FORMAT_IEEE_FLOAT ||
        m_pFormat->wBitsPerSample != 32) {
      // 暂不支持其他格式，直接静音传递以避免崩溃
      return m_pRealRenderClient->ReleaseBuffer(NumFramesWritten,
                                                AUDCLNT_BUFFERFLAGS_SILENT);
    }

    // 1. 将应用程序写入的数据送入 SoundTouch
    m_pSoundTouch->putSamples((const soundtouch::SAMPLETYPE *)m_pInputBuffer,
                              NumFramesWritten);

    // 2. 从 SoundTouch 中取出处理后的数据，并写入真实设备
    UINT32 framesAvailable;
    std::vector<soundtouch::SAMPLETYPE> outputBuffer(8192);

    do {
      framesAvailable = m_pSoundTouch->receiveSamples(
          outputBuffer.data(), outputBuffer.size() / m_pFormat->nChannels);

      if (framesAvailable > 0) {
        BYTE *pRealBuffer = NULL;
        HRESULT hr =
            m_pRealRenderClient->GetBuffer(framesAvailable, &pRealBuffer);
        if (SUCCEEDED(hr)) {
          memcpy(pRealBuffer, outputBuffer.data(),
                 framesAvailable * m_pFormat->nBlockAlign);
          m_pRealRenderClient->ReleaseBuffer(framesAvailable, 0);
        }
      }
    } while (framesAvailable > 0);

    return S_OK;
  }
};

// --- 伪造的 IAudioClient (控制流核心) ---
class FakeAudioClient : public IAudioClient {
private:
  IAudioClient *m_pRealAudioClient;
  LONG m_cRef;
  WAVEFORMATEX *m_pClientFormat; // 保存客户端初始化时使用的格式

public:
  FakeAudioClient(IAudioClient *pReal)
      : m_pRealAudioClient(pReal), m_cRef(1), m_pClientFormat(nullptr) {
    m_pRealAudioClient->AddRef();
  }
  virtual ~FakeAudioClient() {
    if (m_pRealAudioClient)
      m_pRealAudioClient->Release();
    if (m_pClientFormat)
      free(m_pClientFormat);
  }
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_IAudioClient) {
      *ppv = static_cast<IAudioClient *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = NULL;
    return m_pRealAudioClient->QueryInterface(riid, ppv);
  }
  STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
  STDMETHODIMP_(ULONG) Release() {
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef)
      delete this;
    return ulRef;
  }
  STDMETHODIMP Initialize(AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags,
                          REFERENCE_TIME hnsBufferDuration,
                          REFERENCE_TIME hnsPeriodicity,
                          const WAVEFORMATEX *pFormat,
                          LPCGUID AudioSessionGuid) {
    // 不再修改音频格式，直接初始化
    HRESULT hr = m_pRealAudioClient->Initialize(
        ShareMode, StreamFlags, hnsBufferDuration, hnsPeriodicity, pFormat,
        AudioSessionGuid);

    // 如果成功，保存格式以备 GetService 使用
    if (SUCCEEDED(hr) && pFormat) {
      if (m_pClientFormat)
        free(m_pClientFormat);
      size_t fmt_size = sizeof(WAVEFORMATEX) + pFormat->cbSize;
      m_pClientFormat = (WAVEFORMATEX *)malloc(fmt_size);
      if (m_pClientFormat) {
        memcpy(m_pClientFormat, pFormat, fmt_size);
      } else {
        return E_OUTOFMEMORY;
      }
    }
    return hr;
  }
  STDMETHODIMP IsFormatSupported(AUDCLNT_SHAREMODE ShareMode,
                                 const WAVEFORMATEX *pFormat,
                                 WAVEFORMATEX **ppClosestMatch) {
    // 直接转发
    return m_pRealAudioClient->IsFormatSupported(ShareMode, pFormat,
                                                 ppClosestMatch);
  }
  STDMETHODIMP GetMixFormat(WAVEFORMATEX **ppDeviceFormat) {
    // 直接转发
    return m_pRealAudioClient->GetMixFormat(ppDeviceFormat);
  }
  STDMETHODIMP GetService(REFIID riid, void **ppv) {
    // 拦截 IAudioRenderClient 请求
    if (riid == IID_IAudioRenderClient) {
      IAudioRenderClient *pRealRenderClient = NULL;
      HRESULT hr =
          m_pRealAudioClient->GetService(riid, (void **)&pRealRenderClient);
      if (SUCCEEDED(hr)) {
        if (!m_pClientFormat) {
          pRealRenderClient->Release();
          return E_UNEXPECTED; // 格式未在 Initialize 中保存
        }
        UINT32 bufferSize = 0;
        m_pRealAudioClient->GetBufferSize(&bufferSize);
        *ppv = new FakeAudioRenderClient(pRealRenderClient, m_pClientFormat,
                                         bufferSize);
        pRealRenderClient->Release(); // Fake client 已 AddRef
      }
      return hr;
    }
    // 对于其他服务，直接转发
    return m_pRealAudioClient->GetService(riid, ppv);
  }

  STDMETHODIMP GetCurrentPadding(UINT32 *pNumPaddingFrames) {
    if (!pNumPaddingFrames)
      return E_POINTER;
    UINT32 realPadding = 0;
    HRESULT hr = m_pRealAudioClient->GetCurrentPadding(&realPadding);
    if (SUCCEEDED(hr)) {
      // 返回一个按比例缩小的Padding值，欺骗应用程序，让它更快地提供数据
      *pNumPaddingFrames = (UINT32)((double)realPadding / SPEEDUP);
    }
    return hr;
  }

  STDMETHODIMP GetStreamLatency(REFERENCE_TIME *phnsLatency) {
    REFERENCE_TIME realLatency = 0;
    HRESULT hr = m_pRealAudioClient->GetStreamLatency(&realLatency);
    if (SUCCEEDED(hr) && phnsLatency) {
      // 报告一个更短的延迟
      *phnsLatency = (REFERENCE_TIME)((double)realLatency / SPEEDUP);
    }
    return hr;
  }

  STDMETHODIMP GetDevicePeriod(REFERENCE_TIME *phnsDefaultDevicePeriod,
                               REFERENCE_TIME *phnsMinimumDevicePeriod) {
    HRESULT hr = m_pRealAudioClient->GetDevicePeriod(phnsDefaultDevicePeriod,
                                                     phnsMinimumDevicePeriod);
    if (SUCCEEDED(hr)) {
      // 报告一个更短的设备周期
      if (phnsDefaultDevicePeriod)
        *phnsDefaultDevicePeriod =
            (REFERENCE_TIME)((double)*phnsDefaultDevicePeriod / SPEEDUP);
      if (phnsMinimumDevicePeriod)
        *phnsMinimumDevicePeriod =
            (REFERENCE_TIME)((double)*phnsMinimumDevicePeriod / SPEEDUP);
    }
    return hr;
  }

  // --- 其他直接转发的方法 ---
  STDMETHODIMP GetBufferSize(UINT32 *pNumBufferFrames) {
    return m_pRealAudioClient->GetBufferSize(pNumBufferFrames);
  }
  STDMETHODIMP Start(void) { return m_pRealAudioClient->Start(); }
  STDMETHODIMP Stop(void) { return m_pRealAudioClient->Stop(); }
  STDMETHODIMP Reset(void) { return m_pRealAudioClient->Reset(); }
  STDMETHODIMP SetEventHandle(HANDLE eventHandle) {
    return m_pRealAudioClient->SetEventHandle(eventHandle);
  }
};

// --- 伪造的 IMMDevice (与之前相同) ---
class FakeDevice : public IMMDevice {
private:
  IMMDevice *m_pRealDevice;
  LONG m_cRef;

public:
  FakeDevice(IMMDevice *pReal) : m_pRealDevice(pReal), m_cRef(1) {
    m_pRealDevice->AddRef();
  }
  virtual ~FakeDevice() {
    if (m_pRealDevice)
      m_pRealDevice->Release();
  }
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_IMMDevice) {
      *ppv = static_cast<IMMDevice *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = NULL;
    return m_pRealDevice->QueryInterface(riid, ppv);
  }
  STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
  STDMETHODIMP_(ULONG) Release() {
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef)
      delete this;
    return ulRef;
  }
  STDMETHODIMP Activate(REFIID iid, DWORD dwClsCtx,
                        PROPVARIANT *pActivationParams, void **ppInterface) {
    HRESULT hr =
        m_pRealDevice->Activate(iid, dwClsCtx, pActivationParams, ppInterface);
    if (SUCCEEDED(hr) && iid == IID_IAudioClient && ppInterface &&
        *ppInterface) {
      IAudioClient *pRealClient = (IAudioClient *)*ppInterface;
      *ppInterface = new FakeAudioClient(pRealClient);
      pRealClient->Release();
    }
    return hr;
  }
  STDMETHODIMP OpenPropertyStore(DWORD stgmAccess,
                                 IPropertyStore **ppProperties) {
    return m_pRealDevice->OpenPropertyStore(stgmAccess, ppProperties);
  }
  STDMETHODIMP GetId(LPWSTR *ppstrId) { return m_pRealDevice->GetId(ppstrId); }
  STDMETHODIMP GetState(DWORD *pdwState) {
    return m_pRealDevice->GetState(pdwState);
  }
};

// --- 伪造的 IMMDeviceEnumerator (与之前相同) ---
class FakeDeviceEnumerator : public IMMDeviceEnumerator {
private:
  IMMDeviceEnumerator *m_pRealEnumerator;
  LONG m_cRef;

public:
  FakeDeviceEnumerator(IMMDeviceEnumerator *pReal)
      : m_pRealEnumerator(pReal), m_cRef(1) {
    m_pRealEnumerator->AddRef();
  }
  virtual ~FakeDeviceEnumerator() {
    if (m_pRealEnumerator)
      m_pRealEnumerator->Release();
  }
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_IMMDeviceEnumerator) {
      *ppv = static_cast<IMMDeviceEnumerator *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = NULL;
    return m_pRealEnumerator->QueryInterface(riid, ppv);
  }
  STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
  STDMETHODIMP_(ULONG) Release() {
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef)
      delete this;
    return ulRef;
  }
  STDMETHODIMP GetDefaultAudioEndpoint(EDataFlow dataFlow, ERole role,
                                       IMMDevice **ppEndpoint) {
    HRESULT hr =
        m_pRealEnumerator->GetDefaultAudioEndpoint(dataFlow, role, ppEndpoint);
    if (SUCCEEDED(hr) && ppEndpoint && *ppEndpoint) {
      IMMDevice *pRealDevice = *ppEndpoint;
      *ppEndpoint = new FakeDevice(pRealDevice);
      pRealDevice->Release();
    }
    return hr;
  }
  STDMETHODIMP GetDevice(LPCWSTR pwstrId, IMMDevice **ppDevice) {
    HRESULT hr = m_pRealEnumerator->GetDevice(pwstrId, ppDevice);
    if (SUCCEEDED(hr) && ppDevice && *ppDevice) {
      IMMDevice *pRealDevice = *ppDevice;
      *ppDevice = new FakeDevice(pRealDevice);
      pRealDevice->Release();
    }
    return hr;
  }
  STDMETHODIMP EnumAudioEndpoints(EDataFlow dataFlow, DWORD dwStateMask,
                                  IMMDeviceCollection **ppDevices) {
    return m_pRealEnumerator->EnumAudioEndpoints(dataFlow, dwStateMask,
                                                 ppDevices);
  }
  STDMETHODIMP
  RegisterEndpointNotificationCallback(IMMNotificationClient *pClient) {
    return m_pRealEnumerator->RegisterEndpointNotificationCallback(pClient);
  }
  STDMETHODIMP
  UnregisterEndpointNotificationCallback(IMMNotificationClient *pClient) {
    return m_pRealEnumerator->UnregisterEndpointNotificationCallback(pClient);
  }
};

// --- 伪造的 IClassFactory (与之前相同) ---
class FakeClassFactory : public IClassFactory {
private:
  IClassFactory *m_pRealFactory;
  LONG m_cRef;

public:
  FakeClassFactory(IClassFactory *pReal) : m_pRealFactory(pReal), m_cRef(1) {
    m_pRealFactory->AddRef();
  }
  virtual ~FakeClassFactory() {
    if (m_pRealFactory)
      m_pRealFactory->Release();
  }
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_IClassFactory) {
      *ppv = static_cast<IClassFactory *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = NULL;
    return m_pRealFactory->QueryInterface(riid, ppv);
  }
  STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
  STDMETHODIMP_(ULONG) Release() {
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef)
      delete this;
    return ulRef;
  }
  STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                              void **ppvObject) {
    HRESULT hr = m_pRealFactory->CreateInstance(pUnkOuter, riid, ppvObject);
    if (SUCCEEDED(hr) && riid == IID_IMMDeviceEnumerator && ppvObject &&
        *ppvObject) {
      IMMDeviceEnumerator *pRealEnumerator = (IMMDeviceEnumerator *)*ppvObject;
      *ppvObject = new FakeDeviceEnumerator(pRealEnumerator);
      pRealEnumerator->Release();
    }
    return hr;
  }
  STDMETHODIMP LockServer(BOOL fLock) {
    return m_pRealFactory->LockServer(fLock);
  }
};

// --- 劫持的 DllGetClassObject (与之前相同) ---
HRESULT WINAPI FakeDllGetClassObject(REFCLSID rclsid, REFIID riid,
                                     LPVOID *ppv) {
  HRESULT hr = pDllGetClassObject(rclsid, riid, ppv);
  if (SUCCEEDED(hr) && rclsid == CLSID_MMDeviceEnumerator) {
    IClassFactory *pRealFactory = (IClassFactory *)*ppv;
    *ppv = new FakeClassFactory(pRealFactory);
    pRealFactory->Release();
  }
  return hr;
}

// --- DLL 入口和导出函数转发 (与之前相同) ---
LPCSTR mImportNames[] = {
    "ActivateAudioInterfaceAsync",
    "DllCanUnloadNow",
    "DllGetClassObject",
    "DllRegisterServer",
    "DllUnregisterServer",
    (LPCSTR)2,
    (LPCSTR)3,
    (LPCSTR)4,
    (LPCSTR)5,
    (LPCSTR)6,
    (LPCSTR)7,
    (LPCSTR)8,
    (LPCSTR)9,
    (LPCSTR)10,
    (LPCSTR)11,
    (LPCSTR)12,
    (LPCSTR)13,
    (LPCSTR)14,
    (LPCSTR)15,
    (LPCSTR)16,
    (LPCSTR)18,
    (LPCSTR)19,
    (LPCSTR)20,
    (LPCSTR)21,
    (LPCSTR)22,
    (LPCSTR)23,
    (LPCSTR)24,
    (LPCSTR)25,
    (LPCSTR)26,
    (LPCSTR)27,
    (LPCSTR)28,
    (LPCSTR)29,
    (LPCSTR)30,
    (LPCSTR)31,
    (LPCSTR)32,
    (LPCSTR)33,
    (LPCSTR)34,
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    hRealDll = LoadLibraryA("real_mmdevapi.dll");
    if (!hRealDll) {
      return FALSE;
    }
    for (int i = 0; i < 37; ++i)
      p[i] = GetProcAddress(hRealDll, mImportNames[i]);
    pDllGetClassObject = (DllGetClassObject_t)p[2];
    p[2] = (FARPROC)FakeDllGetClassObject;
  } else if (fdwReason == DLL_PROCESS_DETACH) {
    if (hRealDll)
      FreeLibrary(hRealDll);
  }
  return TRUE;
}

extern "C" {
__declspec(naked) void ActivateAudioInterfaceAsync_wrapper() {
  __asm { jmp p[0] }
}
__declspec(naked) void DllCanUnloadNow_wrapper() {
  __asm { jmp p[1] }
}
__declspec(naked) void DllGetClassObject_wrapper() {
  __asm { jmp p[2] }
}
__declspec(naked) void DllRegisterServer_wrapper() {
  __asm { jmp p[3] }
}
__declspec(naked) void DllUnregisterServer_wrapper() {
  __asm { jmp p[4] }
}
__declspec(naked) void ExportByOrdinal2() {
  __asm { jmp p[5] }
}
__declspec(naked) void ExportByOrdinal3() {
  __asm { jmp p[6] }
}
__declspec(naked) void ExportByOrdinal4() {
  __asm { jmp p[7] }
}
__declspec(naked) void ExportByOrdinal5() {
  __asm { jmp p[8] }
}
__declspec(naked) void ExportByOrdinal6() {
  __asm { jmp p[9] }
}
__declspec(naked) void ExportByOrdinal7() {
  __asm { jmp p[10] }
}
__declspec(naked) void ExportByOrdinal8() {
  __asm { jmp p[11] }
}
__declspec(naked) void ExportByOrdinal9() {
  __asm { jmp p[12] }
}
__declspec(naked) void ExportByOrdinal10() {
  __asm { jmp p[13] }
}
__declspec(naked) void ExportByOrdinal11() {
  __asm { jmp p[14] }
}
__declspec(naked) void ExportByOrdinal12() {
  __asm { jmp p[15] }
}
__declspec(naked) void ExportByOrdinal13() {
  __asm { jmp p[16] }
}
__declspec(naked) void ExportByOrdinal14() {
  __asm { jmp p[17] }
}
__declspec(naked) void ExportByOrdinal15() {
  __asm { jmp p[18] }
}
__declspec(naked) void ExportByOrdinal16() {
  __asm { jmp p[19] }
}
__declspec(naked) void ExportByOrdinal18() {
  __asm { jmp p[20] }
}
__declspec(naked) void ExportByOrdinal19() {
  __asm { jmp p[21] }
}
__declspec(naked) void ExportByOrdinal20() {
  __asm { jmp p[22] }
}
__declspec(naked) void ExportByOrdinal21() {
  __asm { jmp p[23] }
}
__declspec(naked) void ExportByOrdinal22() {
  __asm { jmp p[24] }
}
__declspec(naked) void ExportByOrdinal23() {
  __asm { jmp p[25] }
}
__declspec(naked) void ExportByOrdinal24() {
  __asm { jmp p[26] }
}
__declspec(naked) void ExportByOrdinal25() {
  __asm { jmp p[27] }
}
__declspec(naked) void ExportByOrdinal26() {
  __asm { jmp p[28] }
}
__declspec(naked) void ExportByOrdinal27() {
  __asm { jmp p[29] }
}
__declspec(naked) void ExportByOrdinal28() {
  __asm { jmp p[30] }
}
__declspec(naked) void ExportByOrdinal29() {
  __asm { jmp p[31] }
}
__declspec(naked) void ExportByOrdinal30() {
  __asm { jmp p[32] }
}
__declspec(naked) void ExportByOrdinal31() {
  __asm { jmp p[33] }
}
__declspec(naked) void ExportByOrdinal32() {
  __asm { jmp p[34] }
}
__declspec(naked) void ExportByOrdinal33() {
  __asm { jmp p[35] }
}
__declspec(naked) void ExportByOrdinal34() {
  __asm { jmp p[36] }
}
}