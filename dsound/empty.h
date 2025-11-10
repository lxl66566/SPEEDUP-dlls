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
    return 1;
  }

  ULONG refCount = it->second.Release_orig(pThis);
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
  if (dwFrequency != 0 && dwFrequency != DSBFREQUENCY_ORIGINAL) {
    newFrequency =
        static_cast<DWORD>(static_cast<double>(dwFrequency) * SPEEDUP);
    if (newFrequency < DSBFREQUENCY_MIN)
      newFrequency = DSBFREQUENCY_MIN;
    if (newFrequency > DSBFREQUENCY_MAX)
      newFrequency = DSBFREQUENCY_MAX;
  }

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

  // --- 改进点：在创建时就修改频率 ---
  DSBUFFERDESC modifiedDesc;
  WAVEFORMATEX modifiedWfx;
  LPCDSBUFFERDESC pDescToUse = pcDSBufferDesc;

  // 我们需要修改创建参数，从源头“欺骗”DirectSound
  if (pcDSBufferDesc && pcDSBufferDesc->lpwfxFormat) {
    // 复制原始结构体，因为它们是 const
    memcpy(&modifiedDesc, pcDSBufferDesc, sizeof(DSBUFFERDESC));
    memcpy(&modifiedWfx, pcDSBufferDesc->lpwfxFormat, sizeof(WAVEFORMATEX));

    // 对采样率应用加速倍率
    modifiedWfx.nSamplesPerSec = static_cast<DWORD>(
        static_cast<double>(modifiedWfx.nSamplesPerSec) * SPEEDUP);

    // 关键：必须同时更新 nAvgBytesPerSec 以匹配新的采样率
    modifiedWfx.nAvgBytesPerSec =
        modifiedWfx.nSamplesPerSec * modifiedWfx.nBlockAlign;

    // 让修改后的描述符指向修改后的格式结构体
    modifiedDesc.lpwfxFormat = &modifiedWfx;

    // 使用我们修改后的描述符来调用原始函数
    pDescToUse = &modifiedDesc;
  }
  // --- 改进结束 ---

  // 使用可能被修改过的描述符调用原始的 CreateSoundBuffer
  HRESULT hr = it->second.CreateSoundBuffer_orig(pThis, pDescToUse, ppDSBuffer,
                                                 pUnkOuter);

  // 如果缓冲区创建成功，我们就 Hook 它
  if (SUCCEEDED(hr) && ppDSBuffer && *ppDSBuffer) {
    IDirectSoundBuffer *pBuffer = *ppDSBuffer;

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

// --- 核心 Hook 函数 ---

void HookDirectSound(IUnknown *pDsound) {
  IDirectSound *pDS = static_cast<IDirectSound *>(pDsound);

  if (g_dsound_hooks.count(pDS)) {
    return;
  }

  void **pVTable = *reinterpret_cast<void ***>(pDS);
  const int vtable_size = 12;
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
}

// --- 导出的伪造函数实现 ---

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
  // --- 改进点：同样在此处修改频率 ---
  DSBUFFERDESC modifiedDsDesc;
  WAVEFORMATEX modifiedDsWfx;
  LPCDSBUFFERDESC pDsDescToUse = pcDsBufferDesc;

  if (pcDsBufferDesc && pcDsBufferDesc->lpwfxFormat) {
    memcpy(&modifiedDsDesc, pcDsBufferDesc, sizeof(DSBUFFERDESC));
    memcpy(&modifiedDsWfx, pcDsBufferDesc->lpwfxFormat, sizeof(WAVEFORMATEX));

    modifiedDsWfx.nSamplesPerSec = static_cast<DWORD>(
        static_cast<double>(modifiedDsWfx.nSamplesPerSec) * SPEEDUP);
    modifiedDsWfx.nAvgBytesPerSec =
        modifiedDsWfx.nSamplesPerSec * modifiedDsWfx.nBlockAlign;

    modifiedDsDesc.lpwfxFormat = &modifiedDsWfx;
    pDsDescToUse = &modifiedDsDesc;
  }
  // --- 改进结束 ---

  HRESULT hr = DirectSoundFullDuplexCreate_real(
      pcGuidCaptureDevice, pcGuidRenderDevice, pcDscBufferDesc,
      pDsDescToUse, // 使用修改后的描述符
      hwnd, dwLevel, ppDSFD, ppDSCB8, ppDSB8, pUnkOuter);

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