#include <windows.h>
#include <stdio.h>
#include "../hook_macro.h"

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
inline void log_info(const char* info) {
}
#else
FILE* debug;
inline void log_info(const char* info) {
  fprintf(debug, "%s\n", info);
  fflush(debug);
}
#endif

#include "../empty.h"

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
  DirectSoundCaptureEnumerateA_real = (DirectSoundCaptureEnumerateA_ptr)mProcs[2];
  mProcs[2] = (UINT_PTR)&DirectSoundCaptureEnumerateA_fake;
#endif
#ifdef DIRECTSOUNDCAPTUREENUMERATEW
  DirectSoundCaptureEnumerateW_real = (DirectSoundCaptureEnumerateW_ptr)mProcs[3];
  mProcs[3] = (UINT_PTR)&DirectSoundCaptureEnumerateW_fake;
#endif
#ifdef DIRECTSOUNDCREATE
  DirectSoundCreate_real = (DirectSoundCreate_ptr)mProcs[4];
  mProcs[4] = (UINT_PTR)&DirectSoundCreate_fake;
#endif
#ifdef DIRECTSOUNDCREATE8
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
    mHinstDLL = LoadLibrary("C:\\Windows\\SysWOW64\\dsound.dll");
    if (!mHinstDLL) {
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

extern "C" __declspec(naked) void __stdcall DirectSoundCaptureCreate_wrapper(){
#ifdef _DEBUG
  log_info("calling DirectSoundCaptureCreate");
#endif
  __asm{jmp mProcs[0 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundCaptureCreate8_wrapper(){
#ifdef _DEBUG
  log_info("calling DirectSoundCaptureCreate8");
#endif
  __asm{jmp mProcs[1 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundCaptureEnumerateA_wrapper(){
#ifdef _DEBUG
  log_info("calling DirectSoundCaptureEnumerateA");
#endif
  __asm{jmp mProcs[2 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundCaptureEnumerateW_wrapper(){
#ifdef _DEBUG
  log_info("calling DirectSoundCaptureEnumerateW");
#endif
  __asm{jmp mProcs[3 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundCreate_wrapper(){
#ifdef _DEBUG
  log_info("calling DirectSoundCreate");
#endif
  __asm{jmp mProcs[4 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundCreate8_wrapper(){
#ifdef _DEBUG
  log_info("calling DirectSoundCreate8");
#endif
  __asm{jmp mProcs[5 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundEnumerateA_wrapper(){
#ifdef _DEBUG
  log_info("calling DirectSoundEnumerateA");
#endif
  __asm{jmp mProcs[6 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundEnumerateW_wrapper(){
#ifdef _DEBUG
  log_info("calling DirectSoundEnumerateW");
#endif
  __asm{jmp mProcs[7 * 4]}
}
extern "C" __declspec(naked) void __stdcall DirectSoundFullDuplexCreate_wrapper(){
#ifdef _DEBUG
  log_info("calling DirectSoundFullDuplexCreate");
#endif
  __asm{jmp mProcs[8 * 4]}
}
extern "C" __declspec(naked) void __stdcall DllCanUnloadNow_wrapper(){
#ifdef _DEBUG
  log_info("calling DllCanUnloadNow");
#endif
  __asm{jmp mProcs[9 * 4]}
}
extern "C" __declspec(naked) void __stdcall DllGetClassObject_wrapper(){
#ifdef _DEBUG
  log_info("calling DllGetClassObject");
#endif
  __asm{jmp mProcs[10 * 4]}
}
extern "C" __declspec(naked) void __stdcall GetDeviceID_wrapper(){
#ifdef _DEBUG
  log_info("calling GetDeviceID");
#endif
  __asm{jmp mProcs[11 * 4]}
}
