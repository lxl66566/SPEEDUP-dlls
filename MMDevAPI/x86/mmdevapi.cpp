#include <windows.h>

#include <stdio.h>

#include "../hook_macro.h"

HINSTANCE mHinst = 0, mHinstDLL = 0;

UINT_PTR mProcs[37] = {0};

LPCSTR mImportNames[] = {
  "ActivateAudioInterfaceAsync",
  "DllCanUnloadNow",
  "DllGetClassObject",
  "DllRegisterServer",
  "DllUnregisterServer",
  (LPCSTR)2,
  (LPCSTR)3,
  (LPCSTR)4,
  (LPCSTR)5,
  (LPCSTR)6,
  (LPCSTR)7,
  (LPCSTR)8,
  (LPCSTR)9,
  (LPCSTR)10,
  (LPCSTR)11,
  (LPCSTR)12,
  (LPCSTR)13,
  (LPCSTR)14,
  (LPCSTR)15,
  (LPCSTR)16,
  (LPCSTR)18,
  (LPCSTR)19,
  (LPCSTR)20,
  (LPCSTR)21,
  (LPCSTR)22,
  (LPCSTR)23,
  (LPCSTR)24,
  (LPCSTR)25,
  (LPCSTR)26,
  (LPCSTR)27,
  (LPCSTR)28,
  (LPCSTR)29,
  (LPCSTR)30,
  (LPCSTR)31,
  (LPCSTR)32,
  (LPCSTR)33,
  (LPCSTR)34,
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

#include "../hook.h"

inline void _hook_setup() {
#ifdef ACTIVATEAUDIOINTERFACEASYNC
  ActivateAudioInterfaceAsync_real = (ActivateAudioInterfaceAsync_ptr)mProcs[0];
  mProcs[0] = (UINT_PTR)&ActivateAudioInterfaceAsync_fake;
#endif
#ifdef DLLCANUNLOADNOW
  DllCanUnloadNow_real = (DllCanUnloadNow_ptr)mProcs[1];
  mProcs[1] = (UINT_PTR)&DllCanUnloadNow_fake;
#endif
#ifdef DLLGETCLASSOBJECT
  DllGetClassObject_real = (DllGetClassObject_ptr)mProcs[2];
  mProcs[2] = (UINT_PTR)&DllGetClassObject_fake;
#endif
#ifdef DLLREGISTERSERVER
  DllRegisterServer_real = (DllRegisterServer_ptr)mProcs[3];
  mProcs[3] = (UINT_PTR)&DllRegisterServer_fake;
#endif
#ifdef DLLUNREGISTERSERVER
  DllUnregisterServer_real = (DllUnregisterServer_ptr)mProcs[4];
  mProcs[4] = (UINT_PTR)&DllUnregisterServer_fake;
#endif
#ifdef EXPORTBYORDINAL2
  ExportByOrdinal2_real = (ExportByOrdinal2_ptr)mProcs[5];
  mProcs[5] = (UINT_PTR)&ExportByOrdinal2_fake;
#endif
#ifdef EXPORTBYORDINAL3
  ExportByOrdinal3_real = (ExportByOrdinal3_ptr)mProcs[6];
  mProcs[6] = (UINT_PTR)&ExportByOrdinal3_fake;
#endif
#ifdef EXPORTBYORDINAL4
  ExportByOrdinal4_real = (ExportByOrdinal4_ptr)mProcs[7];
  mProcs[7] = (UINT_PTR)&ExportByOrdinal4_fake;
#endif
#ifdef EXPORTBYORDINAL5
  ExportByOrdinal5_real = (ExportByOrdinal5_ptr)mProcs[8];
  mProcs[8] = (UINT_PTR)&ExportByOrdinal5_fake;
#endif
#ifdef EXPORTBYORDINAL6
  ExportByOrdinal6_real = (ExportByOrdinal6_ptr)mProcs[9];
  mProcs[9] = (UINT_PTR)&ExportByOrdinal6_fake;
#endif
#ifdef EXPORTBYORDINAL7
  ExportByOrdinal7_real = (ExportByOrdinal7_ptr)mProcs[10];
  mProcs[10] = (UINT_PTR)&ExportByOrdinal7_fake;
#endif
#ifdef EXPORTBYORDINAL8
  ExportByOrdinal8_real = (ExportByOrdinal8_ptr)mProcs[11];
  mProcs[11] = (UINT_PTR)&ExportByOrdinal8_fake;
#endif
#ifdef EXPORTBYORDINAL9
  ExportByOrdinal9_real = (ExportByOrdinal9_ptr)mProcs[12];
  mProcs[12] = (UINT_PTR)&ExportByOrdinal9_fake;
#endif
#ifdef EXPORTBYORDINAL10
  ExportByOrdinal10_real = (ExportByOrdinal10_ptr)mProcs[13];
  mProcs[13] = (UINT_PTR)&ExportByOrdinal10_fake;
#endif
#ifdef EXPORTBYORDINAL11
  ExportByOrdinal11_real = (ExportByOrdinal11_ptr)mProcs[14];
  mProcs[14] = (UINT_PTR)&ExportByOrdinal11_fake;
#endif
#ifdef EXPORTBYORDINAL12
  ExportByOrdinal12_real = (ExportByOrdinal12_ptr)mProcs[15];
  mProcs[15] = (UINT_PTR)&ExportByOrdinal12_fake;
#endif
#ifdef EXPORTBYORDINAL13
  ExportByOrdinal13_real = (ExportByOrdinal13_ptr)mProcs[16];
  mProcs[16] = (UINT_PTR)&ExportByOrdinal13_fake;
#endif
#ifdef EXPORTBYORDINAL14
  ExportByOrdinal14_real = (ExportByOrdinal14_ptr)mProcs[17];
  mProcs[17] = (UINT_PTR)&ExportByOrdinal14_fake;
#endif
#ifdef EXPORTBYORDINAL15
  ExportByOrdinal15_real = (ExportByOrdinal15_ptr)mProcs[18];
  mProcs[18] = (UINT_PTR)&ExportByOrdinal15_fake;
#endif
#ifdef EXPORTBYORDINAL16
  ExportByOrdinal16_real = (ExportByOrdinal16_ptr)mProcs[19];
  mProcs[19] = (UINT_PTR)&ExportByOrdinal16_fake;
#endif
#ifdef EXPORTBYORDINAL18
  ExportByOrdinal18_real = (ExportByOrdinal18_ptr)mProcs[20];
  mProcs[20] = (UINT_PTR)&ExportByOrdinal18_fake;
#endif
#ifdef EXPORTBYORDINAL19
  ExportByOrdinal19_real = (ExportByOrdinal19_ptr)mProcs[21];
  mProcs[21] = (UINT_PTR)&ExportByOrdinal19_fake;
#endif
#ifdef EXPORTBYORDINAL20
  ExportByOrdinal20_real = (ExportByOrdinal20_ptr)mProcs[22];
  mProcs[22] = (UINT_PTR)&ExportByOrdinal20_fake;
#endif
#ifdef EXPORTBYORDINAL21
  ExportByOrdinal21_real = (ExportByOrdinal21_ptr)mProcs[23];
  mProcs[23] = (UINT_PTR)&ExportByOrdinal21_fake;
#endif
#ifdef EXPORTBYORDINAL22
  ExportByOrdinal22_real = (ExportByOrdinal22_ptr)mProcs[24];
  mProcs[24] = (UINT_PTR)&ExportByOrdinal22_fake;
#endif
#ifdef EXPORTBYORDINAL23
  ExportByOrdinal23_real = (ExportByOrdinal23_ptr)mProcs[25];
  mProcs[25] = (UINT_PTR)&ExportByOrdinal23_fake;
#endif
#ifdef EXPORTBYORDINAL24
  ExportByOrdinal24_real = (ExportByOrdinal24_ptr)mProcs[26];
  mProcs[26] = (UINT_PTR)&ExportByOrdinal24_fake;
#endif
#ifdef EXPORTBYORDINAL25
  ExportByOrdinal25_real = (ExportByOrdinal25_ptr)mProcs[27];
  mProcs[27] = (UINT_PTR)&ExportByOrdinal25_fake;
#endif
#ifdef EXPORTBYORDINAL26
  ExportByOrdinal26_real = (ExportByOrdinal26_ptr)mProcs[28];
  mProcs[28] = (UINT_PTR)&ExportByOrdinal26_fake;
#endif
#ifdef EXPORTBYORDINAL27
  ExportByOrdinal27_real = (ExportByOrdinal27_ptr)mProcs[29];
  mProcs[29] = (UINT_PTR)&ExportByOrdinal27_fake;
#endif
#ifdef EXPORTBYORDINAL28
  ExportByOrdinal28_real = (ExportByOrdinal28_ptr)mProcs[30];
  mProcs[30] = (UINT_PTR)&ExportByOrdinal28_fake;
#endif
#ifdef EXPORTBYORDINAL29
  ExportByOrdinal29_real = (ExportByOrdinal29_ptr)mProcs[31];
  mProcs[31] = (UINT_PTR)&ExportByOrdinal29_fake;
#endif
#ifdef EXPORTBYORDINAL30
  ExportByOrdinal30_real = (ExportByOrdinal30_ptr)mProcs[32];
  mProcs[32] = (UINT_PTR)&ExportByOrdinal30_fake;
#endif
#ifdef EXPORTBYORDINAL31
  ExportByOrdinal31_real = (ExportByOrdinal31_ptr)mProcs[33];
  mProcs[33] = (UINT_PTR)&ExportByOrdinal31_fake;
#endif
#ifdef EXPORTBYORDINAL32
  ExportByOrdinal32_real = (ExportByOrdinal32_ptr)mProcs[34];
  mProcs[34] = (UINT_PTR)&ExportByOrdinal32_fake;
#endif
#ifdef EXPORTBYORDINAL33
  ExportByOrdinal33_real = (ExportByOrdinal33_ptr)mProcs[35];
  mProcs[35] = (UINT_PTR)&ExportByOrdinal33_fake;
#endif
#ifdef EXPORTBYORDINAL34
  ExportByOrdinal34_real = (ExportByOrdinal34_ptr)mProcs[36];
  mProcs[36] = (UINT_PTR)&ExportByOrdinal34_fake;
#endif
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  mHinst = hinstDLL;
  if (fdwReason == DLL_PROCESS_ATTACH) {
    mHinstDLL = LoadLibrary("C:\\Windows\\SysWOW64\\MMDevAPI.dll");
    if (!mHinstDLL) {
      return FALSE;
    }
    for (int i = 0; i < 37; ++i) {
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

extern "C" __declspec(naked) void __stdcall ActivateAudioInterfaceAsync_wrapper(){
#ifdef _DEBUG
  log_info("calling ActivateAudioInterfaceAsync");
#endif
  __asm{jmp mProcs[0 * 4]}
}
extern "C" __declspec(naked) void __stdcall DllCanUnloadNow_wrapper(){
#ifdef _DEBUG
  log_info("calling DllCanUnloadNow");
#endif
  __asm{jmp mProcs[1 * 4]}
}
extern "C" __declspec(naked) void __stdcall DllGetClassObject_wrapper(){
#ifdef _DEBUG
  log_info("calling DllGetClassObject");
#endif
  __asm{jmp mProcs[2 * 4]}
}
extern "C" __declspec(naked) void __stdcall DllRegisterServer_wrapper(){
#ifdef _DEBUG
  log_info("calling DllRegisterServer");
#endif
  __asm{jmp mProcs[3 * 4]}
}
extern "C" __declspec(naked) void __stdcall DllUnregisterServer_wrapper(){
#ifdef _DEBUG
  log_info("calling DllUnregisterServer");
#endif
  __asm{jmp mProcs[4 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal2(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[5 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal3(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[6 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal4(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[7 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal5(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[8 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal6(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[9 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal7(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[10 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal8(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[11 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal9(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[12 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal10(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[13 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal11(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[14 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal12(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[15 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal13(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[16 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal14(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[17 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal15(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[18 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal16(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[19 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal18(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[20 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal19(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[21 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal20(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[22 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal21(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[23 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal22(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[24 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal23(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[25 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal24(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[26 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal25(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[27 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal26(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[28 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal27(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[29 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal28(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[30 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal29(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[31 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal30(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[32 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal31(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[33 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal32(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[34 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal33(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[35 * 4]}
}
extern "C" __declspec(naked) void __stdcall ExportByOrdinal34(){
#ifdef _DEBUG
  log_info("calling [NONAME]");
#endif
  __asm{jmp mProcs[36 * 4]}
}
