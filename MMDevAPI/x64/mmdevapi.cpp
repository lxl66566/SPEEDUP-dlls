#include <windows.h>

#include <audioclient.h>
#include <cstdio>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <vector>

#include <soundtouch/SoundTouch.h>

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
  if (debug) {
    fprintf(debug, "%s\n", info);
    fflush(debug);
  }
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
DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF,
            0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);

// --- 数据处理层: 伪造的 IAudioRenderClient ---
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
        m_pFormat(nullptr), m_pInputBuffer(nullptr),
        m_inputBufferSizeInFrames(0) {
    m_pRealRenderClient->AddRef();

    size_t fmt_size = sizeof(WAVEFORMATEX) + pFormat->cbSize;
    m_pFormat = (WAVEFORMATEX *)malloc(fmt_size);
    if (m_pFormat) {
      memcpy(m_pFormat, pFormat, fmt_size);
    }

    m_pSoundTouch = new soundtouch::SoundTouch();
    m_pSoundTouch->setSampleRate(m_pFormat->nSamplesPerSec);
    m_pSoundTouch->setChannels(m_pFormat->nChannels);
    m_pSoundTouch->setTempo(SPEEDUP);

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

    // SoundTouch 内部使用 float 处理，这里假设输入也是 float。
    // 如果是其他格式 (如 16-bit PCM), 需要在此处进行转换。
    bool isFloat = (m_pFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                    (m_pFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                     ((WAVEFORMATEXTENSIBLE *)m_pFormat)->SubFormat ==
                         KSDATAFORMAT_SUBTYPE_IEEE_FLOAT));

    if (!isFloat || m_pFormat->wBitsPerSample != 32) {
      log_info("Unsupported format for processing. Passing silence.");
      BYTE *pRealBuffer = NULL;
      UINT32 padding = 0;
      // 尝试获取一个缓冲区来填充静音，以避免音频中断
      HRESULT hr =
          m_pRealRenderClient->GetBuffer(NumFramesWritten, &pRealBuffer);
      if (SUCCEEDED(hr)) {
        m_pRealRenderClient->ReleaseBuffer(NumFramesWritten,
                                           AUDCLNT_BUFFERFLAGS_SILENT);
      }
      return S_OK;
    }

    // 1. 将应用程序写入的数据送入 SoundTouch
    m_pSoundTouch->putSamples((const soundtouch::SAMPLETYPE *)m_pInputBuffer,
                              NumFramesWritten);

    // 2. 从 SoundTouch 中取出处理后的数据，并写入真实设备
    UINT32 framesAvailable;
    std::vector<soundtouch::SAMPLETYPE> outputBuffer(8192 *
                                                     m_pFormat->nChannels);

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
        } else {
          log_info("Failed to get real buffer. Dropping processed samples.");
        }
      }
    } while (m_pSoundTouch->numSamples() >=
             64); // 循环直到SoundTouch内部缓冲减少

    return S_OK;
  }
};

// --- 控制层: 伪造的 IAudioClient ---
class FakeAudioClient : public IAudioClient {
private:
  IAudioClient *m_pRealAudioClient;
  LONG m_cRef;
  WAVEFORMATEX *m_pClientFormat;

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
    HRESULT hr = m_pRealAudioClient->Initialize(
        ShareMode, StreamFlags, hnsBufferDuration, hnsPeriodicity, pFormat,
        AudioSessionGuid);

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
    return m_pRealAudioClient->IsFormatSupported(ShareMode, pFormat,
                                                 ppClosestMatch);
  }
  STDMETHODIMP GetMixFormat(WAVEFORMATEX **ppDeviceFormat) {
    return m_pRealAudioClient->GetMixFormat(ppDeviceFormat);
  }

  STDMETHODIMP GetService(REFIID riid, void **ppv) {
    if (riid == IID_IAudioRenderClient) {
      IAudioRenderClient *pRealRenderClient = NULL;
      HRESULT hr =
          m_pRealAudioClient->GetService(riid, (void **)&pRealRenderClient);
      if (SUCCEEDED(hr)) {
        if (!m_pClientFormat) {
          pRealRenderClient->Release();
          return E_UNEXPECTED;
        }
        UINT32 bufferSize = 0;
        m_pRealAudioClient->GetBufferSize(&bufferSize);
        *ppv = new FakeAudioRenderClient(pRealRenderClient, m_pClientFormat,
                                         bufferSize);
        pRealRenderClient->Release();
      }
      return hr;
    }
    return m_pRealAudioClient->GetService(riid, ppv);
  }

  STDMETHODIMP GetBufferSize(UINT32 *pNumBufferFrames) {
    return m_pRealAudioClient->GetBufferSize(pNumBufferFrames);
  }

  STDMETHODIMP GetStreamLatency(REFERENCE_TIME *phnsLatency) {
    REFERENCE_TIME realLatency = 0;
    HRESULT hr = m_pRealAudioClient->GetStreamLatency(&realLatency);
    if (SUCCEEDED(hr) && phnsLatency) {
      *phnsLatency = (REFERENCE_TIME)((double)realLatency / SPEEDUP);
    }
    return hr;
  }

  // --- 欺骗应用程序，让它加速提供数据 ---
  STDMETHODIMP GetCurrentPadding(UINT32 *pNumPaddingFrames) {
    if (!pNumPaddingFrames)
      return E_POINTER;
    UINT32 realPadding = 0;
    HRESULT hr = m_pRealAudioClient->GetCurrentPadding(&realPadding);
    if (SUCCEEDED(hr)) {
      // 返回一个按比例缩小的Padding值
      // 这会让应用程序认为缓冲区消耗得更快，从而更频繁地提供数据
      *pNumPaddingFrames = (UINT32)((double)realPadding / SPEEDUP);
    }
    return hr;
  }

  STDMETHODIMP Start(void) { return m_pRealAudioClient->Start(); }
  STDMETHODIMP Stop(void) { return m_pRealAudioClient->Stop(); }
  STDMETHODIMP Reset(void) { return m_pRealAudioClient->Reset(); }
  STDMETHODIMP SetEventHandle(HANDLE eventHandle) {
    return m_pRealAudioClient->SetEventHandle(eventHandle);
  }
  STDMETHODIMP GetDevicePeriod(REFERENCE_TIME *phnsDefaultDevicePeriod,
                               REFERENCE_TIME *phnsMinimumDevicePeriod) {
    HRESULT hr = m_pRealAudioClient->GetDevicePeriod(phnsDefaultDevicePeriod,
                                                     phnsMinimumDevicePeriod);
    if (SUCCEEDED(hr)) {
      if (phnsDefaultDevicePeriod)
        *phnsDefaultDevicePeriod =
            (REFERENCE_TIME)((double)*phnsDefaultDevicePeriod / SPEEDUP);
      if (phnsMinimumDevicePeriod)
        *phnsMinimumDevicePeriod =
            (REFERENCE_TIME)((double)*phnsMinimumDevicePeriod / SPEEDUP);
    }
    return hr;
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

// --- Hook 核心: 伪造 DllGetClassObject (与之前相同) ---
#define DLLGETCLASSOBJECT
typedef HRESULT(WINAPI *DllGetClassObject_ptr)(REFCLSID, REFIID, LPVOID *);
DllGetClassObject_ptr DllGetClassObject_real = NULL;

HRESULT WINAPI DllGetClassObject_fake(REFCLSID rclsid, REFIID riid,
                                      LPVOID *ppv) {
  HRESULT hr = DllGetClassObject_real(rclsid, riid, ppv);
  if (SUCCEEDED(hr) && rclsid == CLSID_MMDeviceEnumerator) {
    IClassFactory *pRealFactory = (IClassFactory *)*ppv;
    *ppv = new FakeClassFactory(pRealFactory);
    pRealFactory->Release();
  }
  return hr;
}

// --- _hook_setup 和 DllMain (与之前相同) ---
inline void _hook_setup() {
#ifdef DLLGETCLASSOBJECT
  DllGetClassObject_real = (DllGetClassObject_ptr)mProcs[2];
  mProcs[2] = (UINT_PTR)&DllGetClassObject_fake;
#endif
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
    log_info("--- DLL Attached ---");
#endif
  } else if (fdwReason == DLL_PROCESS_DETACH) {
#ifdef _DEBUG
    log_info("--- DLL Detached ---");
    if (debug)
      fclose(debug);
#endif
    FreeLibrary(mHinstDLL);
  }
  return TRUE;
}