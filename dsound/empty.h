#ifndef _EMPTY_H
#define _EMPTY_H

#include <windows.h>

#include <dsound.h>
#include <map>
#include <utility>
#include <vector>

// --- VTable 索引常量 ---
// 根据 dsound.h 和 COM 规范，IUnknown 的 Release 方法在 VTable 中的索引是 2
constexpr int IUNKNOWN_RELEASE_INDEX = 2;
// IDirectSound 的 CreateSoundBuffer 方法在 VTable 中的索引是 10
constexpr int IDIRECTSOUND_CREATESOUNDBUFFER_INDEX = 10;
// IDirectSoundBuffer 的 SetFrequency 方法在 VTable 中的索引是 17
constexpr int IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX = 17;

// --- 前向声明伪造函数 ---
HRESULT WINAPI CreateSoundBuffer_fake(IDirectSound *pThis,
                                      LPCDSBUFFERDESC pcDSBufferDesc,
                                      LPDIRECTSOUNDBUFFER *ppDSBuffer,
                                      LPUNKNOWN pUnkOuter);
ULONG WINAPI DS_Release_fake(IDirectSound *pThis);
HRESULT WINAPI SetFrequency_fake(IDirectSoundBuffer *pThis, DWORD dwFrequency);
ULONG WINAPI DSB_Release_fake(IDirectSoundBuffer *pThis);

// --- 用于存储 Hook 信息的结构体和全局 Map ---

// IDirectSound 接口的 Hook 数据
struct DSoundHook {
  // 指向原始 VTable 函数的指针
  HRESULT(WINAPI *CreateSoundBuffer_orig)(IDirectSound *, LPCDSBUFFERDESC,
                                          LPDIRECTSOUNDBUFFER *, LPUNKNOWN);
  ULONG(WINAPI *Release_orig)(IDirectSound *);
  // 我们自己复制并修改过的 VTable
  std::vector<void *> vtable_clone;
};
std::map<IDirectSound *, DSoundHook> g_dsound_hooks;

// IDirectSoundBuffer 接口的 Hook 数据
struct DSoundBufferHook {
  // 指向原始 VTable 函数的指针
  HRESULT(WINAPI *SetFrequency_orig)(IDirectSoundBuffer *, DWORD);
  ULONG(WINAPI *Release_orig)(IDirectSoundBuffer *);
  // 我们自己复制并修改过的 VTable
  std::vector<void *> vtable_clone;
};
std::map<IDirectSoundBuffer *, DSoundBufferHook> g_dsound_buffer_hooks;

// --- 伪造的 VTable 方法实现 ---

// 伪造的 IDirectSoundBuffer::Release
ULONG WINAPI DSB_Release_fake(IDirectSoundBuffer *pThis) {
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    // 如果找不到 Hook 数据，说明可能出错了，直接返回，避免崩溃
    return 1;
  }

  // 调用原始的 Release 方法
  ULONG refCount = it->second.Release_orig(pThis);

  // 如果引用计数为 0，说明对象将被销毁，我们需要清理我们的 Hook 数据
  if (refCount == 0) {
    g_dsound_buffer_hooks.erase(it);
  }
  return refCount;
}

// 伪造的 IDirectSoundBuffer::SetFrequency
HRESULT WINAPI SetFrequency_fake(IDirectSoundBuffer *pThis, DWORD dwFrequency) {
  auto it = g_dsound_buffer_hooks.find(pThis);
  if (it == g_dsound_buffer_hooks.end()) {
    return DSERR_GENERIC;
  }

  DWORD newFrequency = dwFrequency;
  // DSBFREQUENCY_ORIGINAL (0) 是一个特殊值，我们不应该修改它
  if (dwFrequency != 0 && dwFrequency != DSBFREQUENCY_ORIGINAL) {
    newFrequency =
        static_cast<DWORD>(static_cast<double>(dwFrequency) * SPEEDUP);
    // 确保频率在 DirectSound 的有效范围内
    if (newFrequency < DSBFREQUENCY_MIN)
      newFrequency = DSBFREQUENCY_MIN;
    if (newFrequency > DSBFREQUENCY_MAX)
      newFrequency = DSBFREQUENCY_MAX;
  }

  // 使用修改后的频率调用原始的 SetFrequency 方法
  return it->second.SetFrequency_orig(pThis, newFrequency);
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

// 伪造的 IDirectSound::CreateSoundBuffer
HRESULT WINAPI CreateSoundBuffer_fake(IDirectSound *pThis,
                                      LPCDSBUFFERDESC pcDSBufferDesc,
                                      LPDIRECTSOUNDBUFFER *ppDSBuffer,
                                      LPUNKNOWN pUnkOuter) {
  auto it = g_dsound_hooks.find(pThis);
  if (it == g_dsound_hooks.end()) {
    return DSERR_GENERIC;
  }

  // 调用原始的 CreateSoundBuffer 来获取真实的缓冲区对象
  HRESULT hr = it->second.CreateSoundBuffer_orig(pThis, pcDSBufferDesc,
                                                 ppDSBuffer, pUnkOuter);

  // 如果缓冲区创建成功，我们就 Hook 它
  if (SUCCEEDED(hr) && ppDSBuffer && *ppDSBuffer) {
    IDirectSoundBuffer *pBuffer = *ppDSBuffer;

    // 防止重复 Hook
    if (g_dsound_buffer_hooks.count(pBuffer)) {
      return hr;
    }

    // 复制 VTable
    void **pVTable = *reinterpret_cast<void ***>(pBuffer);
    // IDirectSoundBuffer8 有 24 个方法，我们复制足够的大小
    const int vtable_size = 24;
    DSoundBufferHook hook_data;
    hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);

    // 保存原始函数指针
    hook_data.Release_orig = reinterpret_cast<decltype(hook_data.Release_orig)>(
        hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX]);
    hook_data.SetFrequency_orig =
        reinterpret_cast<decltype(hook_data.SetFrequency_orig)>(
            hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX]);

    // 在复制的 VTable 中替换为我们的伪造函数
    hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DSB_Release_fake;
    hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX] =
        &SetFrequency_fake;

    // 保存 Hook 数据，并将对象的 VTable 指针指向我们的副本
    g_dsound_buffer_hooks[pBuffer] = std::move(hook_data);
    *reinterpret_cast<void ***>(pBuffer) =
        g_dsound_buffer_hooks[pBuffer].vtable_clone.data();
  }
  return hr;
}

// --- 核心 Hook 函数 ---

// 对 IDirectSound 或 IDirectSound8 接口进行 Hook
void HookDirectSound(IUnknown *pDsound) {
  IDirectSound *pDS = static_cast<IDirectSound *>(pDsound);

  if (g_dsound_hooks.count(pDS)) {
    return; // 已经 Hook 过了
  }

  // 复制 VTable
  void **pVTable = *reinterpret_cast<void ***>(pDS);
  // IDirectSound8 有 12 个方法
  const int vtable_size = 12;
  DSoundHook hook_data;
  hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);

  // 保存原始函数指针
  hook_data.Release_orig = reinterpret_cast<decltype(hook_data.Release_orig)>(
      hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX]);
  hook_data.CreateSoundBuffer_orig =
      reinterpret_cast<decltype(hook_data.CreateSoundBuffer_orig)>(
          hook_data.vtable_clone[IDIRECTSOUND_CREATESOUNDBUFFER_INDEX]);

  // 替换为伪造函数
  hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DS_Release_fake;
  hook_data.vtable_clone[IDIRECTSOUND_CREATESOUNDBUFFER_INDEX] =
      &CreateSoundBuffer_fake;

  // 保存 Hook 数据并替换 VTable 指针
  g_dsound_hooks[pDS] = std::move(hook_data);
  *reinterpret_cast<void ***>(pDS) = g_dsound_hooks[pDS].vtable_clone.data();
}

// --- 导出的伪造函数实现 ---

#define DIRECTSOUNDCREATE
FAKE(HRESULT, WINAPI, DirectSoundCreate, LPCGUID lpcGuidDevice,
     LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter) {
  // 调用原始的 DirectSoundCreate
  HRESULT hr = DirectSoundCreate_real(lpcGuidDevice, ppDS, pUnkOuter);
  // 如果成功，Hook 返回的 IDirectSound 对象
  if (SUCCEEDED(hr) && ppDS && *ppDS) {
    HookDirectSound(*ppDS);
  }
  return hr;
}

#define DIRECTSOUNDCREATE8
FAKE(HRESULT, WINAPI, DirectSoundCreate8, LPCGUID lpcGuidDevice,
     LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter) {
  // 调用原始的 DirectSoundCreate8
  HRESULT hr = DirectSoundCreate8_real(lpcGuidDevice, ppDS8, pUnkOuter);
  // 如果成功，Hook 返回的 IDirectSound8 对象
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
  // 调用原始函数
  HRESULT hr = DirectSoundFullDuplexCreate_real(
      pcGuidCaptureDevice, pcGuidRenderDevice, pcDscBufferDesc, pcDsBufferDesc,
      hwnd, dwLevel, ppDSFD, ppDSCB8, ppDSB8, pUnkOuter);

  // 此函数直接创建了一个缓冲区，我们需要直接 Hook 它
  if (SUCCEEDED(hr) && ppDSB8 && *ppDSB8) {
    IDirectSoundBuffer *pBuffer = *ppDSB8;

    if (g_dsound_buffer_hooks.count(pBuffer)) {
      return hr;
    }

    void **pVTable = *reinterpret_cast<void ***>(pBuffer);
    const int vtable_size = 24;
    DSoundBufferHook hook_data;
    hook_data.vtable_clone.assign(pVTable, pVTable + vtable_size);

    hook_data.Release_orig = reinterpret_cast<decltype(hook_data.Release_orig)>(
        hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX]);
    hook_data.SetFrequency_orig =
        reinterpret_cast<decltype(hook_data.SetFrequency_orig)>(
            hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX]);

    hook_data.vtable_clone[IUNKNOWN_RELEASE_INDEX] = &DSB_Release_fake;
    hook_data.vtable_clone[IDIRECTSOUNDBUFFER_SETFREQUENCY_INDEX] =
        &SetFrequency_fake;

    g_dsound_buffer_hooks[pBuffer] = std::move(hook_data);
    *reinterpret_cast<void ***>(pBuffer) =
        g_dsound_buffer_hooks[pBuffer].vtable_clone.data();
  }

  return hr;
}

#endif // _EMPTY_H