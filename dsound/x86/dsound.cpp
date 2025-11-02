#include <windows.h>

#include "hook_macro.h"
#include <algorithm>
#include <dsound.h>
#include <map>
#include <memory>
#include <stdio.h>
#include <vector>

#include <soundtouch/SoundTouch.h>

// --- 函数指针类型定义 ---
// IDirectSound8
typedef HRESULT(STDMETHODCALLTYPE *CreateSoundBuffer_ptr)(IDirectSound8 *,
                                                          const DSBUFFERDESC *,
                                                          LPDIRECTSOUNDBUFFER *,
                                                          LPUNKNOWN);

// IDirectSoundBuffer
typedef HRESULT(STDMETHODCALLTYPE *Lock_ptr)(IDirectSoundBuffer *, DWORD, DWORD,
                                             LPVOID *, LPDWORD, LPVOID *,
                                             LPDWORD, DWORD);
typedef HRESULT(STDMETHODCALLTYPE *Unlock_ptr)(IDirectSoundBuffer *, LPVOID,
                                               DWORD, LPVOID, DWORD);
typedef HRESULT(STDMETHODCALLTYPE *Play_ptr)(IDirectSoundBuffer *, DWORD, DWORD,
                                             DWORD);
typedef HRESULT(STDMETHODCALLTYPE *Stop_ptr)(IDirectSoundBuffer *);
typedef HRESULT(STDMETHODCALLTYPE *SetCurrentPosition_ptr)(IDirectSoundBuffer *,
                                                           DWORD);
typedef HRESULT(STDMETHODCALLTYPE *GetCurrentPosition_ptr)(IDirectSoundBuffer *,
                                                           LPDWORD, LPDWORD);
typedef ULONG(STDMETHODCALLTYPE *Release_ptr)(IDirectSoundBuffer *);

// --- 核心封装类 ---
// 用于管理每个被 hook 的缓冲区的状态、影子缓冲区和 SoundTouch 实例
class BufferWrapper {
public:
  // 原始 VTable 函数指针
  Lock_ptr originalLock = nullptr;
  Unlock_ptr originalUnlock = nullptr;
  Play_ptr originalPlay = nullptr;
  Stop_ptr originalStop = nullptr;
  SetCurrentPosition_ptr originalSetCurrentPosition = nullptr;
  GetCurrentPosition_ptr originalGetCurrentPosition = nullptr;
  Release_ptr originalRelease = nullptr;

  IDirectSoundBuffer *realBuffer = nullptr;
  WAVEFORMATEX waveFormat = {};
  DWORD realBufferSize = 0;

  std::vector<char> shadowBuffer; // 影子缓冲区，大小是真实缓冲区的 SPEEDUP 倍
  DWORD shadowWriteCursor = 0;    // 应用程序在影子缓冲区的写入位置

  soundtouch::SoundTouch soundTouch;
  std::vector<float> floatBuffer; // SoundTouch 处理用的浮点缓冲区

  DWORD realWriteCursor = 0; // 我们在真实缓冲区的写入位置

  BufferWrapper(IDirectSoundBuffer *buffer, const DSBUFFERDESC *desc)
      : realBuffer(buffer) {
    // 1. 保存音频格式和真实缓冲区大小
    if (desc->lpwfxFormat) {
      waveFormat = *desc->lpwfxFormat;
    }
    realBufferSize = desc->dwBufferBytes;

    // 2. 创建影子缓冲区
    DWORD shadowSize = static_cast<DWORD>(realBufferSize * SPEEDUP);
    shadowBuffer.resize(shadowSize);
    std::fill(shadowBuffer.begin(), shadowBuffer.end(),
              (waveFormat.wBitsPerSample == 8) ? 128 : 0);

    // 3. 初始化 SoundTouch
    soundTouch.setSampleRate(waveFormat.nSamplesPerSec);
    soundTouch.setChannels(waveFormat.nChannels);
    soundTouch.setTempo(SPEEDUP);

    // 预分配一些空间以提高效率
    floatBuffer.reserve(8192 * waveFormat.nChannels);
  }

  // 将 16-bit PCM 数据转换为 float
  void convertToFloat(const short *input, float *output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
      output[i] = static_cast<float>(input[i]) / 32768.0f;
    }
  }

  // 将 float 数据转换回 16-bit PCM
  void convertToShort(const float *input, short *output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
      float sample = input[i] * 32768.0f;
      // Clipping
      if (sample > 32767.0f)
        sample = 32767.0f;
      if (sample < -32768.0f)
        sample = -32768.0f;
      output[i] = static_cast<short>(sample);
    }
  }

  // 处理数据：从影子缓冲区 -> SoundTouch -> 真实缓冲区
  void processAudio(DWORD bytesToWrite) {
    // 目前仅支持 16-bit 音频，这是最常见的情况
    if (waveFormat.wBitsPerSample != 16 || waveFormat.nBlockAlign == 0) {
      return;
    }

    DWORD samplesToWrite = bytesToWrite / waveFormat.nBlockAlign;
    if (samplesToWrite == 0)
      return;

    // 准备输入数据
    const short *inputSamples = reinterpret_cast<const short *>(
        shadowBuffer.data() + shadowWriteCursor);

    // 转换并送入 SoundTouch
    floatBuffer.resize(samplesToWrite * waveFormat.nChannels);
    convertToFloat(inputSamples, floatBuffer.data(),
                   samplesToWrite * waveFormat.nChannels);
    soundTouch.putSamples(floatBuffer.data(), samplesToWrite);

    // 更新影子缓冲区的写入指针
    shadowWriteCursor =
        (shadowWriteCursor + bytesToWrite) % shadowBuffer.size();

    // 从 SoundTouch 获取处理后的数据并写入真实缓冲区
    uint numProcessedSamples;
    do {
      // 调整缓冲区大小以接收数据
      floatBuffer.resize(4096 * waveFormat.nChannels);
      numProcessedSamples = soundTouch.receiveSamples(floatBuffer.data(), 4096);

      if (numProcessedSamples > 0) {
        DWORD bytesToCopyToReal = numProcessedSamples * waveFormat.nBlockAlign;

        LPVOID ptr1 = nullptr, ptr2 = nullptr;
        DWORD size1 = 0, size2 = 0;

        // 锁定真实缓冲区
        HRESULT hr =
            originalLock(realBuffer, realWriteCursor, bytesToCopyToReal, &ptr1,
                         &size1, &ptr2, &size2, 0);
        if (SUCCEEDED(hr)) {
          std::vector<short> shortBuffer(numProcessedSamples *
                                         waveFormat.nChannels);
          convertToShort(floatBuffer.data(), shortBuffer.data(),
                         numProcessedSamples * waveFormat.nChannels);

          if (ptr1 && size1 > 0) {
            memcpy(ptr1, shortBuffer.data(), size1);
          }
          if (ptr2 && size2 > 0) {
            memcpy(ptr2, reinterpret_cast<char *>(shortBuffer.data()) + size1,
                   size2);
          }

          // 解锁真实缓冲区
          originalUnlock(realBuffer, ptr1, size1, ptr2, size2);

          // 更新真实缓冲区的写入指针
          realWriteCursor =
              (realWriteCursor + bytesToCopyToReal) % realBufferSize;
        }
      }
    } while (numProcessedSamples != 0);
  }
};

// --- 全局变量 ---
CreateSoundBuffer_ptr g_originalCreateSoundBuffer = nullptr;
// 使用 map 管理所有被 hook 的缓冲区
std::map<IDirectSoundBuffer *, std::unique_ptr<BufferWrapper>> g_bufferMap;

// --- Hooked 函数实现 ---

HRESULT STDMETHODCALLTYPE Lock_fake(IDirectSoundBuffer *pThis,
                                    DWORD dwWriteCursor, DWORD dwWriteBytes,
                                    LPVOID *ppvAudioPtr1,
                                    LPDWORD pdwAudioBytes1,
                                    LPVOID *ppvAudioPtr2,
                                    LPDWORD pdwAudioBytes2, DWORD dwFlags) {
  auto it = g_bufferMap.find(pThis);
  if (it == g_bufferMap.end())
    return E_FAIL;

  BufferWrapper *wrapper = it->second.get();

  // "欺骗"：应用程序请求锁定，我们返回影子缓冲区的指针
  if (dwFlags & DSBLOCK_FROMWRITECURSOR) {
    // 应用程序想从它认为的写入点开始写，我们用影子缓冲区的位置
    wrapper->originalGetCurrentPosition(wrapper->realBuffer, NULL,
                                        &dwWriteCursor);
    dwWriteCursor = static_cast<DWORD>(dwWriteCursor * SPEEDUP);
  }

  // 确保写入位置在影子缓冲区范围内
  dwWriteCursor %= wrapper->shadowBuffer.size();

  *ppvAudioPtr1 = wrapper->shadowBuffer.data() + dwWriteCursor;
  if (dwWriteCursor + dwWriteBytes > wrapper->shadowBuffer.size()) {
    *pdwAudioBytes1 = (DWORD)wrapper->shadowBuffer.size() - dwWriteCursor;
    *ppvAudioPtr2 = wrapper->shadowBuffer.data();
    *pdwAudioBytes2 = dwWriteBytes - *pdwAudioBytes1;
  } else {
    *pdwAudioBytes1 = dwWriteBytes;
    *ppvAudioPtr2 = nullptr;
    *pdwAudioBytes2 = 0;
  }

  // 记录下应用程序将要写入的位置，Unlock 时会用到
  wrapper->shadowWriteCursor = dwWriteCursor;

  return DS_OK;
}

HRESULT STDMETHODCALLTYPE Unlock_fake(IDirectSoundBuffer *pThis,
                                      LPVOID pvAudioPtr1, DWORD dwAudioBytes1,
                                      LPVOID pvAudioPtr2, DWORD dwAudioBytes2) {
  auto it = g_bufferMap.find(pThis);
  if (it == g_bufferMap.end())
    return E_FAIL;

  BufferWrapper *wrapper = it->second.get();

  // 数据已经由应用程序通过 Lock_fake 返回的指针写入了影子缓冲区
  // 现在我们调用 processAudio 来处理这些新写入的数据
  wrapper->processAudio(dwAudioBytes1 + dwAudioBytes2);

  return DS_OK;
}

HRESULT STDMETHODCALLTYPE
GetCurrentPosition_fake(IDirectSoundBuffer *pThis, LPDWORD pdwCurrentPlayCursor,
                        LPDWORD pdwCurrentWriteCursor) {
  auto it = g_bufferMap.find(pThis);
  if (it == g_bufferMap.end())
    return E_FAIL;

  BufferWrapper *wrapper = it->second.get();

  DWORD realPlay = 0, realWrite = 0;
  HRESULT hr = wrapper->originalGetCurrentPosition(wrapper->realBuffer,
                                                   &realPlay, &realWrite);

  if (SUCCEEDED(hr)) {
    if (pdwCurrentPlayCursor) {
      // 将真实播放位置放大，以匹配影子缓冲区的尺寸，欺骗应用程序
      *pdwCurrentPlayCursor =
          static_cast<DWORD>(realPlay * SPEEDUP) % wrapper->shadowBuffer.size();
    }
    if (pdwCurrentWriteCursor) {
      // 写入位置是应用程序在影子缓冲区的位置
      *pdwCurrentWriteCursor = static_cast<DWORD>(realWrite * SPEEDUP) %
                               wrapper->shadowBuffer.size();
    }
  }
  return hr;
}

ULONG STDMETHODCALLTYPE Release_fake(IDirectSoundBuffer *pThis) {
  auto it = g_bufferMap.find(pThis);
  if (it == g_bufferMap.end()) {
    return 1;
  }

  Release_ptr original_Release = it->second->originalRelease;

  // 调用原始 Release，获取新的引用计数
  ULONG refCount = original_Release(pThis);

  if (refCount == 0) {
    // 引用计数为0，对象将被销毁，从 map 中移除我们的包装器以释放内存
    g_bufferMap.erase(it);
  }

  return refCount;
}

HRESULT STDMETHODCALLTYPE Play_fake(IDirectSoundBuffer *pThis,
                                    DWORD dwReserved1, DWORD dwPriority,
                                    DWORD dwFlags) {
  auto it = g_bufferMap.find(pThis);
  if (it != g_bufferMap.end())
    return it->second->originalPlay(pThis, dwReserved1, dwPriority, dwFlags);
  return E_FAIL;
}

HRESULT STDMETHODCALLTYPE Stop_fake(IDirectSoundBuffer *pThis) {
  auto it = g_bufferMap.find(pThis);
  if (it != g_bufferMap.end())
    return it->second->originalStop(pThis);
  return E_FAIL;
}

HRESULT STDMETHODCALLTYPE SetCurrentPosition_fake(IDirectSoundBuffer *pThis,
                                                  DWORD dwNewPosition) {
  auto it = g_bufferMap.find(pThis);
  if (it != g_bufferMap.end()) {
    BufferWrapper *wrapper = it->second.get();
    // 将应用程序设置的影子位置转换为真实位置
    DWORD realNewPosition =
        static_cast<DWORD>(dwNewPosition / SPEEDUP) % wrapper->realBufferSize;

    // 更新我们自己的游标
    wrapper->shadowWriteCursor = dwNewPosition % wrapper->shadowBuffer.size();
    wrapper->realWriteCursor = realNewPosition;

    // 清空 SoundTouch 内部的缓冲，因为播放位置已跳变
    wrapper->soundTouch.clear();

    return wrapper->originalSetCurrentPosition(pThis, realNewPosition);
  }
  return E_FAIL;
}

// 我们自己的 CreateSoundBuffer 实现
HRESULT STDMETHODCALLTYPE
CreateSoundBuffer_fake(IDirectSound8 *pThis, const DSBUFFERDESC *pcDSBufferDesc,
                       LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter) {

  // 确保我们能处理这种格式 (例如，必须有格式信息)
  if (!pcDSBufferDesc || !pcDSBufferDesc->lpwfxFormat) {
    return g_originalCreateSoundBuffer(pThis, pcDSBufferDesc, ppDSBuffer,
                                       pUnkOuter);
  }

  // 调用原始函数创建真实的缓冲区
  HRESULT hr =
      g_originalCreateSoundBuffer(pThis, pcDSBufferDesc, ppDSBuffer, pUnkOuter);

  if (SUCCEEDED(hr) && ppDSBuffer && *ppDSBuffer) {
    IDirectSoundBuffer *pDSBuffer = *ppDSBuffer;

    // 创建我们的包装器
    auto wrapper = std::make_unique<BufferWrapper>(pDSBuffer, pcDSBufferDesc);

    // VTable Hooking
    void **vtable = *(void ***)pDSBuffer;
    DWORD oldProtect;

    // 根据 IDirectSoundBuffer 接口文档确定 VTable 索引
    const int releaseIndex = 2;
    const int getCurrentPositionIndex = 6;
    const int lockIndex = 11;
    const int playIndex = 12;
    const int setCurrentPositionIndex = 14;
    const int stopIndex = 18;
    const int unlockIndex = 19;

// 宏定义简化 Hook 过程
#define HOOK_METHOD(index, func_name, ptr_type)                                \
  wrapper->original##func_name = (ptr_type)vtable[index];                      \
  VirtualProtect(&vtable[index], sizeof(void *), PAGE_READWRITE, &oldProtect); \
  vtable[index] = &func_name##_fake;                                           \
  VirtualProtect(&vtable[index], sizeof(void *), oldProtect, &oldProtect)

    // 替换 VTable 中的指针
    HOOK_METHOD(lockIndex, Lock, Lock_ptr);
    HOOK_METHOD(unlockIndex, Unlock, Unlock_ptr);
    HOOK_METHOD(playIndex, Play, Play_ptr);
    HOOK_METHOD(stopIndex, Stop, Stop_ptr);
    HOOK_METHOD(setCurrentPositionIndex, SetCurrentPosition,
                SetCurrentPosition_ptr);
    HOOK_METHOD(getCurrentPositionIndex, GetCurrentPosition,
                GetCurrentPosition_ptr);
    HOOK_METHOD(releaseIndex, Release, Release_ptr);

    // 将包装器存入 map，键是原始的 IDirectSoundBuffer 指针
    g_bufferMap[pDSBuffer] = std::move(wrapper);
  }

  return hr;
}

// DirectSoundCreate8 的钩子函数
FAKE(HRESULT, WINAPI, DirectSoundCreate8, const GUID *lpGUID,
     LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter) {
  HRESULT hr = DirectSoundCreate8_real(lpGUID, ppDS8, pUnkOuter);

  if (SUCCEEDED(hr) && ppDS8 && *ppDS8) {
    IDirectSound8 *pDS8 = *ppDS8;
    void **vtable = *(void ***)pDS8;
    // IUnknown (3) + CreateSoundBuffer (index 0) = 3
    int createSoundBufferVTableIndex = 3;

    g_originalCreateSoundBuffer =
        (CreateSoundBuffer_ptr)vtable[createSoundBufferVTableIndex];

    DWORD oldProtect;
    VirtualProtect(&vtable[createSoundBufferVTableIndex], sizeof(void *),
                   PAGE_READWRITE, &oldProtect);
    vtable[createSoundBufferVTableIndex] = &CreateSoundBuffer_fake;
    VirtualProtect(&vtable[createSoundBufferVTableIndex], sizeof(void *),
                   oldProtect, &oldProtect);
  }

  return hr;
}

// --- 结束 Hooking 核心代码 ---

HINSTANCE mHinst = 0, mHinstDLL = 0;

UINT_PTR mProcs[12] = {0};

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
  // 我们只 hook DirectSoundCreate8，所以在这里设置
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
    // 修改为更健壮的从系统目录加载原始 DLL 的方式
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
    debug = fopen("./dsound_hook_debug_x86.log", "w");
#endif
  } else if (fdwReason == DLL_PROCESS_DETACH) {
#ifdef _DEBUG
    if (debug)
      fclose(debug);
#endif
    FreeLibrary(mHinstDLL);
  }
  return TRUE;
}

extern "C" __declspec(naked) void __stdcall DirectSoundCaptureCreate_wrapper() {
  __asm {jmp mProcs[0 * 4]}
}
extern "C" __declspec(naked) void __stdcall
DirectSoundCaptureCreate8_wrapper() {
  __asm {jmp mProcs[1 * 4]}
}
extern "C" __declspec(naked) void __stdcall
DirectSoundCaptureEnumerateA_wrapper() {
  __asm {jmp mProcs[2 * 4]}
}
extern "C" __declspec(naked) void __stdcall
DirectSoundCaptureEnumerateW_wrapper() {
  __asm {jmp mProcs[3 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundCreate_wrapper() {
  __asm {jmp mProcs[4 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundCreate8_wrapper() {
  __asm {jmp mProcs[5 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundEnumerateA_wrapper() {
  __asm {jmp mProcs[6 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundEnumerateW_wrapper() {
  __asm {jmp mProcs[7 * 4]}
}
extern "C" __declspec(naked) void __stdcall
DirectSoundFullDuplexCreate_wrapper() {
  __asm {jmp mProcs[8 * 4]}
}
extern "C" __declspec(naked) void __stdcall DllCanUnloadNow_wrapper() {
  __asm {jmp mProcs[9 * 4]}
}
extern "C" __declspec(naked) void __stdcall DllGetClassObject_wrapper() {
  __asm {jmp mProcs[10 * 4]}
}
extern "C" __declspec(naked) void __stdcall GetDeviceID_wrapper() {
  __asm {jmp mProcs[11 * 4]}
}