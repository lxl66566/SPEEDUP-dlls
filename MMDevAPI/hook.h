#ifndef _HOOK_H_
#define _HOOK_H_

#include <audioclient.h>
#include <initguid.h>
#include <mmdeviceapi.h>

#include "hook_macro.h"

// --- COM 接口的 GUIDs ---
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D,
            0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46,
            0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2,
            0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IMMDevice, 0xD666063F, 0x1587, 0x4E43, 0x81, 0xF1, 0xB9, 0x48,
            0xE8, 0x07, 0x36, 0x3F);
DEFINE_GUID(IID_IAudioClock, 0xCD63314F, 0x3F04, 0x4c06, 0x9F, 0x54, 0x77, 0xA7,
            0x48, 0xAB, 0x3C, 0x68);
DEFINE_GUID(IID_IMMDeviceCollection, 0x0BD7A1BE, 0x7A1A, 0x44DB, 0x83, 0x97,
            0xCC, 0x53, 0x9C, 0xB3, 0x5D, 0x69);

void ModifyWaveFormatForSpeedup(WAVEFORMATEX *fmt) {
  if (!fmt)
    return;
  fmt->nSamplesPerSec = (DWORD)((double)fmt->nSamplesPerSec * SPEEDUP + 0.5);
  fmt->nAvgBytesPerSec = fmt->nSamplesPerSec * fmt->nBlockAlign;
}

void RevertWaveFormatFromSpeedup(WAVEFORMATEX *fmt) {
  if (!fmt)
    return;
  fmt->nSamplesPerSec = (DWORD)((double)fmt->nSamplesPerSec / SPEEDUP + 0.5);
  fmt->nAvgBytesPerSec = fmt->nSamplesPerSec * fmt->nBlockAlign;
}

class FakeAudioClock : public IAudioClock {
private:
  IAudioClock *m_pRealAudioClock;
  LONG m_cRef;

public:
  FakeAudioClock(IAudioClock *pReal) : m_pRealAudioClock(pReal), m_cRef(1) {
    m_pRealAudioClock->AddRef();
  }
  virtual ~FakeAudioClock() {
    if (m_pRealAudioClock)
      m_pRealAudioClock->Release();
  }
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_IAudioClock) {
      *ppv = static_cast<IAudioClock *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = NULL;
    return m_pRealAudioClock->QueryInterface(riid, ppv);
  }
  STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
  STDMETHODIMP_(ULONG) Release() {
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef)
      delete this;
    return ulRef;
  }
  STDMETHODIMP GetFrequency(UINT64 *pu64Frequency) {
    HRESULT hr = m_pRealAudioClock->GetFrequency(pu64Frequency);
    if (SUCCEEDED(hr) && pu64Frequency) {
      *pu64Frequency = (UINT64)((double)*pu64Frequency / SPEEDUP + 0.5);
    }
    return hr;
  }

  STDMETHODIMP GetPosition(UINT64 *pu64Position, UINT64 *pu64QPCPosition) {
    HRESULT hr = m_pRealAudioClock->GetPosition(pu64Position, pu64QPCPosition);
    if (SUCCEEDED(hr) && pu64Position) {
      *pu64Position = (UINT64)((double)*pu64Position / SPEEDUP + 0.5);
    }
    return hr;
  }
  STDMETHODIMP GetCharacteristics(DWORD *pdwCharacteristics) {
    return m_pRealAudioClock->GetCharacteristics(pdwCharacteristics);
  }
};

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
    if (!pFormat) {
      // 如果 pFormat 为空，直接透传
      return m_pRealAudioClient->Initialize(ShareMode, StreamFlags,
                                            hnsBufferDuration, hnsPeriodicity,
                                            pFormat, AudioSessionGuid);
    }

    // 准备被修改的格式
    size_t fmt_size = sizeof(WAVEFORMATEX) + pFormat->cbSize;
    modified_fmt = (WAVEFORMATEX *)malloc(fmt_size);
    if (!modified_fmt)
      return E_OUTOFMEMORY;
    memcpy(modified_fmt, pFormat, fmt_size);

    // 获取原始采样率，以供后续计算
    DWORD originalSampleRate = pFormat->nSamplesPerSec;

    // 使用我们更新后的函数修改格式
    ModifyWaveFormatForSpeedup(modified_fmt);
    DWORD newSampleRate = modified_fmt->nSamplesPerSec;

    // 1. 计算应用程序期望的缓冲区帧数
    UINT32 numBufferFrames =
        (UINT32)((double)hnsBufferDuration * originalSampleRate / 10000000.0 +
                 0.5);

    // 2. 根据期望的帧数和新的采样率，反推出修改后的缓冲区时长
    REFERENCE_TIME modified_duration =
        (REFERENCE_TIME)((double)numBufferFrames * 10000000.0 / newSampleRate +
                         0.5);

    REFERENCE_TIME modified_period = 0;
    // 仅在 hnsPeriodicity 不为0时计算（0有特殊含义，表示自动选择）
    if (hnsPeriodicity != 0) {
      // 3. 计算应用程序期望的周期帧数
      UINT32 numPeriodFrames =
          (UINT32)((double)hnsPeriodicity * originalSampleRate / 10000000.0 +
                   0.5);
      // 4. 根据期望的帧数和新的采样率，反推出修改后的周期
      modified_period = (REFERENCE_TIME)((double)numPeriodFrames * 10000000.0 /
                                             newSampleRate +
                                         0.5);
    }

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
  STDMETHODIMP GetBufferSize(UINT32 *pNumBufferFrames) {
    return m_pRealAudioClient->GetBufferSize(pNumBufferFrames);
  }
  STDMETHODIMP GetStreamLatency(REFERENCE_TIME *phnsLatency) {
    HRESULT hr = m_pRealAudioClient->GetStreamLatency(phnsLatency);
    if (SUCCEEDED(hr) && phnsLatency) {
      // 应用程序看到的时间应该是被拉长过的
      *phnsLatency = (REFERENCE_TIME)((double)*phnsLatency * SPEEDUP);
    }
    return hr;
  }
  STDMETHODIMP GetCurrentPadding(UINT32 *pNumPaddingFrames) {
    return m_pRealAudioClient->GetCurrentPadding(pNumPaddingFrames);
  }
  STDMETHODIMP GetService(REFIID riid, void **ppv) {
    if (riid == IID_IAudioClock) {
      IAudioClock *pRealClock = NULL;
      HRESULT hr = m_pRealAudioClient->GetService(riid, (void **)&pRealClock);
      if (SUCCEEDED(hr) && pRealClock) {
        *ppv = new FakeAudioClock(pRealClock);
        pRealClock->Release();
      }
      return hr;
    }
    // 对于其他服务，直接透传
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
    HRESULT hr = m_pRealAudioClient->GetDevicePeriod(phnsDefaultDevicePeriod,
                                                     phnsMinimumDevicePeriod);
    if (SUCCEEDED(hr)) {
      // 同样，应用程序看到的时间周期也应该是拉长过的
      if (phnsDefaultDevicePeriod) {
        *phnsDefaultDevicePeriod =
            (REFERENCE_TIME)((double)*phnsDefaultDevicePeriod * SPEEDUP);
      }
      if (phnsMinimumDevicePeriod) {
        *phnsMinimumDevicePeriod =
            (REFERENCE_TIME)((double)*phnsMinimumDevicePeriod * SPEEDUP);
      }
    }
    return hr;
  }
};

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

class FakeDeviceCollection : public IMMDeviceCollection {
private:
  IMMDeviceCollection *m_pRealCollection;
  LONG m_cRef;

public:
  FakeDeviceCollection(IMMDeviceCollection *pReal)
      : m_pRealCollection(pReal), m_cRef(1) {
    m_pRealCollection->AddRef();
  }
  virtual ~FakeDeviceCollection() {
    if (m_pRealCollection)
      m_pRealCollection->Release();
  }
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_IMMDeviceCollection) {
      *ppv = static_cast<IMMDeviceCollection *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = NULL;
    return m_pRealCollection->QueryInterface(riid, ppv);
  }
  STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
  STDMETHODIMP_(ULONG) Release() {
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef)
      delete this;
    return ulRef;
  }
  STDMETHODIMP GetCount(UINT *pcDevices) {
    return m_pRealCollection->GetCount(pcDevices);
  }
  STDMETHODIMP Item(UINT nDevice, IMMDevice **ppDevice) {
    HRESULT hr = m_pRealCollection->Item(nDevice, ppDevice);
    if (SUCCEEDED(hr) && ppDevice && *ppDevice) {
      // 确保从集合中获取的设备也是伪造的
      IMMDevice *pRealDevice = *ppDevice;
      *ppDevice = new FakeDevice(pRealDevice);
      pRealDevice->Release();
    }
    return hr;
  }
};

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
    HRESULT hr =
        m_pRealEnumerator->EnumAudioEndpoints(dataFlow, dwStateMask, ppDevices);
    if (SUCCEEDED(hr) && ppDevices && *ppDevices) {
      IMMDeviceCollection *pRealCollection = *ppDevices;
      *ppDevices = new FakeDeviceCollection(pRealCollection);
      pRealCollection->Release();
    }
    return hr;
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

#define DLLGETCLASSOBJECT

FAKE(HRESULT, WINAPI, DllGetClassObject, REFCLSID rclsid, REFIID riid,
     LPVOID *ppv) {
  HRESULT hr = DllGetClassObject_real(rclsid, riid, ppv);

  if (SUCCEEDED(hr) && rclsid == CLSID_MMDeviceEnumerator) {
    IClassFactory *pRealFactory = (IClassFactory *)*ppv;
    *ppv = new FakeClassFactory(pRealFactory);
    pRealFactory->Release();
  }
  return hr;
}

#endif