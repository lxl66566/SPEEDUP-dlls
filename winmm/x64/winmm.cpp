#include <windows.h>
#include <stdio.h>
#include "hook_macro.h"

HINSTANCE mHinst = 0, mHinstDLL = 0;

extern "C" UINT_PTR mProcs[181] = {0};

LPCSTR mImportNames[] = {
  "CloseDriver",
  "DefDriverProc",
  "DriverCallback",
  "DrvGetModuleHandle",
  "GetDriverModuleHandle",
  "OpenDriver",
  "PlaySound",
  "PlaySoundA",
  "PlaySoundW",
  "SendDriverMessage",
  "WOWAppExit",
  "auxGetDevCapsA",
  "auxGetDevCapsW",
  "auxGetNumDevs",
  "auxGetVolume",
  "auxOutMessage",
  "auxSetVolume",
  "joyConfigChanged",
  "joyGetDevCapsA",
  "joyGetDevCapsW",
  "joyGetNumDevs",
  "joyGetPos",
  "joyGetPosEx",
  "joyGetThreshold",
  "joyReleaseCapture",
  "joySetCapture",
  "joySetThreshold",
  "mciDriverNotify",
  "mciDriverYield",
  "mciExecute",
  "mciFreeCommandResource",
  "mciGetCreatorTask",
  "mciGetDeviceIDA",
  "mciGetDeviceIDFromElementIDA",
  "mciGetDeviceIDFromElementIDW",
  "mciGetDeviceIDW",
  "mciGetDriverData",
  "mciGetErrorStringA",
  "mciGetErrorStringW",
  "mciGetYieldProc",
  "mciLoadCommandResource",
  "mciSendCommandA",
  "mciSendCommandW",
  "mciSendStringA",
  "mciSendStringW",
  "mciSetDriverData",
  "mciSetYieldProc",
  "midiConnect",
  "midiDisconnect",
  "midiInAddBuffer",
  "midiInClose",
  "midiInGetDevCapsA",
  "midiInGetDevCapsW",
  "midiInGetErrorTextA",
  "midiInGetErrorTextW",
  "midiInGetID",
  "midiInGetNumDevs",
  "midiInMessage",
  "midiInOpen",
  "midiInPrepareHeader",
  "midiInReset",
  "midiInStart",
  "midiInStop",
  "midiInUnprepareHeader",
  "midiOutCacheDrumPatches",
  "midiOutCachePatches",
  "midiOutClose",
  "midiOutGetDevCapsA",
  "midiOutGetDevCapsW",
  "midiOutGetErrorTextA",
  "midiOutGetErrorTextW",
  "midiOutGetID",
  "midiOutGetNumDevs",
  "midiOutGetVolume",
  "midiOutLongMsg",
  "midiOutMessage",
  "midiOutOpen",
  "midiOutPrepareHeader",
  "midiOutReset",
  "midiOutSetVolume",
  "midiOutShortMsg",
  "midiOutUnprepareHeader",
  "midiStreamClose",
  "midiStreamOpen",
  "midiStreamOut",
  "midiStreamPause",
  "midiStreamPosition",
  "midiStreamProperty",
  "midiStreamRestart",
  "midiStreamStop",
  "mixerClose",
  "mixerGetControlDetailsA",
  "mixerGetControlDetailsW",
  "mixerGetDevCapsA",
  "mixerGetDevCapsW",
  "mixerGetID",
  "mixerGetLineControlsA",
  "mixerGetLineControlsW",
  "mixerGetLineInfoA",
  "mixerGetLineInfoW",
  "mixerGetNumDevs",
  "mixerMessage",
  "mixerOpen",
  "mixerSetControlDetails",
  "mmDrvInstall",
  "mmGetCurrentTask",
  "mmTaskBlock",
  "mmTaskCreate",
  "mmTaskSignal",
  "mmTaskYield",
  "mmioAdvance",
  "mmioAscend",
  "mmioClose",
  "mmioCreateChunk",
  "mmioDescend",
  "mmioFlush",
  "mmioGetInfo",
  "mmioInstallIOProcA",
  "mmioInstallIOProcW",
  "mmioOpenA",
  "mmioOpenW",
  "mmioRead",
  "mmioRenameA",
  "mmioRenameW",
  "mmioSeek",
  "mmioSendMessage",
  "mmioSetBuffer",
  "mmioSetInfo",
  "mmioStringToFOURCCA",
  "mmioStringToFOURCCW",
  "mmioWrite",
  "mmsystemGetVersion",
  "sndPlaySoundA",
  "sndPlaySoundW",
  "timeBeginPeriod",
  "timeEndPeriod",
  "timeGetDevCaps",
  "timeGetSystemTime",
  "timeGetTime",
  "timeKillEvent",
  "timeSetEvent",
  "waveInAddBuffer",
  "waveInClose",
  "waveInGetDevCapsA",
  "waveInGetDevCapsW",
  "waveInGetErrorTextA",
  "waveInGetErrorTextW",
  "waveInGetID",
  "waveInGetNumDevs",
  "waveInGetPosition",
  "waveInMessage",
  "waveInOpen",
  "waveInPrepareHeader",
  "waveInReset",
  "waveInStart",
  "waveInStop",
  "waveInUnprepareHeader",
  "waveOutBreakLoop",
  "waveOutClose",
  "waveOutGetDevCapsA",
  "waveOutGetDevCapsW",
  "waveOutGetErrorTextA",
  "waveOutGetErrorTextW",
  "waveOutGetID",
  "waveOutGetNumDevs",
  "waveOutGetPitch",
  "waveOutGetPlaybackRate",
  "waveOutGetPosition",
  "waveOutGetVolume",
  "waveOutMessage",
  "waveOutOpen",
  "waveOutPause",
  "waveOutPrepareHeader",
  "waveOutReset",
  "waveOutRestart",
  "waveOutSetPitch",
  "waveOutSetPlaybackRate",
  "waveOutSetVolume",
  "waveOutUnprepareHeader",
  "waveOutWrite",
  (LPCSTR)2,
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
#ifdef CLOSEDRIVER
  CloseDriver_real = (CloseDriver_ptr)mProcs[0];
  mProcs[0] = (UINT_PTR)&CloseDriver_fake;
#endif
#ifdef DEFDRIVERPROC
  DefDriverProc_real = (DefDriverProc_ptr)mProcs[1];
  mProcs[1] = (UINT_PTR)&DefDriverProc_fake;
#endif
#ifdef DRIVERCALLBACK
  DriverCallback_real = (DriverCallback_ptr)mProcs[2];
  mProcs[2] = (UINT_PTR)&DriverCallback_fake;
#endif
#ifdef DRVGETMODULEHANDLE
  DrvGetModuleHandle_real = (DrvGetModuleHandle_ptr)mProcs[3];
  mProcs[3] = (UINT_PTR)&DrvGetModuleHandle_fake;
#endif
#ifdef GETDRIVERMODULEHANDLE
  GetDriverModuleHandle_real = (GetDriverModuleHandle_ptr)mProcs[4];
  mProcs[4] = (UINT_PTR)&GetDriverModuleHandle_fake;
#endif
#ifdef OPENDRIVER
  OpenDriver_real = (OpenDriver_ptr)mProcs[5];
  mProcs[5] = (UINT_PTR)&OpenDriver_fake;
#endif
#ifdef PLAYSOUND
  PlaySound_real = (PlaySound_ptr)mProcs[6];
  mProcs[6] = (UINT_PTR)&PlaySound_fake;
#endif
#ifdef PLAYSOUNDA
  PlaySoundA_real = (PlaySoundA_ptr)mProcs[7];
  mProcs[7] = (UINT_PTR)&PlaySoundA_fake;
#endif
#ifdef PLAYSOUNDW
  PlaySoundW_real = (PlaySoundW_ptr)mProcs[8];
  mProcs[8] = (UINT_PTR)&PlaySoundW_fake;
#endif
#ifdef SENDDRIVERMESSAGE
  SendDriverMessage_real = (SendDriverMessage_ptr)mProcs[9];
  mProcs[9] = (UINT_PTR)&SendDriverMessage_fake;
#endif
#ifdef WOWAPPEXIT
  WOWAppExit_real = (WOWAppExit_ptr)mProcs[10];
  mProcs[10] = (UINT_PTR)&WOWAppExit_fake;
#endif
#ifdef AUXGETDEVCAPSA
  auxGetDevCapsA_real = (auxGetDevCapsA_ptr)mProcs[11];
  mProcs[11] = (UINT_PTR)&auxGetDevCapsA_fake;
#endif
#ifdef AUXGETDEVCAPSW
  auxGetDevCapsW_real = (auxGetDevCapsW_ptr)mProcs[12];
  mProcs[12] = (UINT_PTR)&auxGetDevCapsW_fake;
#endif
#ifdef AUXGETNUMDEVS
  auxGetNumDevs_real = (auxGetNumDevs_ptr)mProcs[13];
  mProcs[13] = (UINT_PTR)&auxGetNumDevs_fake;
#endif
#ifdef AUXGETVOLUME
  auxGetVolume_real = (auxGetVolume_ptr)mProcs[14];
  mProcs[14] = (UINT_PTR)&auxGetVolume_fake;
#endif
#ifdef AUXOUTMESSAGE
  auxOutMessage_real = (auxOutMessage_ptr)mProcs[15];
  mProcs[15] = (UINT_PTR)&auxOutMessage_fake;
#endif
#ifdef AUXSETVOLUME
  auxSetVolume_real = (auxSetVolume_ptr)mProcs[16];
  mProcs[16] = (UINT_PTR)&auxSetVolume_fake;
#endif
#ifdef JOYCONFIGCHANGED
  joyConfigChanged_real = (joyConfigChanged_ptr)mProcs[17];
  mProcs[17] = (UINT_PTR)&joyConfigChanged_fake;
#endif
#ifdef JOYGETDEVCAPSA
  joyGetDevCapsA_real = (joyGetDevCapsA_ptr)mProcs[18];
  mProcs[18] = (UINT_PTR)&joyGetDevCapsA_fake;
#endif
#ifdef JOYGETDEVCAPSW
  joyGetDevCapsW_real = (joyGetDevCapsW_ptr)mProcs[19];
  mProcs[19] = (UINT_PTR)&joyGetDevCapsW_fake;
#endif
#ifdef JOYGETNUMDEVS
  joyGetNumDevs_real = (joyGetNumDevs_ptr)mProcs[20];
  mProcs[20] = (UINT_PTR)&joyGetNumDevs_fake;
#endif
#ifdef JOYGETPOS
  joyGetPos_real = (joyGetPos_ptr)mProcs[21];
  mProcs[21] = (UINT_PTR)&joyGetPos_fake;
#endif
#ifdef JOYGETPOSEX
  joyGetPosEx_real = (joyGetPosEx_ptr)mProcs[22];
  mProcs[22] = (UINT_PTR)&joyGetPosEx_fake;
#endif
#ifdef JOYGETTHRESHOLD
  joyGetThreshold_real = (joyGetThreshold_ptr)mProcs[23];
  mProcs[23] = (UINT_PTR)&joyGetThreshold_fake;
#endif
#ifdef JOYRELEASECAPTURE
  joyReleaseCapture_real = (joyReleaseCapture_ptr)mProcs[24];
  mProcs[24] = (UINT_PTR)&joyReleaseCapture_fake;
#endif
#ifdef JOYSETCAPTURE
  joySetCapture_real = (joySetCapture_ptr)mProcs[25];
  mProcs[25] = (UINT_PTR)&joySetCapture_fake;
#endif
#ifdef JOYSETTHRESHOLD
  joySetThreshold_real = (joySetThreshold_ptr)mProcs[26];
  mProcs[26] = (UINT_PTR)&joySetThreshold_fake;
#endif
#ifdef MCIDRIVERNOTIFY
  mciDriverNotify_real = (mciDriverNotify_ptr)mProcs[27];
  mProcs[27] = (UINT_PTR)&mciDriverNotify_fake;
#endif
#ifdef MCIDRIVERYIELD
  mciDriverYield_real = (mciDriverYield_ptr)mProcs[28];
  mProcs[28] = (UINT_PTR)&mciDriverYield_fake;
#endif
#ifdef MCIEXECUTE
  mciExecute_real = (mciExecute_ptr)mProcs[29];
  mProcs[29] = (UINT_PTR)&mciExecute_fake;
#endif
#ifdef MCIFREECOMMANDRESOURCE
  mciFreeCommandResource_real = (mciFreeCommandResource_ptr)mProcs[30];
  mProcs[30] = (UINT_PTR)&mciFreeCommandResource_fake;
#endif
#ifdef MCIGETCREATORTASK
  mciGetCreatorTask_real = (mciGetCreatorTask_ptr)mProcs[31];
  mProcs[31] = (UINT_PTR)&mciGetCreatorTask_fake;
#endif
#ifdef MCIGETDEVICEIDA
  mciGetDeviceIDA_real = (mciGetDeviceIDA_ptr)mProcs[32];
  mProcs[32] = (UINT_PTR)&mciGetDeviceIDA_fake;
#endif
#ifdef MCIGETDEVICEIDFROMELEMENTIDA
  mciGetDeviceIDFromElementIDA_real = (mciGetDeviceIDFromElementIDA_ptr)mProcs[33];
  mProcs[33] = (UINT_PTR)&mciGetDeviceIDFromElementIDA_fake;
#endif
#ifdef MCIGETDEVICEIDFROMELEMENTIDW
  mciGetDeviceIDFromElementIDW_real = (mciGetDeviceIDFromElementIDW_ptr)mProcs[34];
  mProcs[34] = (UINT_PTR)&mciGetDeviceIDFromElementIDW_fake;
#endif
#ifdef MCIGETDEVICEIDW
  mciGetDeviceIDW_real = (mciGetDeviceIDW_ptr)mProcs[35];
  mProcs[35] = (UINT_PTR)&mciGetDeviceIDW_fake;
#endif
#ifdef MCIGETDRIVERDATA
  mciGetDriverData_real = (mciGetDriverData_ptr)mProcs[36];
  mProcs[36] = (UINT_PTR)&mciGetDriverData_fake;
#endif
#ifdef MCIGETERRORSTRINGA
  mciGetErrorStringA_real = (mciGetErrorStringA_ptr)mProcs[37];
  mProcs[37] = (UINT_PTR)&mciGetErrorStringA_fake;
#endif
#ifdef MCIGETERRORSTRINGW
  mciGetErrorStringW_real = (mciGetErrorStringW_ptr)mProcs[38];
  mProcs[38] = (UINT_PTR)&mciGetErrorStringW_fake;
#endif
#ifdef MCIGETYIELDPROC
  mciGetYieldProc_real = (mciGetYieldProc_ptr)mProcs[39];
  mProcs[39] = (UINT_PTR)&mciGetYieldProc_fake;
#endif
#ifdef MCILOADCOMMANDRESOURCE
  mciLoadCommandResource_real = (mciLoadCommandResource_ptr)mProcs[40];
  mProcs[40] = (UINT_PTR)&mciLoadCommandResource_fake;
#endif
#ifdef MCISENDCOMMANDA
  mciSendCommandA_real = (mciSendCommandA_ptr)mProcs[41];
  mProcs[41] = (UINT_PTR)&mciSendCommandA_fake;
#endif
#ifdef MCISENDCOMMANDW
  mciSendCommandW_real = (mciSendCommandW_ptr)mProcs[42];
  mProcs[42] = (UINT_PTR)&mciSendCommandW_fake;
#endif
#ifdef MCISENDSTRINGA
  mciSendStringA_real = (mciSendStringA_ptr)mProcs[43];
  mProcs[43] = (UINT_PTR)&mciSendStringA_fake;
#endif
#ifdef MCISENDSTRINGW
  mciSendStringW_real = (mciSendStringW_ptr)mProcs[44];
  mProcs[44] = (UINT_PTR)&mciSendStringW_fake;
#endif
#ifdef MCISETDRIVERDATA
  mciSetDriverData_real = (mciSetDriverData_ptr)mProcs[45];
  mProcs[45] = (UINT_PTR)&mciSetDriverData_fake;
#endif
#ifdef MCISETYIELDPROC
  mciSetYieldProc_real = (mciSetYieldProc_ptr)mProcs[46];
  mProcs[46] = (UINT_PTR)&mciSetYieldProc_fake;
#endif
#ifdef MIDICONNECT
  midiConnect_real = (midiConnect_ptr)mProcs[47];
  mProcs[47] = (UINT_PTR)&midiConnect_fake;
#endif
#ifdef MIDIDISCONNECT
  midiDisconnect_real = (midiDisconnect_ptr)mProcs[48];
  mProcs[48] = (UINT_PTR)&midiDisconnect_fake;
#endif
#ifdef MIDIINADDBUFFER
  midiInAddBuffer_real = (midiInAddBuffer_ptr)mProcs[49];
  mProcs[49] = (UINT_PTR)&midiInAddBuffer_fake;
#endif
#ifdef MIDIINCLOSE
  midiInClose_real = (midiInClose_ptr)mProcs[50];
  mProcs[50] = (UINT_PTR)&midiInClose_fake;
#endif
#ifdef MIDIINGETDEVCAPSA
  midiInGetDevCapsA_real = (midiInGetDevCapsA_ptr)mProcs[51];
  mProcs[51] = (UINT_PTR)&midiInGetDevCapsA_fake;
#endif
#ifdef MIDIINGETDEVCAPSW
  midiInGetDevCapsW_real = (midiInGetDevCapsW_ptr)mProcs[52];
  mProcs[52] = (UINT_PTR)&midiInGetDevCapsW_fake;
#endif
#ifdef MIDIINGETERRORTEXTA
  midiInGetErrorTextA_real = (midiInGetErrorTextA_ptr)mProcs[53];
  mProcs[53] = (UINT_PTR)&midiInGetErrorTextA_fake;
#endif
#ifdef MIDIINGETERRORTEXTW
  midiInGetErrorTextW_real = (midiInGetErrorTextW_ptr)mProcs[54];
  mProcs[54] = (UINT_PTR)&midiInGetErrorTextW_fake;
#endif
#ifdef MIDIINGETID
  midiInGetID_real = (midiInGetID_ptr)mProcs[55];
  mProcs[55] = (UINT_PTR)&midiInGetID_fake;
#endif
#ifdef MIDIINGETNUMDEVS
  midiInGetNumDevs_real = (midiInGetNumDevs_ptr)mProcs[56];
  mProcs[56] = (UINT_PTR)&midiInGetNumDevs_fake;
#endif
#ifdef MIDIINMESSAGE
  midiInMessage_real = (midiInMessage_ptr)mProcs[57];
  mProcs[57] = (UINT_PTR)&midiInMessage_fake;
#endif
#ifdef MIDIINOPEN
  midiInOpen_real = (midiInOpen_ptr)mProcs[58];
  mProcs[58] = (UINT_PTR)&midiInOpen_fake;
#endif
#ifdef MIDIINPREPAREHEADER
  midiInPrepareHeader_real = (midiInPrepareHeader_ptr)mProcs[59];
  mProcs[59] = (UINT_PTR)&midiInPrepareHeader_fake;
#endif
#ifdef MIDIINRESET
  midiInReset_real = (midiInReset_ptr)mProcs[60];
  mProcs[60] = (UINT_PTR)&midiInReset_fake;
#endif
#ifdef MIDIINSTART
  midiInStart_real = (midiInStart_ptr)mProcs[61];
  mProcs[61] = (UINT_PTR)&midiInStart_fake;
#endif
#ifdef MIDIINSTOP
  midiInStop_real = (midiInStop_ptr)mProcs[62];
  mProcs[62] = (UINT_PTR)&midiInStop_fake;
#endif
#ifdef MIDIINUNPREPAREHEADER
  midiInUnprepareHeader_real = (midiInUnprepareHeader_ptr)mProcs[63];
  mProcs[63] = (UINT_PTR)&midiInUnprepareHeader_fake;
#endif
#ifdef MIDIOUTCACHEDRUMPATCHES
  midiOutCacheDrumPatches_real = (midiOutCacheDrumPatches_ptr)mProcs[64];
  mProcs[64] = (UINT_PTR)&midiOutCacheDrumPatches_fake;
#endif
#ifdef MIDIOUTCACHEPATCHES
  midiOutCachePatches_real = (midiOutCachePatches_ptr)mProcs[65];
  mProcs[65] = (UINT_PTR)&midiOutCachePatches_fake;
#endif
#ifdef MIDIOUTCLOSE
  midiOutClose_real = (midiOutClose_ptr)mProcs[66];
  mProcs[66] = (UINT_PTR)&midiOutClose_fake;
#endif
#ifdef MIDIOUTGETDEVCAPSA
  midiOutGetDevCapsA_real = (midiOutGetDevCapsA_ptr)mProcs[67];
  mProcs[67] = (UINT_PTR)&midiOutGetDevCapsA_fake;
#endif
#ifdef MIDIOUTGETDEVCAPSW
  midiOutGetDevCapsW_real = (midiOutGetDevCapsW_ptr)mProcs[68];
  mProcs[68] = (UINT_PTR)&midiOutGetDevCapsW_fake;
#endif
#ifdef MIDIOUTGETERRORTEXTA
  midiOutGetErrorTextA_real = (midiOutGetErrorTextA_ptr)mProcs[69];
  mProcs[69] = (UINT_PTR)&midiOutGetErrorTextA_fake;
#endif
#ifdef MIDIOUTGETERRORTEXTW
  midiOutGetErrorTextW_real = (midiOutGetErrorTextW_ptr)mProcs[70];
  mProcs[70] = (UINT_PTR)&midiOutGetErrorTextW_fake;
#endif
#ifdef MIDIOUTGETID
  midiOutGetID_real = (midiOutGetID_ptr)mProcs[71];
  mProcs[71] = (UINT_PTR)&midiOutGetID_fake;
#endif
#ifdef MIDIOUTGETNUMDEVS
  midiOutGetNumDevs_real = (midiOutGetNumDevs_ptr)mProcs[72];
  mProcs[72] = (UINT_PTR)&midiOutGetNumDevs_fake;
#endif
#ifdef MIDIOUTGETVOLUME
  midiOutGetVolume_real = (midiOutGetVolume_ptr)mProcs[73];
  mProcs[73] = (UINT_PTR)&midiOutGetVolume_fake;
#endif
#ifdef MIDIOUTLONGMSG
  midiOutLongMsg_real = (midiOutLongMsg_ptr)mProcs[74];
  mProcs[74] = (UINT_PTR)&midiOutLongMsg_fake;
#endif
#ifdef MIDIOUTMESSAGE
  midiOutMessage_real = (midiOutMessage_ptr)mProcs[75];
  mProcs[75] = (UINT_PTR)&midiOutMessage_fake;
#endif
#ifdef MIDIOUTOPEN
  midiOutOpen_real = (midiOutOpen_ptr)mProcs[76];
  mProcs[76] = (UINT_PTR)&midiOutOpen_fake;
#endif
#ifdef MIDIOUTPREPAREHEADER
  midiOutPrepareHeader_real = (midiOutPrepareHeader_ptr)mProcs[77];
  mProcs[77] = (UINT_PTR)&midiOutPrepareHeader_fake;
#endif
#ifdef MIDIOUTRESET
  midiOutReset_real = (midiOutReset_ptr)mProcs[78];
  mProcs[78] = (UINT_PTR)&midiOutReset_fake;
#endif
#ifdef MIDIOUTSETVOLUME
  midiOutSetVolume_real = (midiOutSetVolume_ptr)mProcs[79];
  mProcs[79] = (UINT_PTR)&midiOutSetVolume_fake;
#endif
#ifdef MIDIOUTSHORTMSG
  midiOutShortMsg_real = (midiOutShortMsg_ptr)mProcs[80];
  mProcs[80] = (UINT_PTR)&midiOutShortMsg_fake;
#endif
#ifdef MIDIOUTUNPREPAREHEADER
  midiOutUnprepareHeader_real = (midiOutUnprepareHeader_ptr)mProcs[81];
  mProcs[81] = (UINT_PTR)&midiOutUnprepareHeader_fake;
#endif
#ifdef MIDISTREAMCLOSE
  midiStreamClose_real = (midiStreamClose_ptr)mProcs[82];
  mProcs[82] = (UINT_PTR)&midiStreamClose_fake;
#endif
#ifdef MIDISTREAMOPEN
  midiStreamOpen_real = (midiStreamOpen_ptr)mProcs[83];
  mProcs[83] = (UINT_PTR)&midiStreamOpen_fake;
#endif
#ifdef MIDISTREAMOUT
  midiStreamOut_real = (midiStreamOut_ptr)mProcs[84];
  mProcs[84] = (UINT_PTR)&midiStreamOut_fake;
#endif
#ifdef MIDISTREAMPAUSE
  midiStreamPause_real = (midiStreamPause_ptr)mProcs[85];
  mProcs[85] = (UINT_PTR)&midiStreamPause_fake;
#endif
#ifdef MIDISTREAMPOSITION
  midiStreamPosition_real = (midiStreamPosition_ptr)mProcs[86];
  mProcs[86] = (UINT_PTR)&midiStreamPosition_fake;
#endif
#ifdef MIDISTREAMPROPERTY
  midiStreamProperty_real = (midiStreamProperty_ptr)mProcs[87];
  mProcs[87] = (UINT_PTR)&midiStreamProperty_fake;
#endif
#ifdef MIDISTREAMRESTART
  midiStreamRestart_real = (midiStreamRestart_ptr)mProcs[88];
  mProcs[88] = (UINT_PTR)&midiStreamRestart_fake;
#endif
#ifdef MIDISTREAMSTOP
  midiStreamStop_real = (midiStreamStop_ptr)mProcs[89];
  mProcs[89] = (UINT_PTR)&midiStreamStop_fake;
#endif
#ifdef MIXERCLOSE
  mixerClose_real = (mixerClose_ptr)mProcs[90];
  mProcs[90] = (UINT_PTR)&mixerClose_fake;
#endif
#ifdef MIXERGETCONTROLDETAILSA
  mixerGetControlDetailsA_real = (mixerGetControlDetailsA_ptr)mProcs[91];
  mProcs[91] = (UINT_PTR)&mixerGetControlDetailsA_fake;
#endif
#ifdef MIXERGETCONTROLDETAILSW
  mixerGetControlDetailsW_real = (mixerGetControlDetailsW_ptr)mProcs[92];
  mProcs[92] = (UINT_PTR)&mixerGetControlDetailsW_fake;
#endif
#ifdef MIXERGETDEVCAPSA
  mixerGetDevCapsA_real = (mixerGetDevCapsA_ptr)mProcs[93];
  mProcs[93] = (UINT_PTR)&mixerGetDevCapsA_fake;
#endif
#ifdef MIXERGETDEVCAPSW
  mixerGetDevCapsW_real = (mixerGetDevCapsW_ptr)mProcs[94];
  mProcs[94] = (UINT_PTR)&mixerGetDevCapsW_fake;
#endif
#ifdef MIXERGETID
  mixerGetID_real = (mixerGetID_ptr)mProcs[95];
  mProcs[95] = (UINT_PTR)&mixerGetID_fake;
#endif
#ifdef MIXERGETLINECONTROLSA
  mixerGetLineControlsA_real = (mixerGetLineControlsA_ptr)mProcs[96];
  mProcs[96] = (UINT_PTR)&mixerGetLineControlsA_fake;
#endif
#ifdef MIXERGETLINECONTROLSW
  mixerGetLineControlsW_real = (mixerGetLineControlsW_ptr)mProcs[97];
  mProcs[97] = (UINT_PTR)&mixerGetLineControlsW_fake;
#endif
#ifdef MIXERGETLINEINFOA
  mixerGetLineInfoA_real = (mixerGetLineInfoA_ptr)mProcs[98];
  mProcs[98] = (UINT_PTR)&mixerGetLineInfoA_fake;
#endif
#ifdef MIXERGETLINEINFOW
  mixerGetLineInfoW_real = (mixerGetLineInfoW_ptr)mProcs[99];
  mProcs[99] = (UINT_PTR)&mixerGetLineInfoW_fake;
#endif
#ifdef MIXERGETNUMDEVS
  mixerGetNumDevs_real = (mixerGetNumDevs_ptr)mProcs[100];
  mProcs[100] = (UINT_PTR)&mixerGetNumDevs_fake;
#endif
#ifdef MIXERMESSAGE
  mixerMessage_real = (mixerMessage_ptr)mProcs[101];
  mProcs[101] = (UINT_PTR)&mixerMessage_fake;
#endif
#ifdef MIXEROPEN
  mixerOpen_real = (mixerOpen_ptr)mProcs[102];
  mProcs[102] = (UINT_PTR)&mixerOpen_fake;
#endif
#ifdef MIXERSETCONTROLDETAILS
  mixerSetControlDetails_real = (mixerSetControlDetails_ptr)mProcs[103];
  mProcs[103] = (UINT_PTR)&mixerSetControlDetails_fake;
#endif
#ifdef MMDRVINSTALL
  mmDrvInstall_real = (mmDrvInstall_ptr)mProcs[104];
  mProcs[104] = (UINT_PTR)&mmDrvInstall_fake;
#endif
#ifdef MMGETCURRENTTASK
  mmGetCurrentTask_real = (mmGetCurrentTask_ptr)mProcs[105];
  mProcs[105] = (UINT_PTR)&mmGetCurrentTask_fake;
#endif
#ifdef MMTASKBLOCK
  mmTaskBlock_real = (mmTaskBlock_ptr)mProcs[106];
  mProcs[106] = (UINT_PTR)&mmTaskBlock_fake;
#endif
#ifdef MMTASKCREATE
  mmTaskCreate_real = (mmTaskCreate_ptr)mProcs[107];
  mProcs[107] = (UINT_PTR)&mmTaskCreate_fake;
#endif
#ifdef MMTASKSIGNAL
  mmTaskSignal_real = (mmTaskSignal_ptr)mProcs[108];
  mProcs[108] = (UINT_PTR)&mmTaskSignal_fake;
#endif
#ifdef MMTASKYIELD
  mmTaskYield_real = (mmTaskYield_ptr)mProcs[109];
  mProcs[109] = (UINT_PTR)&mmTaskYield_fake;
#endif
#ifdef MMIOADVANCE
  mmioAdvance_real = (mmioAdvance_ptr)mProcs[110];
  mProcs[110] = (UINT_PTR)&mmioAdvance_fake;
#endif
#ifdef MMIOASCEND
  mmioAscend_real = (mmioAscend_ptr)mProcs[111];
  mProcs[111] = (UINT_PTR)&mmioAscend_fake;
#endif
#ifdef MMIOCLOSE
  mmioClose_real = (mmioClose_ptr)mProcs[112];
  mProcs[112] = (UINT_PTR)&mmioClose_fake;
#endif
#ifdef MMIOCREATECHUNK
  mmioCreateChunk_real = (mmioCreateChunk_ptr)mProcs[113];
  mProcs[113] = (UINT_PTR)&mmioCreateChunk_fake;
#endif
#ifdef MMIODESCEND
  mmioDescend_real = (mmioDescend_ptr)mProcs[114];
  mProcs[114] = (UINT_PTR)&mmioDescend_fake;
#endif
#ifdef MMIOFLUSH
  mmioFlush_real = (mmioFlush_ptr)mProcs[115];
  mProcs[115] = (UINT_PTR)&mmioFlush_fake;
#endif
#ifdef MMIOGETINFO
  mmioGetInfo_real = (mmioGetInfo_ptr)mProcs[116];
  mProcs[116] = (UINT_PTR)&mmioGetInfo_fake;
#endif
#ifdef MMIOINSTALLIOPROCA
  mmioInstallIOProcA_real = (mmioInstallIOProcA_ptr)mProcs[117];
  mProcs[117] = (UINT_PTR)&mmioInstallIOProcA_fake;
#endif
#ifdef MMIOINSTALLIOPROCW
  mmioInstallIOProcW_real = (mmioInstallIOProcW_ptr)mProcs[118];
  mProcs[118] = (UINT_PTR)&mmioInstallIOProcW_fake;
#endif
#ifdef MMIOOPENA
  mmioOpenA_real = (mmioOpenA_ptr)mProcs[119];
  mProcs[119] = (UINT_PTR)&mmioOpenA_fake;
#endif
#ifdef MMIOOPENW
  mmioOpenW_real = (mmioOpenW_ptr)mProcs[120];
  mProcs[120] = (UINT_PTR)&mmioOpenW_fake;
#endif
#ifdef MMIOREAD
  mmioRead_real = (mmioRead_ptr)mProcs[121];
  mProcs[121] = (UINT_PTR)&mmioRead_fake;
#endif
#ifdef MMIORENAMEA
  mmioRenameA_real = (mmioRenameA_ptr)mProcs[122];
  mProcs[122] = (UINT_PTR)&mmioRenameA_fake;
#endif
#ifdef MMIORENAMEW
  mmioRenameW_real = (mmioRenameW_ptr)mProcs[123];
  mProcs[123] = (UINT_PTR)&mmioRenameW_fake;
#endif
#ifdef MMIOSEEK
  mmioSeek_real = (mmioSeek_ptr)mProcs[124];
  mProcs[124] = (UINT_PTR)&mmioSeek_fake;
#endif
#ifdef MMIOSENDMESSAGE
  mmioSendMessage_real = (mmioSendMessage_ptr)mProcs[125];
  mProcs[125] = (UINT_PTR)&mmioSendMessage_fake;
#endif
#ifdef MMIOSETBUFFER
  mmioSetBuffer_real = (mmioSetBuffer_ptr)mProcs[126];
  mProcs[126] = (UINT_PTR)&mmioSetBuffer_fake;
#endif
#ifdef MMIOSETINFO
  mmioSetInfo_real = (mmioSetInfo_ptr)mProcs[127];
  mProcs[127] = (UINT_PTR)&mmioSetInfo_fake;
#endif
#ifdef MMIOSTRINGTOFOURCCA
  mmioStringToFOURCCA_real = (mmioStringToFOURCCA_ptr)mProcs[128];
  mProcs[128] = (UINT_PTR)&mmioStringToFOURCCA_fake;
#endif
#ifdef MMIOSTRINGTOFOURCCW
  mmioStringToFOURCCW_real = (mmioStringToFOURCCW_ptr)mProcs[129];
  mProcs[129] = (UINT_PTR)&mmioStringToFOURCCW_fake;
#endif
#ifdef MMIOWRITE
  mmioWrite_real = (mmioWrite_ptr)mProcs[130];
  mProcs[130] = (UINT_PTR)&mmioWrite_fake;
#endif
#ifdef MMSYSTEMGETVERSION
  mmsystemGetVersion_real = (mmsystemGetVersion_ptr)mProcs[131];
  mProcs[131] = (UINT_PTR)&mmsystemGetVersion_fake;
#endif
#ifdef SNDPLAYSOUNDA
  sndPlaySoundA_real = (sndPlaySoundA_ptr)mProcs[132];
  mProcs[132] = (UINT_PTR)&sndPlaySoundA_fake;
#endif
#ifdef SNDPLAYSOUNDW
  sndPlaySoundW_real = (sndPlaySoundW_ptr)mProcs[133];
  mProcs[133] = (UINT_PTR)&sndPlaySoundW_fake;
#endif
#ifdef TIMEBEGINPERIOD
  timeBeginPeriod_real = (timeBeginPeriod_ptr)mProcs[134];
  mProcs[134] = (UINT_PTR)&timeBeginPeriod_fake;
#endif
#ifdef TIMEENDPERIOD
  timeEndPeriod_real = (timeEndPeriod_ptr)mProcs[135];
  mProcs[135] = (UINT_PTR)&timeEndPeriod_fake;
#endif
#ifdef TIMEGETDEVCAPS
  timeGetDevCaps_real = (timeGetDevCaps_ptr)mProcs[136];
  mProcs[136] = (UINT_PTR)&timeGetDevCaps_fake;
#endif
#ifdef TIMEGETSYSTEMTIME
  timeGetSystemTime_real = (timeGetSystemTime_ptr)mProcs[137];
  mProcs[137] = (UINT_PTR)&timeGetSystemTime_fake;
#endif
#ifdef TIMEGETTIME
  timeGetTime_real = (timeGetTime_ptr)mProcs[138];
  mProcs[138] = (UINT_PTR)&timeGetTime_fake;
#endif
#ifdef TIMEKILLEVENT
  timeKillEvent_real = (timeKillEvent_ptr)mProcs[139];
  mProcs[139] = (UINT_PTR)&timeKillEvent_fake;
#endif
#ifdef TIMESETEVENT
  timeSetEvent_real = (timeSetEvent_ptr)mProcs[140];
  mProcs[140] = (UINT_PTR)&timeSetEvent_fake;
#endif
#ifdef WAVEINADDBUFFER
  waveInAddBuffer_real = (waveInAddBuffer_ptr)mProcs[141];
  mProcs[141] = (UINT_PTR)&waveInAddBuffer_fake;
#endif
#ifdef WAVEINCLOSE
  waveInClose_real = (waveInClose_ptr)mProcs[142];
  mProcs[142] = (UINT_PTR)&waveInClose_fake;
#endif
#ifdef WAVEINGETDEVCAPSA
  waveInGetDevCapsA_real = (waveInGetDevCapsA_ptr)mProcs[143];
  mProcs[143] = (UINT_PTR)&waveInGetDevCapsA_fake;
#endif
#ifdef WAVEINGETDEVCAPSW
  waveInGetDevCapsW_real = (waveInGetDevCapsW_ptr)mProcs[144];
  mProcs[144] = (UINT_PTR)&waveInGetDevCapsW_fake;
#endif
#ifdef WAVEINGETERRORTEXTA
  waveInGetErrorTextA_real = (waveInGetErrorTextA_ptr)mProcs[145];
  mProcs[145] = (UINT_PTR)&waveInGetErrorTextA_fake;
#endif
#ifdef WAVEINGETERRORTEXTW
  waveInGetErrorTextW_real = (waveInGetErrorTextW_ptr)mProcs[146];
  mProcs[146] = (UINT_PTR)&waveInGetErrorTextW_fake;
#endif
#ifdef WAVEINGETID
  waveInGetID_real = (waveInGetID_ptr)mProcs[147];
  mProcs[147] = (UINT_PTR)&waveInGetID_fake;
#endif
#ifdef WAVEINGETNUMDEVS
  waveInGetNumDevs_real = (waveInGetNumDevs_ptr)mProcs[148];
  mProcs[148] = (UINT_PTR)&waveInGetNumDevs_fake;
#endif
#ifdef WAVEINGETPOSITION
  waveInGetPosition_real = (waveInGetPosition_ptr)mProcs[149];
  mProcs[149] = (UINT_PTR)&waveInGetPosition_fake;
#endif
#ifdef WAVEINMESSAGE
  waveInMessage_real = (waveInMessage_ptr)mProcs[150];
  mProcs[150] = (UINT_PTR)&waveInMessage_fake;
#endif
#ifdef WAVEINOPEN
  waveInOpen_real = (waveInOpen_ptr)mProcs[151];
  mProcs[151] = (UINT_PTR)&waveInOpen_fake;
#endif
#ifdef WAVEINPREPAREHEADER
  waveInPrepareHeader_real = (waveInPrepareHeader_ptr)mProcs[152];
  mProcs[152] = (UINT_PTR)&waveInPrepareHeader_fake;
#endif
#ifdef WAVEINRESET
  waveInReset_real = (waveInReset_ptr)mProcs[153];
  mProcs[153] = (UINT_PTR)&waveInReset_fake;
#endif
#ifdef WAVEINSTART
  waveInStart_real = (waveInStart_ptr)mProcs[154];
  mProcs[154] = (UINT_PTR)&waveInStart_fake;
#endif
#ifdef WAVEINSTOP
  waveInStop_real = (waveInStop_ptr)mProcs[155];
  mProcs[155] = (UINT_PTR)&waveInStop_fake;
#endif
#ifdef WAVEINUNPREPAREHEADER
  waveInUnprepareHeader_real = (waveInUnprepareHeader_ptr)mProcs[156];
  mProcs[156] = (UINT_PTR)&waveInUnprepareHeader_fake;
#endif
#ifdef WAVEOUTBREAKLOOP
  waveOutBreakLoop_real = (waveOutBreakLoop_ptr)mProcs[157];
  mProcs[157] = (UINT_PTR)&waveOutBreakLoop_fake;
#endif
#ifdef WAVEOUTCLOSE
  waveOutClose_real = (waveOutClose_ptr)mProcs[158];
  mProcs[158] = (UINT_PTR)&waveOutClose_fake;
#endif
#ifdef WAVEOUTGETDEVCAPSA
  waveOutGetDevCapsA_real = (waveOutGetDevCapsA_ptr)mProcs[159];
  mProcs[159] = (UINT_PTR)&waveOutGetDevCapsA_fake;
#endif
#ifdef WAVEOUTGETDEVCAPSW
  waveOutGetDevCapsW_real = (waveOutGetDevCapsW_ptr)mProcs[160];
  mProcs[160] = (UINT_PTR)&waveOutGetDevCapsW_fake;
#endif
#ifdef WAVEOUTGETERRORTEXTA
  waveOutGetErrorTextA_real = (waveOutGetErrorTextA_ptr)mProcs[161];
  mProcs[161] = (UINT_PTR)&waveOutGetErrorTextA_fake;
#endif
#ifdef WAVEOUTGETERRORTEXTW
  waveOutGetErrorTextW_real = (waveOutGetErrorTextW_ptr)mProcs[162];
  mProcs[162] = (UINT_PTR)&waveOutGetErrorTextW_fake;
#endif
#ifdef WAVEOUTGETID
  waveOutGetID_real = (waveOutGetID_ptr)mProcs[163];
  mProcs[163] = (UINT_PTR)&waveOutGetID_fake;
#endif
#ifdef WAVEOUTGETNUMDEVS
  waveOutGetNumDevs_real = (waveOutGetNumDevs_ptr)mProcs[164];
  mProcs[164] = (UINT_PTR)&waveOutGetNumDevs_fake;
#endif
#ifdef WAVEOUTGETPITCH
  waveOutGetPitch_real = (waveOutGetPitch_ptr)mProcs[165];
  mProcs[165] = (UINT_PTR)&waveOutGetPitch_fake;
#endif
#ifdef WAVEOUTGETPLAYBACKRATE
  waveOutGetPlaybackRate_real = (waveOutGetPlaybackRate_ptr)mProcs[166];
  mProcs[166] = (UINT_PTR)&waveOutGetPlaybackRate_fake;
#endif
#ifdef WAVEOUTGETPOSITION
  waveOutGetPosition_real = (waveOutGetPosition_ptr)mProcs[167];
  mProcs[167] = (UINT_PTR)&waveOutGetPosition_fake;
#endif
#ifdef WAVEOUTGETVOLUME
  waveOutGetVolume_real = (waveOutGetVolume_ptr)mProcs[168];
  mProcs[168] = (UINT_PTR)&waveOutGetVolume_fake;
#endif
#ifdef WAVEOUTMESSAGE
  waveOutMessage_real = (waveOutMessage_ptr)mProcs[169];
  mProcs[169] = (UINT_PTR)&waveOutMessage_fake;
#endif
#ifdef WAVEOUTOPEN
  waveOutOpen_real = (waveOutOpen_ptr)mProcs[170];
  mProcs[170] = (UINT_PTR)&waveOutOpen_fake;
#endif
#ifdef WAVEOUTPAUSE
  waveOutPause_real = (waveOutPause_ptr)mProcs[171];
  mProcs[171] = (UINT_PTR)&waveOutPause_fake;
#endif
#ifdef WAVEOUTPREPAREHEADER
  waveOutPrepareHeader_real = (waveOutPrepareHeader_ptr)mProcs[172];
  mProcs[172] = (UINT_PTR)&waveOutPrepareHeader_fake;
#endif
#ifdef WAVEOUTRESET
  waveOutReset_real = (waveOutReset_ptr)mProcs[173];
  mProcs[173] = (UINT_PTR)&waveOutReset_fake;
#endif
#ifdef WAVEOUTRESTART
  waveOutRestart_real = (waveOutRestart_ptr)mProcs[174];
  mProcs[174] = (UINT_PTR)&waveOutRestart_fake;
#endif
#ifdef WAVEOUTSETPITCH
  waveOutSetPitch_real = (waveOutSetPitch_ptr)mProcs[175];
  mProcs[175] = (UINT_PTR)&waveOutSetPitch_fake;
#endif
#ifdef WAVEOUTSETPLAYBACKRATE
  waveOutSetPlaybackRate_real = (waveOutSetPlaybackRate_ptr)mProcs[176];
  mProcs[176] = (UINT_PTR)&waveOutSetPlaybackRate_fake;
#endif
#ifdef WAVEOUTSETVOLUME
  waveOutSetVolume_real = (waveOutSetVolume_ptr)mProcs[177];
  mProcs[177] = (UINT_PTR)&waveOutSetVolume_fake;
#endif
#ifdef WAVEOUTUNPREPAREHEADER
  waveOutUnprepareHeader_real = (waveOutUnprepareHeader_ptr)mProcs[178];
  mProcs[178] = (UINT_PTR)&waveOutUnprepareHeader_fake;
#endif
#ifdef WAVEOUTWRITE
  waveOutWrite_real = (waveOutWrite_ptr)mProcs[179];
  mProcs[179] = (UINT_PTR)&waveOutWrite_fake;
#endif
#ifdef EXPORTBYORDINAL2
  ExportByOrdinal2_real = (ExportByOrdinal2_ptr)mProcs[180];
  mProcs[180] = (UINT_PTR)&ExportByOrdinal2_fake;
#endif
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  mHinst = hinstDLL;
  if (fdwReason == DLL_PROCESS_ATTACH) {
    mHinstDLL = LoadLibrary("C:\\Windows\\System32\\winmm.dll");
    if (!mHinstDLL) {
      return FALSE;
    }
    for (int i = 0; i < 181; ++i) {
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

extern "C" void CloseDriver_wrapper();
extern "C" void DefDriverProc_wrapper();
extern "C" void DriverCallback_wrapper();
extern "C" void DrvGetModuleHandle_wrapper();
extern "C" void GetDriverModuleHandle_wrapper();
extern "C" void OpenDriver_wrapper();
extern "C" void PlaySound_wrapper();
extern "C" void PlaySoundA_wrapper();
extern "C" void PlaySoundW_wrapper();
extern "C" void SendDriverMessage_wrapper();
extern "C" void WOWAppExit_wrapper();
extern "C" void auxGetDevCapsA_wrapper();
extern "C" void auxGetDevCapsW_wrapper();
extern "C" void auxGetNumDevs_wrapper();
extern "C" void auxGetVolume_wrapper();
extern "C" void auxOutMessage_wrapper();
extern "C" void auxSetVolume_wrapper();
extern "C" void joyConfigChanged_wrapper();
extern "C" void joyGetDevCapsA_wrapper();
extern "C" void joyGetDevCapsW_wrapper();
extern "C" void joyGetNumDevs_wrapper();
extern "C" void joyGetPos_wrapper();
extern "C" void joyGetPosEx_wrapper();
extern "C" void joyGetThreshold_wrapper();
extern "C" void joyReleaseCapture_wrapper();
extern "C" void joySetCapture_wrapper();
extern "C" void joySetThreshold_wrapper();
extern "C" void mciDriverNotify_wrapper();
extern "C" void mciDriverYield_wrapper();
extern "C" void mciExecute_wrapper();
extern "C" void mciFreeCommandResource_wrapper();
extern "C" void mciGetCreatorTask_wrapper();
extern "C" void mciGetDeviceIDA_wrapper();
extern "C" void mciGetDeviceIDFromElementIDA_wrapper();
extern "C" void mciGetDeviceIDFromElementIDW_wrapper();
extern "C" void mciGetDeviceIDW_wrapper();
extern "C" void mciGetDriverData_wrapper();
extern "C" void mciGetErrorStringA_wrapper();
extern "C" void mciGetErrorStringW_wrapper();
extern "C" void mciGetYieldProc_wrapper();
extern "C" void mciLoadCommandResource_wrapper();
extern "C" void mciSendCommandA_wrapper();
extern "C" void mciSendCommandW_wrapper();
extern "C" void mciSendStringA_wrapper();
extern "C" void mciSendStringW_wrapper();
extern "C" void mciSetDriverData_wrapper();
extern "C" void mciSetYieldProc_wrapper();
extern "C" void midiConnect_wrapper();
extern "C" void midiDisconnect_wrapper();
extern "C" void midiInAddBuffer_wrapper();
extern "C" void midiInClose_wrapper();
extern "C" void midiInGetDevCapsA_wrapper();
extern "C" void midiInGetDevCapsW_wrapper();
extern "C" void midiInGetErrorTextA_wrapper();
extern "C" void midiInGetErrorTextW_wrapper();
extern "C" void midiInGetID_wrapper();
extern "C" void midiInGetNumDevs_wrapper();
extern "C" void midiInMessage_wrapper();
extern "C" void midiInOpen_wrapper();
extern "C" void midiInPrepareHeader_wrapper();
extern "C" void midiInReset_wrapper();
extern "C" void midiInStart_wrapper();
extern "C" void midiInStop_wrapper();
extern "C" void midiInUnprepareHeader_wrapper();
extern "C" void midiOutCacheDrumPatches_wrapper();
extern "C" void midiOutCachePatches_wrapper();
extern "C" void midiOutClose_wrapper();
extern "C" void midiOutGetDevCapsA_wrapper();
extern "C" void midiOutGetDevCapsW_wrapper();
extern "C" void midiOutGetErrorTextA_wrapper();
extern "C" void midiOutGetErrorTextW_wrapper();
extern "C" void midiOutGetID_wrapper();
extern "C" void midiOutGetNumDevs_wrapper();
extern "C" void midiOutGetVolume_wrapper();
extern "C" void midiOutLongMsg_wrapper();
extern "C" void midiOutMessage_wrapper();
extern "C" void midiOutOpen_wrapper();
extern "C" void midiOutPrepareHeader_wrapper();
extern "C" void midiOutReset_wrapper();
extern "C" void midiOutSetVolume_wrapper();
extern "C" void midiOutShortMsg_wrapper();
extern "C" void midiOutUnprepareHeader_wrapper();
extern "C" void midiStreamClose_wrapper();
extern "C" void midiStreamOpen_wrapper();
extern "C" void midiStreamOut_wrapper();
extern "C" void midiStreamPause_wrapper();
extern "C" void midiStreamPosition_wrapper();
extern "C" void midiStreamProperty_wrapper();
extern "C" void midiStreamRestart_wrapper();
extern "C" void midiStreamStop_wrapper();
extern "C" void mixerClose_wrapper();
extern "C" void mixerGetControlDetailsA_wrapper();
extern "C" void mixerGetControlDetailsW_wrapper();
extern "C" void mixerGetDevCapsA_wrapper();
extern "C" void mixerGetDevCapsW_wrapper();
extern "C" void mixerGetID_wrapper();
extern "C" void mixerGetLineControlsA_wrapper();
extern "C" void mixerGetLineControlsW_wrapper();
extern "C" void mixerGetLineInfoA_wrapper();
extern "C" void mixerGetLineInfoW_wrapper();
extern "C" void mixerGetNumDevs_wrapper();
extern "C" void mixerMessage_wrapper();
extern "C" void mixerOpen_wrapper();
extern "C" void mixerSetControlDetails_wrapper();
extern "C" void mmDrvInstall_wrapper();
extern "C" void mmGetCurrentTask_wrapper();
extern "C" void mmTaskBlock_wrapper();
extern "C" void mmTaskCreate_wrapper();
extern "C" void mmTaskSignal_wrapper();
extern "C" void mmTaskYield_wrapper();
extern "C" void mmioAdvance_wrapper();
extern "C" void mmioAscend_wrapper();
extern "C" void mmioClose_wrapper();
extern "C" void mmioCreateChunk_wrapper();
extern "C" void mmioDescend_wrapper();
extern "C" void mmioFlush_wrapper();
extern "C" void mmioGetInfo_wrapper();
extern "C" void mmioInstallIOProcA_wrapper();
extern "C" void mmioInstallIOProcW_wrapper();
extern "C" void mmioOpenA_wrapper();
extern "C" void mmioOpenW_wrapper();
extern "C" void mmioRead_wrapper();
extern "C" void mmioRenameA_wrapper();
extern "C" void mmioRenameW_wrapper();
extern "C" void mmioSeek_wrapper();
extern "C" void mmioSendMessage_wrapper();
extern "C" void mmioSetBuffer_wrapper();
extern "C" void mmioSetInfo_wrapper();
extern "C" void mmioStringToFOURCCA_wrapper();
extern "C" void mmioStringToFOURCCW_wrapper();
extern "C" void mmioWrite_wrapper();
extern "C" void mmsystemGetVersion_wrapper();
extern "C" void sndPlaySoundA_wrapper();
extern "C" void sndPlaySoundW_wrapper();
extern "C" void timeBeginPeriod_wrapper();
extern "C" void timeEndPeriod_wrapper();
extern "C" void timeGetDevCaps_wrapper();
extern "C" void timeGetSystemTime_wrapper();
extern "C" void timeGetTime_wrapper();
extern "C" void timeKillEvent_wrapper();
extern "C" void timeSetEvent_wrapper();
extern "C" void waveInAddBuffer_wrapper();
extern "C" void waveInClose_wrapper();
extern "C" void waveInGetDevCapsA_wrapper();
extern "C" void waveInGetDevCapsW_wrapper();
extern "C" void waveInGetErrorTextA_wrapper();
extern "C" void waveInGetErrorTextW_wrapper();
extern "C" void waveInGetID_wrapper();
extern "C" void waveInGetNumDevs_wrapper();
extern "C" void waveInGetPosition_wrapper();
extern "C" void waveInMessage_wrapper();
extern "C" void waveInOpen_wrapper();
extern "C" void waveInPrepareHeader_wrapper();
extern "C" void waveInReset_wrapper();
extern "C" void waveInStart_wrapper();
extern "C" void waveInStop_wrapper();
extern "C" void waveInUnprepareHeader_wrapper();
extern "C" void waveOutBreakLoop_wrapper();
extern "C" void waveOutClose_wrapper();
extern "C" void waveOutGetDevCapsA_wrapper();
extern "C" void waveOutGetDevCapsW_wrapper();
extern "C" void waveOutGetErrorTextA_wrapper();
extern "C" void waveOutGetErrorTextW_wrapper();
extern "C" void waveOutGetID_wrapper();
extern "C" void waveOutGetNumDevs_wrapper();
extern "C" void waveOutGetPitch_wrapper();
extern "C" void waveOutGetPlaybackRate_wrapper();
extern "C" void waveOutGetPosition_wrapper();
extern "C" void waveOutGetVolume_wrapper();
extern "C" void waveOutMessage_wrapper();
extern "C" void waveOutOpen_wrapper();
extern "C" void waveOutPause_wrapper();
extern "C" void waveOutPrepareHeader_wrapper();
extern "C" void waveOutReset_wrapper();
extern "C" void waveOutRestart_wrapper();
extern "C" void waveOutSetPitch_wrapper();
extern "C" void waveOutSetPlaybackRate_wrapper();
extern "C" void waveOutSetVolume_wrapper();
extern "C" void waveOutUnprepareHeader_wrapper();
extern "C" void waveOutWrite_wrapper();
extern "C" void ExportByOrdinal2();
