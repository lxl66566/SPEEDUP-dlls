#include <windows.h>

#include "hook_macro.h"
#include <dsound.h>
#include <map>
#include <stdio.h>

// 定义我们需要的 COM 接口方法的函数指针类型
typedef HRESULT(STDMETHODCALLTYPE *CreateSoundBuffer_ptr)(IDirectSound8 *,
                                                          const DSBUFFERDESC *,
                                                          LPDIRECTSOUNDBUFFER *,
                                                          LPUNKNOWN);
typedef HRESULT(STDMETHODCALLTYPE *SetFrequency_ptr)(IDirectSoundBuffer *,
                                                     DWORD);

// 全局变量，用于存储原始的 CreateSoundBuffer 函数指针
CreateSoundBuffer_ptr g_originalCreateSoundBuffer = nullptr;

// 使用 map 来存储每个 IDirectSoundBuffer 实例对应的原始 SetFrequency 函数指针
std::map<IDirectSoundBuffer *, SetFrequency_ptr> g_originalSetFrequencyMap;

// 我们自己的 SetFrequency 实现
HRESULT STDMETHODCALLTYPE SetFrequency_fake(IDirectSoundBuffer *pThis,
                                            DWORD dwFrequency) {
  // 从 map 中查找原始的 SetFrequency 函数
  auto it = g_originalSetFrequencyMap.find(pThis);
  if (it == g_originalSetFrequencyMap.end()) {
    // 如果找不到，这是一个错误，直接返回
    return E_FAIL;
  }
  SetFrequency_ptr original_SetFrequency = it->second;

  DWORD newFrequency;

  if (dwFrequency == DSBFREQUENCY_ORIGINAL) {
    // 如果游戏请求恢复原始频率，我们需要获取缓冲区的原始格式
    WAVEFORMATEX wfx;
    ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
    // 注意：GetFormat 可能需要一个更大的结构体，但我们只需要 nSamplesPerSec
    if (SUCCEEDED(pThis->GetFormat(&wfx, sizeof(WAVEFORMATEX), NULL))) {
      // 将原始采样率乘以 SPEEDUP 作为新的频率
      newFrequency =
          static_cast<DWORD>(static_cast<float>(wfx.nSamplesPerSec) * SPEEDUP);
    } else {
      // 如果获取格式失败，则无法计算新频率，只能传递0
      newFrequency = DSBFREQUENCY_ORIGINAL;
    }
  } else {
    // 对于所有其他频率设置，直接乘以 SPEEDUP
    newFrequency =
        static_cast<DWORD>(static_cast<float>(dwFrequency) * SPEEDUP);
  }

  // 确保频率在 DirectSound 支持的范围内
  if (newFrequency < DSBFREQUENCY_MIN)
    newFrequency = DSBFREQUENCY_MIN;
  if (newFrequency > DSBFREQUENCY_MAX)
    newFrequency = DSBFREQUENCY_MAX;

  // 调用原始的 SetFrequency 函数
  return original_SetFrequency(pThis, newFrequency);
}

// 我们自己的 CreateSoundBuffer 实现
HRESULT STDMETHODCALLTYPE
CreateSoundBuffer_fake(IDirectSound8 *pThis, const DSBUFFERDESC *pcDSBufferDesc,
                       LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter) {
  // 首先，调用原始的 CreateSoundBuffer 函数来创建缓冲区
  HRESULT hr =
      g_originalCreateSoundBuffer(pThis, pcDSBufferDesc, ppDSBuffer, pUnkOuter);

  if (SUCCEEDED(hr) && ppDSBuffer && *ppDSBuffer) {
    IDirectSoundBuffer *pDSBuffer = *ppDSBuffer;

    // 对新创建的 IDirectSoundBuffer 对象进行 VTable Hook，替换 SetFrequency
    // 方法
    void **vtable = *(void ***)pDSBuffer;

    // IUnknown 接口有 3 个方法 (QueryInterface, AddRef, Release)
    // IDirectSoundBuffer 接口中 SetFrequency 是第 15 个方法
    // 所以在 VTable 中的索引是 3 + 14 = 17
    int setFrequencyVTableIndex = 17;

    // 保存原始的 SetFrequency 函数指针
    SetFrequency_ptr original_SetFrequency =
        (SetFrequency_ptr)vtable[setFrequencyVTableIndex];
    g_originalSetFrequencyMap[pDSBuffer] = original_SetFrequency;

    // 替换 VTable 中的指针为我们自己的函数
    DWORD oldProtect;
    VirtualProtect(&vtable[setFrequencyVTableIndex], sizeof(void *),
                   PAGE_READWRITE, &oldProtect);
    vtable[setFrequencyVTableIndex] = &SetFrequency_fake;
    VirtualProtect(&vtable[setFrequencyVTableIndex], sizeof(void *), oldProtect,
                   &oldProtect);

    // 为了应用初始速度，我们立即调用一次我们自己的 SetFrequency_fake
    // 这会使得缓冲区在第一次播放时就以 SPEEDUP 倍速播放
    if (pcDSBufferDesc->dwFlags & DSBCAPS_CTRLFREQUENCY) {
      pDSBuffer->SetFrequency(DSBFREQUENCY_ORIGINAL);
    }
  }

  return hr;
}

// 使用 wrap_dll 的 FAKE 宏来定义 DirectSoundCreate8 的钩子函数
// 我们只 hook DirectSoundCreate8，因为这是现代游戏最常用的
#define DIRECTSOUNDCREATE8
FAKE(HRESULT, WINAPI, DirectSoundCreate8, const GUID *lpGUID,
     LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter) {
  // 调用原始的 DirectSoundCreate8
  HRESULT hr = DirectSoundCreate8_real(lpGUID, ppDS8, pUnkOuter);

  if (SUCCEEDED(hr) && ppDS8 && *ppDS8) {
    IDirectSound8 *pDS8 = *ppDS8;

    // 对获取到的 IDirectSound8 对象进行 VTable Hook，替换 CreateSoundBuffer
    // 方法
    void **vtable = *(void ***)pDS8;

    // IUnknown 接口有 3 个方法 (QueryInterface, AddRef, Release)
    // IDirectSound8 接口中 CreateSoundBuffer 是第 1 个方法
    // 所以在 VTable 中的索引是 3
    int createSoundBufferVTableIndex = 3;

    // 保存原始的 CreateSoundBuffer 函数指针
    g_originalCreateSoundBuffer =
        (CreateSoundBuffer_ptr)vtable[createSoundBufferVTableIndex];

    // 替换 VTable 中的指针为我们自己的函数
    DWORD oldProtect;
    VirtualProtect(&vtable[createSoundBufferVTableIndex], sizeof(void *),
                   PAGE_READWRITE, &oldProtect);
    vtable[createSoundBufferVTableIndex] = &CreateSoundBuffer_fake;
    VirtualProtect(&vtable[createSoundBufferVTableIndex], sizeof(void *),
                   oldProtect, &oldProtect);
  }

  return hr;
}

// --- 结束你的修改 ---

HINSTANCE mHinst = 0, mHinstDLL = 0;

extern "C" UINT_PTR mProcs[12] = {0};

LPCSTR mImportNames[] = {
    "DirectSoundCaptureCreate",
    "DirectSoundCaptureCreate8",
    "DirectSoundCaptureEnumerateA",
    "DirectSoundCaptureEnumerateW",
    "DirectSoundCreate",
    "DirectSoundCreate8",
    "DirectSoundEnumerateA",
    "DirectSoundEnumerateW",
    "DirectSoundFullDuplexCreate",
    "DllCanUnloadNow",
    "DllGetClassObject",
    "GetDeviceID",
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

// 原来的 empty.h 内容被上面的代码替换了

inline void _hook_setup() {
#ifdef DIRECTSOUNDCAPTURECREATE
  DirectSoundCaptureCreate_real = (DirectSoundCaptureCreate_ptr)mProcs[0];
  mProcs[0] = (UINT_PTR)&DirectSoundCaptureCreate_fake;
#endif
#ifdef DIRECTSOUNDCAPTURECREATE8
  DirectSoundCaptureCreate8_real = (DirectSoundCaptureCreate8_ptr)mProcs[1];
  mProcs[1] = (UINT_PTR)&DirectSoundCaptureCreate8_fake;
#endif
#ifdef DIRECTSOUNDCAPTUREENUMERATEA
  DirectSoundCaptureEnumerateA_real =
      (DirectSoundCaptureEnumerateA_ptr)mProcs[2];
  mProcs[2] = (UINT_PTR)&DirectSoundCaptureEnumerateA_fake;
#endif
#ifdef DIRECTSOUNDCAPTUREENUMERATEW
  DirectSoundCaptureEnumerateW_real =
      (DirectSoundCaptureEnumerateW_ptr)mProcs[3];
  mProcs[3] = (UINT_PTR)&DirectSoundCaptureEnumerateW_fake;
#endif
#ifdef DIRECTSOUNDCREATE
  DirectSoundCreate_real = (DirectSoundCreate_ptr)mProcs[4];
  mProcs[4] = (UINT_PTR)&DirectSoundCreate_fake;
#endif
#ifdef DIRECTSOUNDCREATE8
  // 【修正】根据你提供的原始代码，DirectSoundCreate8 的索引应该是 5
  DirectSoundCreate8_real = (DirectSoundCreate8_ptr)mProcs[5];
  mProcs[5] = (UINT_PTR)&DirectSoundCreate8_fake;
#endif
#ifdef DIRECTSOUNDENUMERATEA
  DirectSoundEnumerateA_real = (DirectSoundEnumerateA_ptr)mProcs[6];
  mProcs[6] = (UINT_PTR)&DirectSoundEnumerateA_fake;
#endif
#ifdef DIRECTSOUNDENUMERATEW
  DirectSoundEnumerateW_real = (DirectSoundEnumerateW_ptr)mProcs[7];
  mProcs[7] = (UINT_PTR)&DirectSoundEnumerateW_fake;
#endif
#ifdef DIRECTSOUNDFULLDUPLEXCREATE
  DirectSoundFullDuplexCreate_real = (DirectSoundFullDuplexCreate_ptr)mProcs[8];
  mProcs[8] = (UINT_PTR)&DirectSoundFullDuplexCreate_fake;
#endif
#ifdef DLLCANUNLOADNOW
  DllCanUnloadNow_real = (DllCanUnloadNow_ptr)mProcs[9];
  mProcs[9] = (UINT_PTR)&DllCanUnloadNow_fake;
#endif
#ifdef DLLGETCLASSOBJECT
  DllGetClassObject_real = (DllGetClassObject_ptr)mProcs[10];
  mProcs[10] = (UINT_PTR)&DllGetClassObject_fake;
#endif
#ifdef GETDEVICEID
  GetDeviceID_real = (GetDeviceID_ptr)mProcs[11];
  mProcs[11] = (UINT_PTR)&GetDeviceID_fake;
#endif
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  mHinst = hinstDLL;
  if (fdwReason == DLL_PROCESS_ATTACH) {
    // 使用系统路径加载原始 DLL，这样更健壮，无需手动重命名和复制文件
    char syspath[MAX_PATH];
    GetSystemDirectoryA(syspath, MAX_PATH);
    strcat_s(syspath, "\\dsound.dll");
    mHinstDLL = LoadLibraryA(syspath);

    if (!mHinstDLL) {
      MessageBoxA(NULL, "Failed to load the real dsound.dll from System32.",
                  "Proxy Error", MB_OK | MB_ICONERROR);
      return FALSE;
    }
    for (int i = 0; i < 12; ++i) {
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

extern "C" void DirectSoundCaptureCreate_wrapper();
extern "C" void DirectSoundCaptureCreate8_wrapper();
extern "C" void DirectSoundCaptureEnumerateA_wrapper();
extern "C" void DirectSoundCaptureEnumerateW_wrapper();
extern "C" void DirectSoundCreate_wrapper();
extern "C" void DirectSoundCreate8_wrapper();
extern "C" void DirectSoundEnumerateA_wrapper();
extern "C" void DirectSoundEnumerateW_wrapper();
extern "C" void DirectSoundFullDuplexCreate_wrapper();
extern "C" void DllCanUnloadNow_wrapper();
extern "C" void DllGetClassObject_wrapper();
extern "C" void GetDeviceID_wrapper();