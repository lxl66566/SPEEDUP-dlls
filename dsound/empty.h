#ifndef _EMPTY_H
#define _EMPTY_H

#include <windows.h>

#include "hook_macro.h"
#include <dsound.h>
#include <initguid.h>
#include <malloc.h>
#include <map>
#include <utility>
#include <vector>

// --- VTable 索引 ---
constexpr int IUNKNOWN_QUERYINTERFACE_INDEX = 0;
constexpr int IUNKNOWN_ADDREF_INDEX = 1;
constexpr int IUNKNOWN_RELEASE_INDEX = 2;
constexpr int IDIRECTSOUND_CREATESOUNDBUFFER_INDEX = 3;
constexpr int IDIRECTSOUND_DUPLICATESOUNDBUFFER_INDEX = 5;
constexpr int IDIRECTSOUNDBUFFER_GETFORMAT_INDEX = 5;
constexpr int IDIRECTSOUNDBUFFER_GETFREQUENCY_INDEX = 8;
constexpr int IDIRECTSOUNDBUFFER_SETFORMAT_INDEX = 14;
constexpr int IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX = 17;

// --- 线程安全 ---
CRITICAL_SECTION g_cs;
struct CriticalSectionInitializer {
  CriticalSectionInitializer() { InitializeCriticalSection(&g_cs); }
  ~CriticalSectionInitializer() { DeleteCriticalSection(&g_cs); }
} g_cs_initializer;

// --- 前向声明 ---
void HookDirectSound(IUnknown *pDsound);
void HookDirectSoundBuffer(IDirectSoundBuffer *pBuffer,
                           const WAVEFORMATEX *pOriginalFormat);

// --- 伪造函数前向声明 ---
HRESULT WINAPI CreateSoundBuffer_fake(IDirectSound *pThis,
                                      LPCDSBUFFERDESC pcDSBufferDesc,
                                      LPDIRECTSOUNDBUFFER *ppDSBuffer,
                                      LPUNKNOWN pUnkOuter);
HRESULT WINAPI DuplicateSoundBuffer_fake(
    IDirectSound *pThis, IDirectSoundBuffer *pDSBufferOriginal,
    IDirectSoundBuffer **ppDSBufferDuplicate);
ULONG WINAPI DS_Release_fake(IDirectSound *pThis);
HRESULT WINAPI SetFrequency_fake(IDirectSoundBuffer *pThis, DWORD dwFrequency);
HRESULT WINAPI SetFormat_fake(IDirectSoundBuffer *pThis, LPCWAVEFORMATEX pcfx);
HRESULT WINAPI GetFrequency_fake(IDirectSoundBuffer *pThis,
                                 LPDWORD pdwFrequency);
HRESULT WINAPI GetFormat_fake(IDirectSoundBuffer *pThis, LPWAVEFORMATEX pwfx,
                              DWORD dwSizeAllocated, LPDWORD pdwSizeWritten);
ULONG WINAPI DSB_Release_fake(IDirectSoundBuffer *pThis);

// --- 用于存储 Hook 信息的结构体和全局 Map ---
struct DSoundHook {
  HRESULT(WINAPI *CreateSoundBuffer_orig)(IDirectSound *, LPCDSBUFFERDESC,
                                          LPDIRECTSOUNDBUFFER *, LPUNKNOWN);
  HRESULT(WINAPI *DuplicateSoundBuffer_orig)(IDirectSound *,
                                             IDirectSoundBuffer *,
                                             IDirectSoundBuffer **);
  ULONG(WINAPI *Release_orig)(IDirectSound *);
  std::vector<void *> vtable_clone;
};
std::map<IDirectSound *, DSoundHook> g_dsound_hooks;

struct DSoundBufferHook {
  HRESULT(WINAPI *SetFormat_orig)(IDirectSoundBuffer *, LPCWAVEFORMATEX);
  HRESULT(WINAPI *SetFrequency_orig)(IDirectSoundBuffer *, DWORD);
  HRESULT(WINAPI *GetFormat_orig)(IDirectSoundBuffer *, LPWAVEFORMATEX, DWORD,
                                  LPDWORD);
  HRESULT(WINAPI *GetFrequency_orig)(IDirectSoundBuffer *, LPDWORD);
  ULONG(WINAPI *Release_orig)(IDirectSoundBuffer *);
  WAVEFORMATEX original_format;
  std::vector<void *> vtable_clone;
};
std::map<IDirectSoundBuffer *, DSoundBufferHook> g_dsound_buffer_hooks;

// --- 辅助函数：Hook 单个 Sound Buffer ---
void HookDirectSoundBuffer(IDirectSoundBuffer *pBuffer,
                           const WAVEFORMATEX *pOriginalFormat) {
  EnterCriticalSection(&g_cs);
  if (g_dsound_buffer_hooks.count(pBuffer) == 0) {
    void **pVTable = *reinterpret_cast<void ***>(pBuffer);
    const int vtable_size = 24;
    DSoundBufferHook hook_data;
    hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);
    hook_data.Release_orig = reinterpret_cast<decltype(hook_data.Release_orig)>(
        pVTable[IUNKNOWN_RELEASE_INDEX]);
    hook_data.GetFormat_orig =
        reinterpret_cast<decltype(hook_data.GetFormat_orig)>(
            pVTable[IDIRECTSOUNDBUFFER_GETFORMAT_INDEX]);
    hook_data.GetFrequency_orig =
        reinterpret_cast<decltype(hook_data.GetFrequency_orig)>(
            pVTable[IDIRECTSOUNDBUFFER_GETFREQUENCY_INDEX]);
    hook_data.SetFormat_orig =
        reinterpret_cast<decltype(hook_data.SetFormat_orig)>(
            pVTable[IDIRECTSOUNDBUFFER_SETFORMAT_INDEX]);
    hook_data.SetFrequency_orig =
        reinterpret_cast<decltype(hook_data.SetFrequency_orig)>(
            pVTable[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX]);
    if (pOriginalFormat) {
      size_t wfxSize = sizeof(WAVEFORMATEX) + pOriginalFormat->cbSize;
      memcpy(&hook_data.original_format, pOriginalFormat, wfxSize);
    } else {
      WAVEFORMATEX current_format;
      DWORD size_written = 0;
      hook_data.GetFormat_orig(pBuffer, &current_format, sizeof(WAVEFORMATEX),
                               &size_written);
      if (size_written > 0) {
        memcpy(&hook_data.original_format, &current_format, size_written);
        hook_data.original_format.nSamplesPerSec = static_cast<DWORD>(
            static_cast<double>(hook_data.original_format.nSamplesPerSec) /
            SPEEDUP);
        hook_data.original_format.nAvgBytesPerSec =
            hook_data.original_format.nSamplesPerSec *
            hook_data.original_format.nBlockAlign;
      }
    }
    hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DSB_Release_fake;
    hook_data.vtable_clone[IDIRECTSOUNDBUFFER_GETFORMAT_INDEX] =
        &GetFormat_fake;
    hook_data.vtable_clone[IDIRECTSOUNDBUFFER_GETFREQUENCY_INDEX] =
        &GetFrequency_fake;
    hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFORMAT_INDEX] =
        &SetFormat_fake;
    hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX] =
        &SetFrequency_fake;
    g_dsound_buffer_hooks[pBuffer] = std::move(hook_data);
    *reinterpret_cast<void ***>(pBuffer) =
        g_dsound_buffer_hooks[pBuffer].vtable_clone.data();
  }
  LeaveCriticalSection(&g_cs);
}

// --- 伪造的 VTable 方法实现 ---
ULONG WINAPI DSB_Release_fake(IDirectSoundBuffer *pThis) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return 1;
  }
  ULONG refCount = it->second.Release_orig(pThis);
  if (refCount == 0)
    g_dsound_buffer_hooks.erase(it);
  LeaveCriticalSection(&g_cs);
  return refCount;
}
HRESULT WINAPI GetFormat_fake(IDirectSoundBuffer *pThis, LPWAVEFORMATEX pwfx,
                              DWORD dwSizeAllocated, LPDWORD pdwSizeWritten) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return DSERR_GENERIC;
  }
  DSoundBufferHook &hook_data = it->second;
  LeaveCriticalSection(&g_cs);
  if (pwfx == nullptr && pdwSizeWritten == nullptr)
    return DSERR_INVALIDPARAM;
  DWORD requiredSize = sizeof(WAVEFORMATEX) + hook_data.original_format.cbSize;
  if (pdwSizeWritten)
    *pdwSizeWritten = requiredSize;
  if (pwfx) {
    if (dwSizeAllocated < requiredSize)
      return DSERR_INVALIDPARAM;
    memcpy(pwfx, &hook_data.original_format, requiredSize);
  }
  return DS_OK;
}
HRESULT WINAPI GetFrequency_fake(IDirectSoundBuffer *pThis,
                                 LPDWORD pdwFrequency) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return DSERR_GENERIC;
  }
  if (pdwFrequency == nullptr) {
    LeaveCriticalSection(&g_cs);
    return DSERR_INVALIDPARAM;
  }
  DWORD realFrequency;
  HRESULT hr = it->second.GetFrequency_orig(pThis, &realFrequency);
  LeaveCriticalSection(&g_cs);
  if (SUCCEEDED(hr))
    *pdwFrequency =
        static_cast<DWORD>(static_cast<double>(realFrequency) / SPEEDUP);
  return hr;
}
HRESULT WINAPI SetFormat_fake(IDirectSoundBuffer *pThis, LPCWAVEFORMATEX pcfx) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return DSERR_GENERIC;
  }
  if (pcfx == nullptr) {
    LeaveCriticalSection(&g_cs);
    return DSERR_INVALIDPARAM;
  }
  size_t wfxSize = sizeof(WAVEFORMATEX) + pcfx->cbSize;
  WAVEFORMATEX *localWfx = (WAVEFORMATEX *)_malloca(wfxSize);
  if (!localWfx) {
    LeaveCriticalSection(&g_cs);
    return DSERR_OUTOFMEMORY;
  }
  memcpy(localWfx, pcfx, wfxSize);
  memcpy(&it->second.original_format, pcfx, wfxSize);
  localWfx->nSamplesPerSec =
      static_cast<DWORD>(static_cast<double>(pcfx->nSamplesPerSec) * SPEEDUP);
  localWfx->nSamplesPerSec =
      max(DSBFREQUENCY_MIN, min(DSBFREQUENCY_MAX, localWfx->nSamplesPerSec));
  localWfx->nAvgBytesPerSec = localWfx->nSamplesPerSec * localWfx->nBlockAlign;
  HRESULT hr = it->second.SetFormat_orig(pThis, localWfx);
  _freea(localWfx);
  LeaveCriticalSection(&g_cs);
  return hr;
}
HRESULT WINAPI SetFrequency_fake(IDirectSoundBuffer *pThis, DWORD dwFrequency) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return DSERR_GENERIC;
  }
  DWORD baseFrequency = (dwFrequency == DSBFREQUENCY_ORIGINAL)
                            ? it->second.original_format.nSamplesPerSec
                            : dwFrequency;
  DWORD newFrequency =
      static_cast<DWORD>(static_cast<double>(baseFrequency) * SPEEDUP);
  newFrequency = max(DSBFREQUENCY_MIN, min(DSBFREQUENCY_MAX, newFrequency));
  HRESULT hr = it->second.SetFrequency_orig(pThis, newFrequency);
  LeaveCriticalSection(&g_cs);
  return hr;
}
ULONG WINAPI DS_Release_fake(IDirectSound *pThis) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_hooks.find(pThis);
  if (it == g_dsound_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return 1;
  }
  ULONG refCount = it->second.Release_orig(pThis);
  if (refCount == 0)
    g_dsound_hooks.erase(it);
  LeaveCriticalSection(&g_cs);
  return refCount;
}
HRESULT WINAPI CreateSoundBuffer_fake(IDirectSound *pThis,
                                      LPCDSBUFFERDESC pcDSBufferDesc,
                                      LPDIRECTSOUNDBUFFER *ppDSBuffer,
                                      LPUNKNOWN pUnkOuter) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_hooks.find(pThis);
  if (it == g_dsound_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return DSERR_GENERIC;
  }
  auto CreateSoundBuffer_orig = it->second.CreateSoundBuffer_orig;
  LeaveCriticalSection(&g_cs);
  if (pcDSBufferDesc == nullptr || pcDSBufferDesc->dwSize == 0 ||
      ppDSBuffer == nullptr)
    return DSERR_INVALIDPARAM;
  if (pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER)
    return CreateSoundBuffer_orig(pThis, pcDSBufferDesc, ppDSBuffer, pUnkOuter);
  DSBUFFERDESC localDesc;
  memcpy(&localDesc, pcDSBufferDesc, pcDSBufferDesc->dwSize);
  WAVEFORMATEX *modified_format = nullptr;
  if (localDesc.lpwfxFormat) {
    size_t wfxSize = sizeof(WAVEFORMATEX) + localDesc.lpwfxFormat->cbSize;
    modified_format = (WAVEFORMATEX *)_malloca(wfxSize);
    if (!modified_format)
      return DSERR_OUTOFMEMORY;
    memcpy(modified_format, localDesc.lpwfxFormat, wfxSize);
    modified_format->nSamplesPerSec = static_cast<DWORD>(
        static_cast<double>(modified_format->nSamplesPerSec) * SPEEDUP);
    modified_format->nSamplesPerSec =
        max(DSBFREQUENCY_MIN,
            min(DSBFREQUENCY_MAX, modified_format->nSamplesPerSec));
    modified_format->nAvgBytesPerSec =
        modified_format->nSamplesPerSec * modified_format->nBlockAlign;
    localDesc.lpwfxFormat = modified_format;
  }
  localDesc.dwFlags |= DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
  localDesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
  HRESULT hr = CreateSoundBuffer_orig(pThis, &localDesc, ppDSBuffer, pUnkOuter);
  if (modified_format)
    _freea(modified_format);
  if (SUCCEEDED(hr) && ppDSBuffer && *ppDSBuffer)
    HookDirectSoundBuffer(*ppDSBuffer, pcDSBufferDesc->lpwfxFormat);
  return hr;
}
HRESULT WINAPI DuplicateSoundBuffer_fake(
    IDirectSound *pThis, IDirectSoundBuffer *pDSBufferOriginal,
    IDirectSoundBuffer **ppDSBufferDuplicate) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_hooks.find(pThis);
  if (it == g_dsound_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return DSERR_GENERIC;
  }
  auto DuplicateSoundBuffer_orig = it->second.DuplicateSoundBuffer_orig;
  LeaveCriticalSection(&g_cs);
  HRESULT hr =
      DuplicateSoundBuffer_orig(pThis, pDSBufferOriginal, ppDSBufferDuplicate);
  if (SUCCEEDED(hr) && ppDSBufferDuplicate && *ppDSBufferDuplicate) {
    WAVEFORMATEX original_format = {0};
    EnterCriticalSection(&g_cs);
    auto orig_it = g_dsound_buffer_hooks.find(pDSBufferOriginal);
    if (orig_it != g_dsound_buffer_hooks.end()) {
      size_t wfxSize =
          sizeof(WAVEFORMATEX) + orig_it->second.original_format.cbSize;
      memcpy(&original_format, &orig_it->second.original_format, wfxSize);
    }
    LeaveCriticalSection(&g_cs);
    HookDirectSoundBuffer(*ppDSBufferDuplicate,
                          (original_format.wFormatTag != 0) ? &original_format
                                                            : nullptr);
  }
  return hr;
}

// --- 核心 Hook 函数 ---
void HookDirectSound(IUnknown *pDsound) {
  IDirectSound *pDS = static_cast<IDirectSound *>(pDsound);
  EnterCriticalSection(&g_cs);
  if (g_dsound_hooks.count(pDS)) {
    LeaveCriticalSection(&g_cs);
    return;
  }
  void **pVTable = *reinterpret_cast<void ***>(pDS);
  const int vtable_size = 12;
  DSoundHook hook_data;
  hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);
  hook_data.Release_orig = reinterpret_cast<decltype(hook_data.Release_orig)>(
      pVTable[IUNKNOWN_RELEASE_INDEX]);
  hook_data.CreateSoundBuffer_orig =
      reinterpret_cast<decltype(hook_data.CreateSoundBuffer_orig)>(
          pVTable[IDIRECTSOUND_CREATESOUNDBUFFER_INDEX]);
  hook_data.DuplicateSoundBuffer_orig =
      reinterpret_cast<decltype(hook_data.DuplicateSoundBuffer_orig)>(
          pVTable[IDIRECTSOUND_DUPLICATESOUNDBUFFER_INDEX]);
  hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DS_Release_fake;
  hook_data.vtable_clone[IDIRECTSOUND_CREATESOUNDBUFFER_INDEX] =
      &CreateSoundBuffer_fake;
  hook_data.vtable_clone[IDIRECTSOUND_DUPLICATESOUNDBUFFER_INDEX] =
      &DuplicateSoundBuffer_fake;
  g_dsound_hooks[pDS] = std::move(hook_data);
  *reinterpret_cast<void ***>(pDS) = g_dsound_hooks[pDS].vtable_clone.data();
  LeaveCriticalSection(&g_cs);
}

// 手动声明 DllGetClassObject 相关指针和函数原型，以解决编译顺序问题
DECL_FN_PTR(HRESULT, STDAPICALLTYPE, DllGetClassObject, REFCLSID rclsid,
            REFIID riid, LPVOID *ppv);
DllGetClassObject_ptr DllGetClassObject_real;
HRESULT STDAPICALLTYPE DllGetClassObject_fake(REFCLSID rclsid, REFIID riid,
                                              LPVOID *ppv);

// --- DllGetClassObject 的 IClassFactory 实现 (关键!) ---
class DSClassFactory : public IClassFactory {
public:
  DSClassFactory() : m_refCount(1) {}
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown || riid == IID_IClassFactory) {
      *ppv = static_cast<IClassFactory *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
  STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_refCount); }
  STDMETHODIMP_(ULONG) Release() {
    ULONG ref = InterlockedDecrement(&m_refCount);
    if (ref == 0)
      delete this;
    return ref;
  }
  STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv) {
    if (pUnkOuter != nullptr)
      return CLASS_E_NOAGGREGATION;
    IClassFactory *pRealFactory = nullptr;
    // 现在 DllGetClassObject_real 在这里是已声明的
    HRESULT hr = DllGetClassObject_real(CLSID_DirectSound, IID_IClassFactory,
                                        (void **)&pRealFactory);
    if (FAILED(hr))
      return hr;
    hr = pRealFactory->CreateInstance(pUnkOuter, riid, ppv);
    pRealFactory->Release();
    if (SUCCEEDED(hr) && ppv && *ppv)
      HookDirectSound(static_cast<IUnknown *>(*ppv));
    return hr;
  }
  STDMETHODIMP LockServer(BOOL fLock) { return S_OK; }

private:
  LONG m_refCount;
};

#define DIRECTSOUNDCREATE
FAKE(HRESULT, WINAPI, DirectSoundCreate, LPCGUID lpcGuidDevice,
     LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter) {
  HRESULT hr = DirectSoundCreate_real(lpcGuidDevice, ppDS, pUnkOuter);
  if (SUCCEEDED(hr) && ppDS && *ppDS)
    HookDirectSound(*ppDS);
  return hr;
}

#define DIRECTSOUNDCREATE8
FAKE(HRESULT, WINAPI, DirectSoundCreate8, LPCGUID lpcGuidDevice,
     LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter) {
  HRESULT hr = DirectSoundCreate8_real(lpcGuidDevice, ppDS8, pUnkOuter);
  if (SUCCEEDED(hr) && ppDS8 && *ppDS8)
    HookDirectSound(*ppDS8);
  return hr;
}

#define DIRECTSOUNDFULLDUPLEXCREATE
FAKE(HRESULT, WINAPI, DirectSoundFullDuplexCreate, LPCGUID pcGuidCaptureDevice,
     LPCGUID pcGuidRenderDevice, LPCDSCBUFFERDESC pcDscBufferDesc,
     LPCDSBUFFERDESC pcDsBufferDesc, HWND hwnd, DWORD dwLevel,
     LPDIRECTSOUNDFULLDUPLEX *ppDSFD, LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCB8,
     LPDIRECTSOUNDBUFFER8 *ppDSB8, LPUNKNOWN pUnkOuter) {
  DSBUFFERDESC localDesc;
  LPCDSBUFFERDESC pModifiedDsBufferDesc = pcDsBufferDesc;
  WAVEFORMATEX *modified_format = nullptr;
  if (pcDsBufferDesc && pcDsBufferDesc->dwSize > 0) {
    memcpy(&localDesc, pcDsBufferDesc, pcDsBufferDesc->dwSize);
    if (localDesc.lpwfxFormat) {
      size_t wfxSize = sizeof(WAVEFORMATEX) + localDesc.lpwfxFormat->cbSize;
      modified_format = (WAVEFORMATEX *)_malloca(wfxSize);
      if (!modified_format)
        return DSERR_OUTOFMEMORY;
      memcpy(modified_format, localDesc.lpwfxFormat, wfxSize);
      modified_format->nSamplesPerSec = static_cast<DWORD>(
          static_cast<double>(modified_format->nSamplesPerSec) * SPEEDUP);
      modified_format->nSamplesPerSec =
          max(DSBFREQUENCY_MIN,
              min(DSBFREQUENCY_MAX, modified_format->nSamplesPerSec));
      modified_format->nAvgBytesPerSec =
          modified_format->nSamplesPerSec * modified_format->nBlockAlign;
      localDesc.lpwfxFormat = modified_format;
    }
    localDesc.dwFlags |= DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
    localDesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
    pModifiedDsBufferDesc = &localDesc;
  }
  HRESULT hr = DirectSoundFullDuplexCreate_real(
      pcGuidCaptureDevice, pcGuidRenderDevice, pcDscBufferDesc,
      pModifiedDsBufferDesc, hwnd, dwLevel, ppDSFD, ppDSCB8, ppDSB8, pUnkOuter);
  if (modified_format)
    _freea(modified_format);
  if (SUCCEEDED(hr) && ppDSB8 && *ppDSB8)
    HookDirectSoundBuffer(*ppDSB8, pcDsBufferDesc ? pcDsBufferDesc->lpwfxFormat
                                                  : nullptr);
  return hr;
}

// DllGetClassObject_fake 的实现
HRESULT STDAPICALLTYPE DllGetClassObject_fake(REFCLSID rclsid, REFIID riid,
                                              LPVOID *ppv) {
  if (rclsid == CLSID_DirectSound || rclsid == CLSID_DirectSound8) {
    if (riid == IID_IClassFactory) {
      DSClassFactory *factory = new (std::nothrow) DSClassFactory();
      if (!factory)
        return E_OUTOFMEMORY;
      *ppv = static_cast<IClassFactory *>(factory);
      return S_OK;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
  }
  return DllGetClassObject_real(rclsid, riid, ppv);
}

#endif // _EMPTY_H