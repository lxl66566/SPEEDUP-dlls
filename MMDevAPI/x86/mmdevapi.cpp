
#include <windows.h>

#include <audioclient.h>
#include <cstdio>
#include <initguid.h>
#include <mmdeviceapi.h>

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

  // === [修正 1] 添加缺失的 GetDevicePeriod 方法 ===
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
  // === [修正 2] 删除了这里错误的 Activate 方法 ===
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

// --- 劫持的 DllGetClassObject ---
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

// --- DLL 入口和导出函数转发 ---
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