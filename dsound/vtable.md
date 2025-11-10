# VTables

## 基础接口

### IUnknown VTable

| 索引 | 方法             |
| :--- | :--------------- |
| 0    | `QueryInterface` |
| 1    | `AddRef`         |
| 2    | `Release`        |

### IReferenceClock VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `GetTime` |
| 4 | `AdviseTime` |
| 5 | `AdvisePeriodic` |
| 6 | `Unadvise` |

### IKsPropertySet VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `Get` |
| 4 | `Set` |
| 5 | `QuerySupport` |

---

## DirectSound 播放核心接口

### IDirectSound VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `CreateSoundBuffer` |
| 4 | `GetCaps` |
| 5 | `DuplicateSoundBuffer` |
| 6 | `SetCooperativeLevel` |
| 7 | `Compact` |
| 8 | `GetSpeakerConfig` |
| 9 | `SetSpeakerConfig` |
| 10 | `Initialize` |

### IDirectSound8 VTable

_继承自: `IDirectSound`_
| 索引 | 方法 |
| :--- | :--- |
| 0-10 | `IDirectSound` 方法 |
| 11 | `VerifyCertification` |

### IDirectSoundBuffer VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `GetCaps` |
| 4 | `GetCurrentPosition` |
| 5 | `GetFormat` |
| 6 | `GetVolume` |
| 7 | `GetPan` |
| 8 | `GetFrequency` |
| 9 | `GetStatus` |
| 10 | `Initialize` |
| 11 | `Lock` |
| 12 | `Play` |
| 13 | `SetCurrentPosition` |
| 14 | `SetFormat` |
| 15 | `SetVolume` |
| 16 | `SetPan` |
| 17 | `SetFrequency` |
| 18 | `Stop` |
| 19 | `Unlock` |
| 20 | `Restore` |

### IDirectSoundBuffer8 VTable

_继承自: `IDirectSoundBuffer`_
| 索引 | 方法 |
| :--- | :--- |
| 0-20 | `IDirectSoundBuffer` 方法 |
| 21 | `SetFX` |
| 22 | `AcquireResources` |
| 23 | `GetObjectInPath` |

### IDirectSoundNotify VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `SetNotificationPositions` |

---

## DirectSound 3D 音效接口

### IDirectSound3DListener VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `GetAllParameters` |
| 4 | `GetDistanceFactor` |
| 5 | `GetDopplerFactor` |
| 6 | `GetOrientation` |
| 7 | `GetPosition` |
| 8 | `GetRolloffFactor` |
| 9 | `GetVelocity` |
| 10 | `SetAllParameters` |
| 11 | `SetDistanceFactor` |
| 12 | `SetDopplerFactor` |
| 13 | `SetOrientation` |
| 14 | `SetPosition` |
| 15 | `SetRolloffFactor` |
| 16 | `SetVelocity` |
| 17 | `CommitDeferredSettings` |

### IDirectSound3DBuffer VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `GetAllParameters` |
| 4 | `GetConeAngles` |
| 5 | `GetConeOrientation` |
| 6 | `GetConeOutsideVolume` |
| 7 | `GetMaxDistance` |
| 8 | `GetMinDistance` |
| 9 | `GetMode` |
| 10 | `GetPosition` |
| 11 | `GetVelocity` |
| 12 | `SetAllParameters` |
| 13 | `SetConeAngles` |
| 14 | `SetConeOrientation` |
| 15 | `SetConeOutsideVolume` |
| 16 | `SetMaxDistance` |
| 17 | `SetMinDistance` |
| 18 | `SetMode` |
| 19 | `SetPosition` |
| 20 | `SetVelocity` |

---

## DirectSound 录音 (Capture) 接口

### IDirectSoundCapture VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `CreateCaptureBuffer` |
| 4 | `GetCaps` |
| 5 | `Initialize` |

### IDirectSoundCaptureBuffer VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `GetCaps` |
| 4 | `GetCurrentPosition` |
| 5 | `GetFormat` |
| 6 | `GetStatus` |
| 7 | `Initialize` |
| 8 | `Lock` |
| 9 | `Start` |
| 10 | `Stop` |
| 11 | `Unlock` |

### IDirectSoundCaptureBuffer8 VTable

_继承自: `IDirectSoundCaptureBuffer`_
| 索引 | 方法 |
| :--- | :--- |
| 0-11 | `IDirectSoundCaptureBuffer` 方法 |
| 12 | `GetObjectInPath` |
| 13 | `GetFXStatus` |

---

## DirectSound 全双工接口

### IDirectSoundFullDuplex VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `Initialize` |

---

## DirectSound 播放效果 (FX) 接口

所有这些效果接口都直接继承自 `IUnknown`。

### IDirectSoundFXGargle VTable

| 索引 | 方法               |
| :--- | :----------------- |
| 0-2  | `IUnknown` 方法    |
| 3    | `SetAllParameters` |
| 4    | `GetAllParameters` |

### IDirectSoundFXChorus VTable

| 索引 | 方法               |
| :--- | :----------------- |
| 0-2  | `IUnknown` 方法    |
| 3    | `SetAllParameters` |
| 4    | `GetAllParameters` |

### IDirectSoundFXFlanger VTable

| 索引 | 方法               |
| :--- | :----------------- |
| 0-2  | `IUnknown` 方法    |
| 3    | `SetAllParameters` |
| 4    | `GetAllParameters` |

### IDirectSoundFXEcho VTable

| 索引 | 方法               |
| :--- | :----------------- |
| 0-2  | `IUnknown` 方法    |
| 3    | `SetAllParameters` |
| 4    | `GetAllParameters` |

### IDirectSoundFXDistortion VTable

| 索引 | 方法               |
| :--- | :----------------- |
| 0-2  | `IUnknown` 方法    |
| 3    | `SetAllParameters` |
| 4    | `GetAllParameters` |

### IDirectSoundFXCompressor VTable

| 索引 | 方法               |
| :--- | :----------------- |
| 0-2  | `IUnknown` 方法    |
| 3    | `SetAllParameters` |
| 4    | `GetAllParameters` |

### IDirectSoundFXParamEq VTable

| 索引 | 方法               |
| :--- | :----------------- |
| 0-2  | `IUnknown` 方法    |
| 3    | `SetAllParameters` |
| 4    | `GetAllParameters` |

### IDirectSoundFXI3DL2Reverb VTable

| 索引 | 方法               |
| :--- | :----------------- |
| 0-2  | `IUnknown` 方法    |
| 3    | `SetAllParameters` |
| 4    | `GetAllParameters` |
| 5    | `SetPreset`        |
| 6    | `GetPreset`        |
| 7    | `SetQuality`       |
| 8    | `GetQuality`       |

### IDirectSoundFXWavesReverb VTable

| 索引 | 方法               |
| :--- | :----------------- |
| 0-2  | `IUnknown` 方法    |
| 3    | `SetAllParameters` |
| 4    | `GetAllParameters` |

---

## DirectSound 录音效果 (Capture FX) 接口

### IDirectSoundCaptureFXAec VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `SetAllParameters` |
| 4 | `GetAllParameters` |
| 5 | `GetStatus` |
| 6 | `Reset` |

### IDirectSoundCaptureFXNoiseSuppress VTable

_继承自: `IUnknown`_
| 索引 | 方法 |
| :--- | :--- |
| 0-2 | `IUnknown` 方法 |
| 3 | `SetAllParameters` |
| 4 | `GetAllParameters` |
| 5 | `Reset` |
