
#include <windows.h>

#include <audioclient.h>
#include <cstdio>
#include <initguid.h>
#include <mmdeviceapi.h>

HINSTANCE mHinst = 0, mHinstDLL = 0;

// mProcs 数组将由 DllMain 从 real_mmdevapi.dll 中填充
// 并在 _hook_setup 中被修改，指向我们的 fake 函数
extern "C" UINT_PTR mProcs[37] = {0};

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

#ifndef _DEBUG
inline void log_info(const char *info) {}
#else
FILE *debug;
inline void log_info(const char *info) {
  fprintf(debug, "%s\n", info);
  fflush(debug);
}
#endif

// --- COM 接口的 GUIDs ---
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D,
            0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46,
            0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2,
            0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IMMDevice, 0xD666063F, 0x1587, 0x4E43, 0x81, 0xF1, 0xB9, 0x48,
            0xE8, 0x07, 0x36, 0x3F);

// --- 核心修改逻辑 ---
void ModifyWaveFormatForSpeedup(WAVEFORMATEX *fmt) {
  if (!fmt)
    return;
  fmt->nSamplesPerSec = (DWORD)((double)fmt->nSamplesPerSec * SPEEDUP);
  fmt->nAvgBytesPerSec = (DWORD)((double)fmt->nAvgBytesPerSec * SPEEDUP);
}

void RevertWaveFormatFromSpeedup(WAVEFORMATEX *fmt) {
  if (!fmt)
    return;
  fmt->nSamplesPerSec = (DWORD)((double)fmt->nSamplesPerSec / SPEEDUP);
  fmt->nAvgBytesPerSec = (DWORD)((double)fmt->nAvgBytesPerSec / SPEEDUP);
}

// --- 伪造的 IAudioClient ---
class FakeAudioClient : public IAudioClient {
private:
  IAudioClient *m_pRealAudioClient;
  LONG m_cRef;

public:
  FakeAudioClient(IAudioClient *pReal) : m_pRealAudioClient(pReal), m_cRef(1) {
    m_pRealAudioClient->AddRef();
  }
  virtual ~FakeAudioClient() {
    if (m_pRealAudioClient)
      m_pRealAudioClient->Release();
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
    WAVEFORMATEX *modified_fmt = NULL;
    if (pFormat) {
      size_t fmt_size = sizeof(WAVEFORMATEX) + pFormat->cbSize;
      modified_fmt = (WAVEFORMATEX *)malloc(fmt_size);
      if (!modified_fmt)
        return E_OUTOFMEMORY;
      memcpy(modified_fmt, pFormat, fmt_size);
      ModifyWaveFormatForSpeedup(modified_fmt);
    }
    REFERENCE_TIME modified_duration =
        (REFERENCE_TIME)((double)hnsBufferDuration / SPEEDUP);
    REFERENCE_TIME modified_period =
        (REFERENCE_TIME)((double)hnsPeriodicity / SPEEDUP);
    HRESULT hr = m_pRealAudioClient->Initialize(
        ShareMode, StreamFlags, modified_duration, modified_period,
        modified_fmt, AudioSessionGuid);
    if (modified_fmt)
      free(modified_fmt);
    return hr;
  }
  STDMETHODIMP IsFormatSupported(AUDCLNT_SHAREMODE ShareMode,
                                 const WAVEFORMATEX *pFormat,
                                 WAVEFORMATEX **ppClosestMatch) {
    WAVEFORMATEX *modified_fmt = NULL;
    if (pFormat) {
      size_t fmt_size = sizeof(WAVEFORMATEX) + pFormat->cbSize;
      modified_fmt = (WAVEFORMATEX *)malloc(fmt_size);
      if (!modified_fmt)
        return E_OUTOFMEMORY;
      memcpy(modified_fmt, pFormat, fmt_size);
      ModifyWaveFormatForSpeedup(modified_fmt);
    }
    HRESULT hr = m_pRealAudioClient->IsFormatSupported(ShareMode, modified_fmt,
                                                       ppClosestMatch);
    if (modified_fmt)
      free(modified_fmt);
    if (hr == S_FALSE && ppClosestMatch && *ppClosestMatch) {
      RevertWaveFormatFromSpeedup(*ppClosestMatch);
    }
    return hr;
  }
  STDMETHODIMP GetMixFormat(WAVEFORMATEX **ppDeviceFormat) {
    HRESULT hr = m_pRealAudioClient->GetMixFormat(ppDeviceFormat);
    if (SUCCEEDED(hr) && ppDeviceFormat && *ppDeviceFormat) {
      RevertWaveFormatFromSpeedup(*ppDeviceFormat);
    }
    return hr;
  }
  // --- 其他 IAudioClient 方法 (直接转发) ---
  STDMETHODIMP GetBufferSize(UINT32 *pNumBufferFrames) {
    return m_pRealAudioClient->GetBufferSize(pNumBufferFrames);
  }
  STDMETHODIMP GetStreamLatency(REFERENCE_TIME *phnsLatency) {
    return m_pRealAudioClient->GetStreamLatency(phnsLatency);
  }
  STDMETHODIMP GetCurrentPadding(UINT32 *pNumPaddingFrames) {
    return m_pRealAudioClient->GetCurrentPadding(pNumPaddingFrames);
  }
  STDMETHODIMP GetService(REFIID riid, void **ppv) {
    return m_pRealAudioClient->GetService(riid, ppv);
  }
  STDMETHODIMP Start(void) { return m_pRealAudioClient->Start(); }
  STDMETHODIMP Stop(void) { return m_pRealAudioClient->Stop(); }
  STDMETHODIMP Reset(void) { return m_pRealAudioClient->Reset(); }
  STDMETHODIMP SetEventHandle(HANDLE eventHandle) {
    return m_pRealAudioClient->SetEventHandle(eventHandle);
  }
  STDMETHODIMP GetDevicePeriod(REFERENCE_TIME *phnsDefaultDevicePeriod,
                               REFERENCE_TIME *phnsMinimumDevicePeriod) {
    return m_pRealAudioClient->GetDevicePeriod(phnsDefaultDevicePeriod,
                                               phnsMinimumDevicePeriod);
  }
};

// --- 伪造的 IMMDevice ---
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
      pRealClient
          ->Release(); // FakeAudioClient 已经 AddRef, 所以这里可以 Release
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

// --- 伪造的 IMMDeviceEnumerator ---
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

// --- 伪造的 IClassFactory ---
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

// --- Hook 核心: 伪造 DllGetClassObject ---

// 激活对 DllGetClassObject 的 Hook
#define DLLGETCLASSOBJECT

// 定义 DllGetClassObject 函数指针类型
typedef HRESULT(WINAPI *DllGetClassObject_ptr)(REFCLSID, REFIID, LPVOID *);
// 定义一个变量来保存原始 DllGetClassObject 函数的地址
DllGetClassObject_ptr DllGetClassObject_real = NULL;

// 我们伪造的 DllGetClassObject 函数
HRESULT WINAPI DllGetClassObject_fake(REFCLSID rclsid, REFIID riid,
                                      LPVOID *ppv) {
  // 首先，调用原始的 DllGetClassObject 函数
  HRESULT hr = DllGetClassObject_real(rclsid, riid, ppv);

  // 如果调用成功，并且请求的是 MMDeviceEnumerator 的类厂
  if (SUCCEEDED(hr) && rclsid == CLSID_MMDeviceEnumerator) {
    // 替换返回的类厂为我们伪造的类厂
    IClassFactory *pRealFactory = (IClassFactory *)*ppv;
    *ppv = new FakeClassFactory(pRealFactory);
    pRealFactory
        ->Release(); // FakeClassFactory 已经 AddRef, 所以这里可以 Release
  }
  return hr;
}

// =================================================================
// ========= 核心逻辑迁移结束 ENDS HERE ==========================
// =================================================================

// _hook_setup 函数将根据上面定义的宏 (例如 DLLGETCLASSOBJECT)
// 自动将 mProcs 数组中的指针替换为我们的 fake 函数
inline void _hook_setup() {
#ifdef ACTIVATEAUDIOINTERFACEASYNC
  ActivateAudioInterfaceAsync_real = (ActivateAudioInterfaceAsync_ptr)mProcs[0];
  mProcs[0] = (UINT_PTR)&ActivateAudioInterfaceAsync_fake;
#endif
#ifdef DLLCANUNLOADNOW
  DllCanUnloadNow_real = (DllCanUnloadNow_ptr)mProcs[1];
  mProcs[1] = (UINT_PTR)&DllCanUnloadNow_fake;
#endif
#ifdef DLLGETCLASSOBJECT
  DllGetClassObject_real = (DllGetClassObject_ptr)mProcs[2];
  mProcs[2] = (UINT_PTR)&DllGetClassObject_fake;
#endif
#ifdef DLLREGISTERSERVER
  DllRegisterServer_real = (DllRegisterServer_ptr)mProcs[3];
  mProcs[3] = (UINT_PTR)&DllRegisterServer_fake;
#endif
#ifdef DLLUNREGISTERSERVER
  DllUnregisterServer_real = (DllUnregisterServer_ptr)mProcs[4];
  mProcs[4] = (UINT_PTR)&DllUnregisterServer_fake;
#endif
  // ... 其他函数的宏判断 ...
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  mHinst = hinstDLL;
  if (fdwReason == DLL_PROCESS_ATTACH) {
    mHinstDLL = LoadLibraryA("real_mmdevapi.dll");

    if (!mHinstDLL) {
      return FALSE;
    }
    for (int i = 0; i < 37; ++i) {
      mProcs[i] = (UINT_PTR)GetProcAddress(mHinstDLL, mImportNames[i]);
    }
    _hook_setup();
#ifdef _DEBUG
    debug = fopen("./debug.log", "a");
#endif
  } else if (fdwReason == DLL_PROCESS_DETACH) {
#ifdef _DEBUG
    fclose(debug);
#endif
    FreeLibrary(mHinstDLL);
  }
  return TRUE;
}