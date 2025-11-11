// empty.h

#ifndef _EMPTY_H
#define _EMPTY_H

#include <windows.h>

#include <dsound.h>
#include <map>
#include <utility>
#include <vector>

// [MODIFIED] 添加了 SetFormat 的 VTable 索引
constexpr int IUNKNOWN_RELEASE_INDEX = 2;
constexpr int IDIRECTSOUND_CREATESOUNDBUFFER_INDEX = 3;
constexpr int IDIRECTSOUNDBUFFER_SETFORMAT_INDEX = 14;
constexpr int IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX = 17;

// --- 线程安全 ---
// DirectSound 接口可能会被多个线程调用，必须保护我们的全局 Hook Map
CRITICAL_SECTION g_cs;
// 在 DllMain 的 DLL_PROCESS_ATTACH 中初始化: InitializeCriticalSection(&g_cs);
// 在 DllMain 的 DLL_PROCESS_DETACH 中销毁: DeleteCriticalSection(&g_cs);

// --- 前向声明伪造函数 ---
HRESULT WINAPI CreateSoundBuffer_fake(IDirectSound *pThis,
                                      LPCDSBUFFERDESC pcDSBufferDesc,
                                      LPDIRECTSOUNDBUFFER *ppDSBuffer,
                                      LPUNKNOWN pUnkOuter);
ULONG WINAPI DS_Release_fake(IDirectSound *pThis);
HRESULT WINAPI SetFrequency_fake(IDirectSoundBuffer *pThis, DWORD dwFrequency);
ULONG WINAPI DSB_Release_fake(IDirectSoundBuffer *pThis);
// [MODIFIED] 添加了 SetFormat_fake 的前向声明
HRESULT WINAPI SetFormat_fake(IDirectSoundBuffer *pThis, LPCWAVEFORMATEX pcfx);

// --- 用于存储 Hook 信息的结构体和全局 Map ---

// IDirectSound 接口的 Hook 数据
struct DSoundHook {
  HRESULT(WINAPI *CreateSoundBuffer_orig)(IDirectSound *, LPCDSBUFFERDESC,
                                          LPDIRECTSOUNDBUFFER *, LPUNKNOWN);
  ULONG(WINAPI *Release_orig)(IDirectSound *);
  std::vector<void *> vtable_clone;
};
std::map<IDirectSound *, DSoundHook> g_dsound_hooks;

// IDirectSoundBuffer 接口的 Hook 数据
struct DSoundBufferHook {
  // [MODIFIED] 添加了 SetFormat 的原始函数指针
  HRESULT(WINAPI *SetFormat_orig)(IDirectSoundBuffer *, LPCWAVEFORMATEX);
  HRESULT(WINAPI *SetFrequency_orig)(IDirectSoundBuffer *, DWORD);
  ULONG(WINAPI *Release_orig)(IDirectSoundBuffer *);
  DWORD original_frequency; // 存储缓冲区创建时的原始采样率
  std::vector<void *> vtable_clone;
};
std::map<IDirectSoundBuffer *, DSoundBufferHook> g_dsound_buffer_hooks;

// --- 伪造的 VTable 方法实现 ---

// 伪造的 IDirectSoundBuffer::Release
ULONG WINAPI DSB_Release_fake(IDirectSoundBuffer *pThis) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return 1;
  }

  ULONG refCount = it->second.Release_orig(pThis);
  if (refCount == 0) {
    g_dsound_buffer_hooks.erase(it);
  }
  LeaveCriticalSection(&g_cs);
  return refCount;
}

// [MODIFIED] 新增伪造的 IDirectSoundBuffer::SetFormat 函数
// 目标：拦截 SetFormat 调用，修改其 WAVEFORMATEX 参数中的采样率
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

  // 创建 WAVEFORMATEX 的本地副本以进行修改
  // 我们需要一个足够大的缓冲区来容纳可能的扩展部分 (pcfx->cbSize)
  size_t wfxSize = sizeof(WAVEFORMATEX) + pcfx->cbSize;
  WAVEFORMATEX *localWfx = (WAVEFORMATEX *)_malloca(wfxSize);
  if (!localWfx) {
    LeaveCriticalSection(&g_cs);
    return DSERR_OUTOFMEMORY;
  }
  memcpy(localWfx, pcfx, wfxSize);

  // 更新原始采样率记录
  it->second.original_frequency = pcfx->nSamplesPerSec;

  // 修改采样率
  localWfx->nSamplesPerSec =
      static_cast<DWORD>(static_cast<double>(pcfx->nSamplesPerSec) * SPEEDUP);

  // nAvgBytesPerSec (每秒字节数) 也必须相应更新，否则调用会失败
  localWfx->nAvgBytesPerSec = localWfx->nSamplesPerSec * localWfx->nBlockAlign;

  // 限制频率在 DirectSound 支持的范围内
  localWfx->nSamplesPerSec = max(DSBFREQUENCY_MIN, localWfx->nSamplesPerSec);
  localWfx->nSamplesPerSec = min(DSBFREQUENCY_MAX, localWfx->nSamplesPerSec);
  localWfx->nAvgBytesPerSec = localWfx->nSamplesPerSec * localWfx->nBlockAlign;

  // 调用原始的 SetFormat 函数
  HRESULT hr = it->second.SetFormat_orig(pThis, localWfx);

  _freea(localWfx); // 释放栈上分配的内存
  LeaveCriticalSection(&g_cs);
  return hr;
}

// 伪造的 IDirectSoundBuffer::SetFrequency
HRESULT WINAPI SetFrequency_fake(IDirectSoundBuffer *pThis, DWORD dwFrequency) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return DSERR_GENERIC;
  }

  DWORD baseFrequency = dwFrequency;
  if (dwFrequency == DSBFREQUENCY_ORIGINAL) {
    baseFrequency = it->second.original_frequency;
  }

  DWORD newFrequency =
      static_cast<DWORD>(static_cast<double>(baseFrequency) * SPEEDUP);

  newFrequency = max(DSBFREQUENCY_MIN, newFrequency);
  newFrequency = min(DSBFREQUENCY_MAX, newFrequency);

  HRESULT hr = it->second.SetFrequency_orig(pThis, newFrequency);
  LeaveCriticalSection(&g_cs);
  return hr;
}

// 伪造的 IDirectSound::Release
ULONG WINAPI DS_Release_fake(IDirectSound *pThis) {
  EnterCriticalSection(&g_cs);
  auto it = g_dsound_hooks.find(pThis);
  if (it == g_dsound_hooks.end()) {
    LeaveCriticalSection(&g_cs);
    return 1;
  }

  ULONG refCount = it->second.Release_orig(pThis);
  if (refCount == 0) {
    g_dsound_hooks.erase(it);
  }
  LeaveCriticalSection(&g_cs);
  return refCount;
}

// 伪造的 IDirectSound::CreateSoundBuffer
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
  LeaveCriticalSection(&g_cs);

  if (pcDSBufferDesc == nullptr || pcDSBufferDesc->dwSize == 0) {
    return DSERR_INVALIDPARAM;
  }

  DSBUFFERDESC1 localDesc;
  memset(&localDesc, 0, sizeof(DSBUFFERDESC1));
  memcpy(&localDesc, pcDSBufferDesc, pcDSBufferDesc->dwSize);
  localDesc.dwSize = pcDSBufferDesc->dwSize;

  // [MODIFIED] --- 关键修改：强制使用软件混音 ---
  // 1. 强制添加频率控制能力，确保我们可以调用 SetFrequency
  localDesc.dwFlags |= DSBCAPS_CTRLFREQUENCY;
  // 2. 移除硬件缓冲区标志，因为硬件缓冲区不支持灵活的频率变换
  localDesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
  // 3. 添加软件缓冲区标志，让CPU来处理重采样，这是实现变速的核心
  localDesc.dwFlags |= DSBCAPS_LOCSOFTWARE;
  // 4. (可选但建议) 静态缓冲区通常也是硬件相关的，一并移除
  localDesc.dwFlags &= ~DSBCAPS_STATIC;

  HRESULT hr = it->second.CreateSoundBuffer_orig(
      pThis, (LPCDSBUFFERDESC)&localDesc, ppDSBuffer, pUnkOuter);

  if (SUCCEEDED(hr) && ppDSBuffer && *ppDSBuffer) {
    IDirectSoundBuffer *pBuffer = *ppDSBuffer;

    EnterCriticalSection(&g_cs);
    if (g_dsound_buffer_hooks.count(pBuffer) == 0) {
      void **pVTable = *reinterpret_cast<void ***>(pBuffer);
      const int vtable_size = 24; // 对于 IDirectSoundBuffer8 足够
      DSoundBufferHook hook_data;
      hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);

      hook_data.Release_orig =
          reinterpret_cast<decltype(hook_data.Release_orig)>(
              hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX]);

      // [MODIFIED] 获取原始的 SetFormat 函数指针
      hook_data.SetFormat_orig =
          reinterpret_cast<decltype(hook_data.SetFormat_orig)>(
              hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFORMAT_INDEX]);

      hook_data.SetFrequency_orig =
          reinterpret_cast<decltype(hook_data.SetFrequency_orig)>(
              hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX]);

      if (pcDSBufferDesc->lpwfxFormat) {
        hook_data.original_frequency =
            pcDSBufferDesc->lpwfxFormat->nSamplesPerSec;
      } else {
        hook_data.original_frequency = 0;
      }

      hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DSB_Release_fake;
      // [MODIFIED] 将 VTable 中的 SetFormat 指向我们的伪造函数
      hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFORMAT_INDEX] =
          &SetFormat_fake;
      hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX] =
          &SetFrequency_fake;

      g_dsound_buffer_hooks[pBuffer] = std::move(hook_data);
      *reinterpret_cast<void ***>(pBuffer) =
          g_dsound_buffer_hooks[pBuffer].vtable_clone.data();

      // 创建成功后，立即主动设置一次加速后的频率
      if (g_dsound_buffer_hooks[pBuffer].original_frequency > 0) {
        DWORD newFrequency = static_cast<DWORD>(
            static_cast<double>(
                g_dsound_buffer_hooks[pBuffer].original_frequency) *
            SPEEDUP);
        newFrequency = max(DSBFREQUENCY_MIN, newFrequency);
        newFrequency = min(DSBFREQUENCY_MAX, newFrequency);

        g_dsound_buffer_hooks[pBuffer].SetFrequency_orig(pBuffer, newFrequency);
      }
    }
    LeaveCriticalSection(&g_cs);
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
  const int vtable_size = 12; // 对于 IDirectSound8 足够
  DSoundHook hook_data;
  hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);

  hook_data.Release_orig = reinterpret_cast<decltype(hook_data.Release_orig)>(
      hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX]);
  hook_data.CreateSoundBuffer_orig =
      reinterpret_cast<decltype(hook_data.CreateSoundBuffer_orig)>(
          hook_data.vtable_clone[IDIRECTSOUND_CREATESOUNDBUFFER_INDEX]);

  hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DS_Release_fake;
  hook_data.vtable_clone[IDIRECTSOUND_CREATESOUNDBUFFER_INDEX] =
      &CreateSoundBuffer_fake;

  g_dsound_hooks[pDS] = std::move(hook_data);
  *reinterpret_cast<void ***>(pDS) = g_dsound_hooks[pDS].vtable_clone.data();
  LeaveCriticalSection(&g_cs);
}

// --- 导出的伪造函数实现 ---

// 注意：FAKE 宏的定义应该在一个单独的头文件中，并确保只被包含一次
// 这里为了完整性保留，但在实际项目中建议分离。
#ifndef _HOOK_MACRO_H
#define _HOOK_MACRO_H

#define DECL_FN_PTR(RETTYPE, CONVENTION, FN_NAME, ...)                         \
  typedef RETTYPE(CONVENTION *FN_NAME##_ptr)(__VA_ARGS__)
#define FAKE(RETTYPE, CONVENTION, FN_NAME, ...)                                \
  DECL_FN_PTR(RETTYPE, CONVENTION, FN_NAME, __VA_ARGS__);                      \
  FN_NAME##_ptr FN_NAME##_real;                                                \
  RETTYPE CONVENTION FN_NAME##_fake(__VA_ARGS__)

#endif

FAKE(HRESULT, WINAPI, DirectSoundCreate, LPCGUID lpcGuidDevice,
     LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter) {
  HRESULT hr = DirectSoundCreate_real(lpcGuidDevice, ppDS, pUnkOuter);
  if (SUCCEEDED(hr) && ppDS && *ppDS) {
    HookDirectSound(*ppDS);
  }
  return hr;
}

FAKE(HRESULT, WINAPI, DirectSoundCreate8, LPCGUID lpcGuidDevice,
     LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter) {
  HRESULT hr = DirectSoundCreate8_real(lpcGuidDevice, ppDS8, pUnkOuter);
  if (SUCCEEDED(hr) && ppDS8 && *ppDS8) {
    HookDirectSound(*ppDS8);
  }
  return hr;
}

FAKE(HRESULT, WINAPI, DirectSoundFullDuplexCreate, LPCGUID pcGuidCaptureDevice,
     LPCGUID pcGuidRenderDevice, LPCDSCBUFFERDESC pcDscBufferDesc,
     LPCDSBUFFERDESC pcDsBufferDesc, HWND hwnd, DWORD dwLevel,
     LPDIRECTSOUNDFULLDUPLEX *ppDSFD, LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCB8,
     LPDIRECTSOUNDBUFFER8 *ppDSB8, LPUNKNOWN pUnkOuter) {

  DSBUFFERDESC1 localDsBufferDesc;
  LPCDSBUFFERDESC pModifiedDsBufferDesc = pcDsBufferDesc;

  if (pcDsBufferDesc && pcDsBufferDesc->dwSize > 0) {
    memset(&localDsBufferDesc, 0, sizeof(DSBUFFERDESC1));
    memcpy(&localDsBufferDesc, pcDsBufferDesc, pcDsBufferDesc->dwSize);
    localDsBufferDesc.dwSize = pcDsBufferDesc->dwSize;

    // [MODIFIED] --- 对全双工创建中的缓冲区也应用相同的强制软件混音逻辑 ---
    localDsBufferDesc.dwFlags |= DSBCAPS_CTRLFREQUENCY;
    localDsBufferDesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
    localDsBufferDesc.dwFlags |= DSBCAPS_LOCSOFTWARE;
    localDsBufferDesc.dwFlags &= ~DSBCAPS_STATIC;

    pModifiedDsBufferDesc = (LPCDSBUFFERDESC)&localDsBufferDesc;
  }

  HRESULT hr = DirectSoundFullDuplexCreate_real(
      pcGuidCaptureDevice, pcGuidRenderDevice, pcDscBufferDesc,
      pModifiedDsBufferDesc, // 使用修改后的描述符
      hwnd, dwLevel, ppDSFD, ppDSCB8, ppDSB8, pUnkOuter);

  // [MODIFIED] 对这里创建的缓冲区应用与 CreateSoundBuffer_fake 中完全相同的
  // Hook 逻辑
  if (SUCCEEDED(hr) && ppDSB8 && *ppDSB8) {
    IDirectSoundBuffer *pBuffer = *ppDSB8;

    EnterCriticalSection(&g_cs);
    if (g_dsound_buffer_hooks.count(pBuffer) == 0) {
      void **pVTable = *reinterpret_cast<void ***>(pBuffer);
      const int vtable_size = 24;
      DSoundBufferHook hook_data;
      hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);

      hook_data.Release_orig =
          reinterpret_cast<decltype(hook_data.Release_orig)>(
              hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX]);

      hook_data.SetFormat_orig =
          reinterpret_cast<decltype(hook_data.SetFormat_orig)>(
              hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFORMAT_INDEX]);

      hook_data.SetFrequency_orig =
          reinterpret_cast<decltype(hook_data.SetFrequency_orig)>(
              hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX]);

      if (pcDsBufferDesc && pcDsBufferDesc->lpwfxFormat) {
        hook_data.original_frequency =
            pcDsBufferDesc->lpwfxFormat->nSamplesPerSec;
      } else {
        hook_data.original_frequency = 0;
      }

      hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DSB_Release_fake;
      hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFORMAT_INDEX] =
          &SetFormat_fake;
      hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX] =
          &SetFrequency_fake;

      g_dsound_buffer_hooks[pBuffer] = std::move(hook_data);
      *reinterpret_cast<void ***>(pBuffer) =
          g_dsound_buffer_hooks[pBuffer].vtable_clone.data();

      if (g_dsound_buffer_hooks[pBuffer].original_frequency > 0) {
        DWORD newFrequency = static_cast<DWORD>(
            static_cast<double>(
                g_dsound_buffer_hooks[pBuffer].original_frequency) *
            SPEEDUP);
        newFrequency = max(DSBFREQUENCY_MIN, newFrequency);
        newFrequency = min(DSBFREQUENCY_MAX, newFrequency);
        g_dsound_buffer_hooks[pBuffer].SetFrequency_orig(pBuffer, newFrequency);
      }
    }
    LeaveCriticalSection(&g_cs);
  }

  return hr;
}

#endif // _EMPTY_H