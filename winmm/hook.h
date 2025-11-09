#ifndef _HOOK_H
#define _HOOK_H
#include <windows.h>

#include <malloc.h>   // 包含 _alloca, 用于在栈上分配内存
#include <mmsystem.h> // 包含 waveOutOpen 和 WAVEFORMATEX 的定义

// 通过定义 WAVEOUTOPEN，我们告诉 winmm.cpp 中的 _hook_setup() 函数
// 来启用我们为 waveOutOpen 编写的 hook。
#define WAVEOUTOPEN

// FAKE 是一个来自 hook_macro.h 的宏，它会帮助我们
// 1. 定义一个指向原始函数(real)的函数指针 (waveOutOpen_real)
// 2. 定义我们的包装函数(fake) (waveOutOpen_fake)
//
// waveOutOpen 的标准函数签名如下：
// MMRESULT WINAPI waveOutOpen(
//   LPHWAVEOUT      phwo,
//   UINT            uDeviceID,
//   LPCWAVEFORMATEX pwfx,
//   DWORD_PTR       dwCallback,
//   DWORD_PTR       dwInstance,
//   DWORD           fdwOpen
// );
FAKE(MMRESULT, WINAPI, waveOutOpen, LPHWAVEOUT phwo, UINT uDeviceID,
     LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance,
     DWORD fdwOpen) {
  // 如果应用程序没有提供有效的音频格式，我们无法进行修改。
  // 直接调用原始函数并返回。
  if (!pwfx) {
    return waveOutOpen_real(phwo, uDeviceID, pwfx, dwCallback, dwInstance,
                            fdwOpen);
  }

  // 这里的逻辑完全参照了您提供的 Wine diff。
  // 我们将在栈上创建一个新的 WAVEFORMATEX 结构，并修改其采样率。

  // WAVEFORMATEX 结构末尾可能附带额外数据 (由 pwfx->cbSize 指示)。
  // 我们必须分配足够的空间来容纳基础结构和这些额外数据。
  // _alloca 是一个在函数栈上分配内存的便捷方法，函数返回时会自动释放。
  size_t formatSize = sizeof(WAVEFORMATEX) + pwfx->cbSize;
  WAVEFORMATEX *modified_pwfx = (WAVEFORMATEX *)_alloca(formatSize);

  // 将应用程序请求的原始音频格式完整复制到我们的新结构中。
  memcpy(modified_pwfx, pwfx, formatSize);

  // 应用加速倍率。这是实现功能的关键。
  // 我们将采样率 (nSamplesPerSec) 和每秒平均字节数 (nAvgBytesPerSec) 乘以
  // SPEEDUP。 注意类型转换，确保浮点数乘法在转换为 DWORD 之前完成。
  modified_pwfx->nSamplesPerSec =
      (DWORD)((double)modified_pwfx->nSamplesPerSec * SPEEDUP);
  modified_pwfx->nAvgBytesPerSec =
      (DWORD)((double)modified_pwfx->nAvgBytesPerSec * SPEEDUP);

  // 现在，调用真正的 waveOutOpen_real 函数，但传入的是我们修改过的音频格式。
  // 如此一来，底层的音频驱动和硬件将以更高的速率进行初始化。
  // 当应用程序后续通过 waveOutWrite 写入音频数据时，这些数据会被更快地消耗，
  // 从而达到音频加速播放的目的。
  MMRESULT result = waveOutOpen_real(phwo, uDeviceID, modified_pwfx, dwCallback,
                                     dwInstance, fdwOpen);

  // 返回原始函数调用的结果。
  return result;
}
#endif