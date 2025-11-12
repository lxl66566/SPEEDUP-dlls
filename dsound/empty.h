#ifndef _EMPTY_H
#define _EMPTY_H

#include <windows.h>

#include <dsound.h>
#include <malloc.h>
#include <map>
#include <utility>
#include <vector>

#include "hook_macro.h"

constexpr int IUNKNOWN_QUERYINTERFACE_INDEX = 0;
constexpr int IUNKNOWN_ADDREF_INDEX = 1;
constexpr int IUNKNOWN_RELEASE_INDEX = 2;

constexpr int IDIRECTSOUND_CREATESOUNDBUFFER_INDEX = 3;

constexpr int IDIRECTSOUNDBUFFER_GETCAPS_INDEX = 3;
constexpr int IDIRECTSOUNDBUFFER_GETCURRENTPOSITION_INDEX = 4;
constexpr int IDIRECTSOUNDBUFFER_GETFORMAT_INDEX = 5;
constexpr int IDIRECTSOUNDBUFFER_GETFREQUENCY_INDEX = 8;
constexpr int IDIRECTSOUNDBUFFER_SETFORMAT_INDEX = 14;
constexpr int IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX = 17;

// --- 前向声明伪造函数 ---
HRESULT WINAPI CreateSoundBuffer_fake(IDirectSound *pThis,
                                      LPCDSBUFFERDESC pcDSBufferDesc,
                                      LPDIRECTSOUNDBUFFER *ppDSBuffer,
                                      LPUNKNOWN pUnkOuter);
ULONG WINAPI DS_Release_fake(IDirectSound *pThis);

HRESULT WINAPI SetFrequency_fake(IDirectSoundBuffer *pThis, DWORD dwFrequency);
HRESULT WINAPI SetFormat_fake(IDirectSoundBuffer *pThis, LPCWAVEFORMATEX pcfx);
HRESULT WINAPI GetFrequency_fake(IDirectSoundBuffer *pThis,
                                 LPDWORD pdwFrequency);
HRESULT WINAPI GetFormat_fake(IDirectSoundBuffer *pThis, LPWAVEFORMATEX pwfx,
                              DWORD dwSizeAllocated, LPDWORD pdwSizeWritten);
ULONG WINAPI DSB_Release_fake(IDirectSoundBuffer *pThis);

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
  HRESULT(WINAPI *SetFormat_orig)(IDirectSoundBuffer *, LPCWAVEFORMATEX);
  HRESULT(WINAPI *SetFrequency_orig)(IDirectSoundBuffer *, DWORD);
  // [MODIFIED] 添加 GetFormat 和 GetFrequency 的原始函数指针
  HRESULT(WINAPI *GetFormat_orig)(IDirectSoundBuffer *, LPWAVEFORMATEX, DWORD,
                                  LPDWORD);
  HRESULT(WINAPI *GetFrequency_orig)(IDirectSoundBuffer *, LPDWORD);
  ULONG(WINAPI *Release_orig)(IDirectSoundBuffer *);

  // [MODIFIED] 存储原始格式信息，用于“撒谎”
  WAVEFORMATEX original_format;
  std::vector<void *> vtable_clone;
};
std::map<IDirectSoundBuffer *, DSoundBufferHook> g_dsound_buffer_hooks;

// --- 伪造的 VTable 方法实现 ---

// 伪造的 IDirectSoundBuffer::Release
ULONG WINAPI DSB_Release_fake(IDirectSoundBuffer *pThis) {
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    // 可能是一个我们没有Hook的缓冲区（例如Primary Buffer），直接返回
    // 理论上不应该发生，但作为保护
    return 1;
  }

  // 调用原始 Release
  ULONG refCount = it->second.Release_orig(pThis);

  // 如果引用计数为0，对象将被销毁，从我们的map中移除
  if (refCount == 0) {
    g_dsound_buffer_hooks.erase(it);
  }
  return refCount;
}

// [NEW] 伪造的 IDirectSoundBuffer::GetFormat
// 目标：向应用程序报告原始的、未经修改的音频格式
HRESULT WINAPI GetFormat_fake(IDirectSoundBuffer *pThis, LPWAVEFORMATEX pwfx,
                              DWORD dwSizeAllocated, LPDWORD pdwSizeWritten) {
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    return DSERR_GENERIC;
  }

  DSoundBufferHook &hook_data = it->second;

  if (pwfx == nullptr && pdwSizeWritten == nullptr) {
    return DSERR_INVALIDPARAM;
  }

  DWORD requiredSize = sizeof(WAVEFORMATEX) + hook_data.original_format.cbSize;

  if (pdwSizeWritten) {
    *pdwSizeWritten = requiredSize;
  }

  if (pwfx) {
    if (dwSizeAllocated < requiredSize) {
      // 缓冲区太小，无法写入完整的格式信息
      return DSERR_INVALIDPARAM;
    }
    // 复制我们保存的原始格式信息给应用程序
    memcpy(pwfx, &hook_data.original_format, requiredSize);
  }

  return DS_OK;
}

HRESULT WINAPI GetFrequency_fake(IDirectSoundBuffer *pThis,
                                 LPDWORD pdwFrequency) {
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    return DSERR_GENERIC;
  }

  if (pdwFrequency == nullptr) {
    return DSERR_INVALIDPARAM;
  }

  // 调用原始函数获取真实的、加速后的频率
  DWORD realFrequency;
  HRESULT hr = it->second.GetFrequency_orig(pThis, &realFrequency);

  if (SUCCEEDED(hr)) {
    // 计算出原始频率并返回
    *pdwFrequency =
        static_cast<DWORD>(static_cast<double>(realFrequency) / SPEEDUP);
  }

  return hr;
}

// 伪造的 IDirectSoundBuffer::SetFormat
HRESULT WINAPI SetFormat_fake(IDirectSoundBuffer *pThis, LPCWAVEFORMATEX pcfx) {
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    return DSERR_GENERIC;
  }

  if (pcfx == nullptr) {
    return DSERR_INVALIDPARAM;
  }

  // 创建 WAVEFORMATEX 的本地副本以进行修改
  size_t wfxSize = sizeof(WAVEFORMATEX) + pcfx->cbSize;
  WAVEFORMATEX *localWfx = (WAVEFORMATEX *)_malloca(wfxSize);
  if (!localWfx) {
    return DSERR_OUTOFMEMORY;
  }
  memcpy(localWfx, pcfx, wfxSize);

  // 更新我们保存的“原始格式”记录
  memcpy(&it->second.original_format, pcfx, wfxSize);

  // 修改采样率和每秒字节数
  localWfx->nSamplesPerSec =
      static_cast<DWORD>(static_cast<double>(pcfx->nSamplesPerSec) * SPEEDUP);
  localWfx->nAvgBytesPerSec = localWfx->nSamplesPerSec * localWfx->nBlockAlign;

  // 限制频率在 DirectSound 支持的范围内
  localWfx->nSamplesPerSec = max(DSBFREQUENCY_MIN, localWfx->nSamplesPerSec);
  localWfx->nSamplesPerSec = min(DSBFREQUENCY_MAX, localWfx->nSamplesPerSec);
  localWfx->nAvgBytesPerSec = localWfx->nSamplesPerSec * localWfx->nBlockAlign;

  // 调用原始的 SetFormat 函数
  HRESULT hr = it->second.SetFormat_orig(pThis, localWfx);

  _freea(localWfx);
  return hr;
}

// 伪造的 IDirectSoundBuffer::SetFrequency
HRESULT WINAPI SetFrequency_fake(IDirectSoundBuffer *pThis, DWORD dwFrequency) {
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    return DSERR_GENERIC;
  }

  DWORD baseFrequency = dwFrequency;
  // 如果程序想恢复到原始频率，我们使用记录的原始频率作为基准
  if (dwFrequency == DSBFREQUENCY_ORIGINAL) {
    baseFrequency = it->second.original_format.nSamplesPerSec;
  }

  DWORD newFrequency =
      static_cast<DWORD>(static_cast<double>(baseFrequency) * SPEEDUP);

  newFrequency = max(DSBFREQUENCY_MIN, newFrequency);
  newFrequency = min(DSBFREQUENCY_MAX, newFrequency);

  HRESULT hr = it->second.SetFrequency_orig(pThis, newFrequency);
  return hr;
}

// 伪造的 IDirectSound::Release
ULONG WINAPI DS_Release_fake(IDirectSound *pThis) {
  auto it = g_dsound_hooks.find(pThis);
  if (it == g_dsound_hooks.end()) {
    return 1;
  }

  ULONG refCount = it->second.Release_orig(pThis);
  if (refCount == 0) {
    g_dsound_hooks.erase(it);
  }
  return refCount;
}

// [REWRITTEN] 伪造的 IDirectSound::CreateSoundBuffer - 这是核心修改
HRESULT WINAPI CreateSoundBuffer_fake(IDirectSound *pThis,
                                      LPCDSBUFFERDESC pcDSBufferDesc,
                                      LPDIRECTSOUNDBUFFER *ppDSBuffer,
                                      LPUNKNOWN pUnkOuter) {
  auto it = g_dsound_hooks.find(pThis);
  if (it == g_dsound_hooks.end()) {
    return DSERR_GENERIC;
  }
  auto CreateSoundBuffer_orig = it->second.CreateSoundBuffer_orig;

  if (pcDSBufferDesc == nullptr || pcDSBufferDesc->dwSize == 0 ||
      ppDSBuffer == nullptr) {
    return DSERR_INVALIDPARAM;
  }

  // Primary Buffer 不需要我们处理，直接调用原始函数
  if (pcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER) {
    return CreateSoundBuffer_orig(pThis, pcDSBufferDesc, ppDSBuffer, pUnkOuter);
  }

  // --- 核心修改：创建修改后的描述符 ---
  DSBUFFERDESC localDesc;
  memcpy(&localDesc, pcDSBufferDesc, pcDSBufferDesc->dwSize);

  WAVEFORMATEX original_format = {0};
  WAVEFORMATEX *modified_format = nullptr;

  if (localDesc.lpwfxFormat) {
    // 复制原始格式，以便后续保存
    size_t wfxSize = sizeof(WAVEFORMATEX) + localDesc.lpwfxFormat->cbSize;
    memcpy(&original_format, localDesc.lpwfxFormat, wfxSize);

    // 在栈上分配内存来存放修改后的格式
    modified_format = (WAVEFORMATEX *)_malloca(wfxSize);
    if (!modified_format)
      return DSERR_OUTOFMEMORY;
    memcpy(modified_format, localDesc.lpwfxFormat, wfxSize);

    // 修改采样率和每秒字节数
    modified_format->nSamplesPerSec = static_cast<DWORD>(
        static_cast<double>(modified_format->nSamplesPerSec) * SPEEDUP);
    modified_format->nAvgBytesPerSec =
        modified_format->nSamplesPerSec * modified_format->nBlockAlign;

    // 限制频率范围
    modified_format->nSamplesPerSec =
        max(DSBFREQUENCY_MIN, modified_format->nSamplesPerSec);
    modified_format->nSamplesPerSec =
        min(DSBFREQUENCY_MAX, modified_format->nSamplesPerSec);
    modified_format->nAvgBytesPerSec =
        modified_format->nSamplesPerSec * modified_format->nBlockAlign;

    // 让我们的本地描述符指向修改后的格式
    localDesc.lpwfxFormat = modified_format;
  }

  // 强制使用软件混音，这是实现变速变调的基础
  localDesc.dwFlags |= DSBCAPS_CTRLFREQUENCY;
  localDesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
  localDesc.dwFlags |= DSBCAPS_LOCSOFTWARE;
  localDesc.dwFlags &= ~DSBCAPS_STATIC;

  // 使用修改后的描述符调用原始函数
  HRESULT hr = CreateSoundBuffer_orig(pThis, &localDesc, ppDSBuffer, pUnkOuter);

  // 释放栈上分配的内存
  if (modified_format) {
    _freea(modified_format);
  }

  // 如果创建成功，Hook 新的 Buffer
  if (SUCCEEDED(hr) && ppDSBuffer && *ppDSBuffer) {
    IDirectSoundBuffer *pBuffer = *ppDSBuffer;

    if (g_dsound_buffer_hooks.count(pBuffer) == 0) {
      void **pVTable = *reinterpret_cast<void ***>(pBuffer);
      // IDirectSoundBuffer8 有 24 个方法，取一个足够大的值
      const int vtable_size = 24;
      DSoundBufferHook hook_data;
      hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);

      // 保存原始函数指针
      hook_data.Release_orig =
          reinterpret_cast<decltype(hook_data.Release_orig)>(
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

      // 保存原始格式
      if (pcDSBufferDesc->lpwfxFormat) {
        size_t wfxSize =
            sizeof(WAVEFORMATEX) + pcDSBufferDesc->lpwfxFormat->cbSize;
        memcpy(&hook_data.original_format, pcDSBufferDesc->lpwfxFormat,
               wfxSize);
      } else {
        // 如果创建时没有提供格式，我们尝试获取一下（虽然不太可能）
        hook_data.GetFormat_orig(pBuffer, &hook_data.original_format,
                                 sizeof(WAVEFORMATEX), nullptr);
        // 并把它降速回来作为“原始”格式
        hook_data.original_format.nSamplesPerSec = static_cast<DWORD>(
            static_cast<double>(hook_data.original_format.nSamplesPerSec) /
            SPEEDUP);
        hook_data.original_format.nAvgBytesPerSec =
            hook_data.original_format.nSamplesPerSec *
            hook_data.original_format.nBlockAlign;
      }

      // 替换 VTable 中的函数指针
      hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DSB_Release_fake;
      hook_data.vtable_clone[IDIRECTSOUNDBUFFER_GETFORMAT_INDEX] =
          &GetFormat_fake;
      hook_data.vtable_clone[IDIRECTSOUNDBUFFER_GETFREQUENCY_INDEX] =
          &GetFrequency_fake;
      hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFORMAT_INDEX] =
          &SetFormat_fake;
      hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX] =
          &SetFrequency_fake;

      // 存储 Hook 数据并替换 VTable
      g_dsound_buffer_hooks[pBuffer] = std::move(hook_data);
      *reinterpret_cast<void ***>(pBuffer) =
          g_dsound_buffer_hooks[pBuffer].vtable_clone.data();
    }
  }
  return hr;
}

// --- 核心 Hook 函数 ---

void HookDirectSound(IUnknown *pDsound) {
  IDirectSound *pDS = static_cast<IDirectSound *>(pDsound);

  if (g_dsound_hooks.count(pDS)) {
    return;
  }

  void **pVTable = *reinterpret_cast<void ***>(pDS);
  const int vtable_size = 12; // 对于 IDirectSound8 足够
  DSoundHook hook_data;
  hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);

  hook_data.Release_orig = reinterpret_cast<decltype(hook_data.Release_orig)>(
      pVTable[IUNKNOWN_RELEASE_INDEX]);
  hook_data.CreateSoundBuffer_orig =
      reinterpret_cast<decltype(hook_data.CreateSoundBuffer_orig)>(
          pVTable[IDIRECTSOUND_CREATESOUNDBUFFER_INDEX]);

  hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DS_Release_fake;
  hook_data.vtable_clone[IDIRECTSOUND_CREATESOUNDBUFFER_INDEX] =
      &CreateSoundBuffer_fake;

  g_dsound_hooks[pDS] = std::move(hook_data);
  *reinterpret_cast<void ***>(pDS) = g_dsound_hooks[pDS].vtable_clone.data();
}

#define DIRECTSOUNDCREATE
FAKE(HRESULT, WINAPI, DirectSoundCreate, LPCGUID lpcGuidDevice,
     LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter) {
  HRESULT hr = DirectSoundCreate_real(lpcGuidDevice, ppDS, pUnkOuter);
  if (SUCCEEDED(hr) && ppDS && *ppDS) {
    HookDirectSound(*ppDS);
  }
  return hr;
}

#define DIRECTSOUNDCREATE8
FAKE(HRESULT, WINAPI, DirectSoundCreate8, LPCGUID lpcGuidDevice,
     LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter) {
  HRESULT hr = DirectSoundCreate8_real(lpcGuidDevice, ppDS8, pUnkOuter);
  if (SUCCEEDED(hr) && ppDS8 && *ppDS8) {
    HookDirectSound(*ppDS8);
  }
  return hr;
}

#define DIRECTSOUNDFULLDUPLEXCREATE
FAKE(HRESULT, WINAPI, DirectSoundFullDuplexCreate, LPCGUID pcGuidCaptureDevice,
     LPCGUID pcGuidRenderDevice, LPCDSCBUFFERDESC pcDscBufferDesc,
     LPCDSBUFFERDESC pcDsBufferDesc, HWND hwnd, DWORD dwLevel,
     LPDIRECTSOUNDFULLDUPLEX *ppDSFD, LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCB8,
     LPDIRECTSOUNDBUFFER8 *ppDSB8, LPUNKNOWN pUnkOuter) {

  // --- 与 CreateSoundBuffer_fake 中完全相同的逻辑 ---
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
      modified_format->nAvgBytesPerSec =
          modified_format->nSamplesPerSec * modified_format->nBlockAlign;
      modified_format->nSamplesPerSec =
          max(DSBFREQUENCY_MIN, modified_format->nSamplesPerSec);
      modified_format->nSamplesPerSec =
          min(DSBFREQUENCY_MAX, modified_format->nSamplesPerSec);
      modified_format->nAvgBytesPerSec =
          modified_format->nSamplesPerSec * modified_format->nBlockAlign;

      localDesc.lpwfxFormat = modified_format;
    }

    localDesc.dwFlags |= DSBCAPS_CTRLFREQUENCY;
    localDesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
    localDesc.dwFlags |= DSBCAPS_LOCSOFTWARE;
    localDesc.dwFlags &= ~DSBCAPS_STATIC;

    pModifiedDsBufferDesc = &localDesc;
  }

  HRESULT hr = DirectSoundFullDuplexCreate_real(
      pcGuidCaptureDevice, pcGuidRenderDevice, pcDscBufferDesc,
      pModifiedDsBufferDesc, hwnd, dwLevel, ppDSFD, ppDSCB8, ppDSB8, pUnkOuter);

  if (modified_format) {
    _freea(modified_format);
  }

  // 对这里创建的缓冲区应用与 CreateSoundBuffer_fake 中完全相同的 Hook 逻辑
  if (SUCCEEDED(hr) && ppDSB8 && *ppDSB8) {
    IDirectSoundBuffer *pBuffer = *ppDSB8;

    if (g_dsound_buffer_hooks.count(pBuffer) == 0) {
      void **pVTable = *reinterpret_cast<void ***>(pBuffer);
      const int vtable_size = 24;
      DSoundBufferHook hook_data;
      hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);

      hook_data.Release_orig =
          reinterpret_cast<decltype(hook_data.Release_orig)>(
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

      if (pcDsBufferDesc && pcDsBufferDesc->lpwfxFormat) {
        size_t wfxSize =
            sizeof(WAVEFORMATEX) + pcDsBufferDesc->lpwfxFormat->cbSize;
        memcpy(&hook_data.original_format, pcDsBufferDesc->lpwfxFormat,
               wfxSize);
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
  }

  return hr;
}

#endif // _EMPTY_H