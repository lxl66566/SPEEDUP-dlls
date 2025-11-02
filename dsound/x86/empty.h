#ifndef _EMPTY_H_
#define _EMPTY_H_

#include <atomic>

// --- 代理 IDirectSoundBuffer8 ---
class MyDirectSoundBuffer8 : public IDirectSoundBuffer8 {
private:
  IDirectSoundBuffer8 *m_pReal;
  std::atomic<ULONG> m_nRef;

public:
  MyDirectSoundBuffer8(IDirectSoundBuffer8 *pReal) : m_pReal(pReal), m_nRef(1) {
    DWORD originalFreq;
    if (SUCCEEDED(m_pReal->GetFrequency(&originalFreq))) {
      DWORD newFrequency =
          static_cast<DWORD>(static_cast<float>(originalFreq) * SPEEDUP);
      if (newFrequency < DSBFREQUENCY_MIN)
        newFrequency = DSBFREQUENCY_MIN;
      if (newFrequency > DSBFREQUENCY_MAX)
        newFrequency = DSBFREQUENCY_MAX;
      m_pReal->SetFrequency(newFrequency);
    }
  }

  virtual ~MyDirectSoundBuffer8() {
    if (m_pReal)
      m_pReal->Release();
  }

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) {
    if (riid == IID_IUnknown || riid == IID_IDirectSoundBuffer ||
        riid == IID_IDirectSoundBuffer8) {
      *ppvObject = this;
      AddRef();
      return S_OK;
    }
    *ppvObject = NULL;
    return m_pReal->QueryInterface(riid, ppvObject);
  }
  STDMETHOD_(ULONG, AddRef)() { return ++m_nRef; }
  STDMETHOD_(ULONG, Release)() {
    ULONG ulRef = --m_nRef;
    if (ulRef == 0)
      delete this;
    return ulRef;
  }

  // IDirectSoundBuffer
  STDMETHOD(GetCaps)(LPDSBCAPS pDSBufferCaps) {
    return m_pReal->GetCaps(pDSBufferCaps);
  }
  STDMETHOD(GetCurrentPosition)(LPDWORD pdwCurrentPlayCursor,
                                LPDWORD pdwCurrentWriteCursor) {
    return m_pReal->GetCurrentPosition(pdwCurrentPlayCursor,
                                       pdwCurrentWriteCursor);
  }
  STDMETHOD(GetFormat)(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated,
                       LPDWORD pdwSizeWritten) {
    return m_pReal->GetFormat(pwfxFormat, dwSizeAllocated, pdwSizeWritten);
  }
  STDMETHOD(GetVolume)(LPLONG plVolume) { return m_pReal->GetVolume(plVolume); }
  STDMETHOD(GetPan)(LPLONG plPan) { return m_pReal->GetPan(plPan); }
  STDMETHOD(GetFrequency)(LPDWORD pdwFrequency) {
    HRESULT hr = m_pReal->GetFrequency(pdwFrequency);
    if (SUCCEEDED(hr) && pdwFrequency) {
      *pdwFrequency =
          static_cast<DWORD>(static_cast<float>(*pdwFrequency) / SPEEDUP);
    }
    return hr;
  }
  STDMETHOD(GetStatus)(LPDWORD pdwStatus) {
    return m_pReal->GetStatus(pdwStatus);
  }
  STDMETHOD(Initialize)(LPDIRECTSOUND pDirectSound,
                        LPCDSBUFFERDESC pcDSBufferDesc) {
    return m_pReal->Initialize(pDirectSound, pcDSBufferDesc);
  }
  STDMETHOD(Lock)(DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1,
                  LPDWORD pdwAudioBytes1, LPVOID *ppvAudioPtr2,
                  LPDWORD pdwAudioBytes2, DWORD dwFlags) {
    return m_pReal->Lock(dwOffset, dwBytes, ppvAudioPtr1, pdwAudioBytes1,
                         ppvAudioPtr2, pdwAudioBytes2, dwFlags);
  }
  STDMETHOD(Play)(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags) {
    return m_pReal->Play(dwReserved1, dwPriority, dwFlags);
  }
  STDMETHOD(SetCurrentPosition)(DWORD dwNewPosition) {
    return m_pReal->SetCurrentPosition(dwNewPosition);
  }
  STDMETHOD(SetFormat)(LPCWAVEFORMATEX pcfxFormat) {
    return m_pReal->SetFormat(pcfxFormat);
  }
  STDMETHOD(SetVolume)(LONG lVolume) { return m_pReal->SetVolume(lVolume); }
  STDMETHOD(SetPan)(LONG lPan) { return m_pReal->SetPan(lPan); }
  STDMETHOD(Stop)() { return m_pReal->Stop(); }
  STDMETHOD(Unlock)(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2,
                    DWORD dwAudioBytes2) {
    return m_pReal->Unlock(pvAudioPtr1, dwAudioBytes1, pvAudioPtr2,
                           dwAudioBytes2);
  }
  STDMETHOD(Restore)() { return m_pReal->Restore(); }

  // *** MODIFIED METHOD ***
  STDMETHOD(SetFrequency)(DWORD dwFrequency) {
    DWORD newFrequency =
        static_cast<DWORD>(static_cast<float>(dwFrequency) * SPEEDUP);
    if (newFrequency < DSBFREQUENCY_MIN)
      newFrequency = DSBFREQUENCY_MIN;
    if (newFrequency > DSBFREQUENCY_MAX)
      newFrequency = DSBFREQUENCY_MAX;
    return m_pReal->SetFrequency(newFrequency);
  }

  // IDirectSoundBuffer8
  STDMETHOD(SetFX)(DWORD dwEffectsCount, LPDSEFFECTDESC pdsfxDesc,
                   LPDWORD pdwResultCodes) {
    return m_pReal->SetFX(dwEffectsCount, pdsfxDesc, pdwResultCodes);
  }
  STDMETHOD(AcquireResources)(DWORD dwFlags, DWORD dwEffectsCount,
                              LPDWORD pdwResultCodes) {
    return m_pReal->AcquireResources(dwFlags, dwEffectsCount, pdwResultCodes);
  }
  STDMETHOD(GetObjectInPath)(REFGUID rguidObject, DWORD dwIndex,
                             REFGUID rguidInterface, LPVOID *ppObject) {
    return m_pReal->GetObjectInPath(rguidObject, dwIndex, rguidInterface,
                                    ppObject);
  }
};

// --- 代理 IDirectSound8 ---
class MyDirectSound8 : public IDirectSound8 {
private:
  IDirectSound8 *m_pReal;
  std::atomic<ULONG> m_nRef;

public:
  MyDirectSound8(IDirectSound8 *pReal) : m_pReal(pReal), m_nRef(1) {}
  virtual ~MyDirectSound8() {
    if (m_pReal)
      m_pReal->Release();
  }

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) {
    if (riid == IID_IUnknown || riid == IID_IDirectSound ||
        riid == IID_IDirectSound8) {
      *ppvObject = this;
      AddRef();
      return S_OK;
    }
    *ppvObject = NULL;
    return m_pReal->QueryInterface(riid, ppvObject);
  }
  STDMETHOD_(ULONG, AddRef)() { return ++m_nRef; }
  STDMETHOD_(ULONG, Release)() {
    ULONG ulRef = --m_nRef;
    if (ulRef == 0)
      delete this;
    return ulRef;
  }

  // IDirectSound
  STDMETHOD(GetCaps)(LPDSCAPS pDSCaps) { return m_pReal->GetCaps(pDSCaps); }
  STDMETHOD(DuplicateSoundBuffer)(LPDIRECTSOUNDBUFFER pDSBufferOriginal,
                                  LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate) {
    return m_pReal->DuplicateSoundBuffer(pDSBufferOriginal,
                                         ppDSBufferDuplicate);
  }
  STDMETHOD(SetCooperativeLevel)(HWND hwnd, DWORD dwLevel) {
    return m_pReal->SetCooperativeLevel(hwnd, dwLevel);
  }
  STDMETHOD(Compact)() { return m_pReal->Compact(); }
  STDMETHOD(GetSpeakerConfig)(LPDWORD pdwSpeakerConfig) {
    return m_pReal->GetSpeakerConfig(pdwSpeakerConfig);
  }
  STDMETHOD(SetSpeakerConfig)(DWORD dwSpeakerConfig) {
    return m_pReal->SetSpeakerConfig(dwSpeakerConfig);
  }
  STDMETHOD(Initialize)(LPCGUID pcGuidDevice) {
    return m_pReal->Initialize(pcGuidDevice);
  }

  // *** MODIFIED METHOD ***
  STDMETHOD(CreateSoundBuffer)(LPCDSBUFFERDESC pcDSBufferDesc,
                               LPDIRECTSOUNDBUFFER *ppDSBuffer,
                               LPUNKNOWN pUnkOuter) {
    HRESULT hr =
        m_pReal->CreateSoundBuffer(pcDSBufferDesc, ppDSBuffer, pUnkOuter);
    if (SUCCEEDED(hr) && ppDSBuffer && *ppDSBuffer) {
      IDirectSoundBuffer8 *pRealBuffer8 = NULL;
      if (SUCCEEDED((*ppDSBuffer)
                        ->QueryInterface(IID_IDirectSoundBuffer8,
                                         (void **)&pRealBuffer8)) &&
          pRealBuffer8) {
        MyDirectSoundBuffer8 *pMyBuffer =
            new MyDirectSoundBuffer8(pRealBuffer8);
        (*ppDSBuffer)->Release();
        *ppDSBuffer = (IDirectSoundBuffer *)pMyBuffer;
      }
    }
    return hr;
  }

  // IDirectSound8
  STDMETHOD(VerifyCertification)(LPDWORD pdwCertified) {
    return m_pReal->VerifyCertification(pdwCertified);
  }
};

// --- 代理 IDirectSound (旧版接口) ---
class MyDirectSound : public IDirectSound {
private:
  IDirectSound *m_pReal;
  std::atomic<ULONG> m_nRef;

public:
  MyDirectSound(IDirectSound *pReal) : m_pReal(pReal), m_nRef(1) {}
  virtual ~MyDirectSound() {
    if (m_pReal)
      m_pReal->Release();
  }

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) {
    if (riid == IID_IUnknown || riid == IID_IDirectSound) {
      *ppvObject = this;
      AddRef();
      return S_OK;
    }
    // 如果游戏请求更新的接口，我们也代理它
    if (riid == IID_IDirectSound8) {
      IDirectSound8 *pReal8 = NULL;
      HRESULT hr = m_pReal->QueryInterface(riid, (void **)&pReal8);
      if (SUCCEEDED(hr) && pReal8) {
        *ppvObject = new MyDirectSound8(pReal8);
        return S_OK;
      }
    }
    *ppvObject = NULL;
    return m_pReal->QueryInterface(riid, ppvObject);
  }
  STDMETHOD_(ULONG, AddRef)() { return ++m_nRef; }
  STDMETHOD_(ULONG, Release)() {
    ULONG ulRef = --m_nRef;
    if (ulRef == 0)
      delete this;
    return ulRef;
  }

  // IDirectSound
  STDMETHOD(GetCaps)(LPDSCAPS pDSCaps) { return m_pReal->GetCaps(pDSCaps); }
  STDMETHOD(DuplicateSoundBuffer)(LPDIRECTSOUNDBUFFER pDSBufferOriginal,
                                  LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate) {
    return m_pReal->DuplicateSoundBuffer(pDSBufferOriginal,
                                         ppDSBufferDuplicate);
  }
  STDMETHOD(SetCooperativeLevel)(HWND hwnd, DWORD dwLevel) {
    return m_pReal->SetCooperativeLevel(hwnd, dwLevel);
  }
  STDMETHOD(Compact)() { return m_pReal->Compact(); }
  STDMETHOD(GetSpeakerConfig)(LPDWORD pdwSpeakerConfig) {
    return m_pReal->GetSpeakerConfig(pdwSpeakerConfig);
  }
  STDMETHOD(SetSpeakerConfig)(DWORD dwSpeakerConfig) {
    return m_pReal->SetSpeakerConfig(dwSpeakerConfig);
  }
  STDMETHOD(Initialize)(LPCGUID pcGuidDevice) {
    return m_pReal->Initialize(pcGuidDevice);
  }

  // *** MODIFIED METHOD ***
  STDMETHOD(CreateSoundBuffer)(LPCDSBUFFERDESC pcDSBufferDesc,
                               LPDIRECTSOUNDBUFFER *ppDSBuffer,
                               LPUNKNOWN pUnkOuter) {
    HRESULT hr =
        m_pReal->CreateSoundBuffer(pcDSBufferDesc, ppDSBuffer, pUnkOuter);
    if (SUCCEEDED(hr) && ppDSBuffer && *ppDSBuffer) {
      IDirectSoundBuffer8 *pRealBuffer8 = NULL;
      if (SUCCEEDED((*ppDSBuffer)
                        ->QueryInterface(IID_IDirectSoundBuffer8,
                                         (void **)&pRealBuffer8)) &&
          pRealBuffer8) {
        MyDirectSoundBuffer8 *pMyBuffer =
            new MyDirectSoundBuffer8(pRealBuffer8);
        (*ppDSBuffer)->Release();
        *ppDSBuffer = (IDirectSoundBuffer *)pMyBuffer;
      }
    }
    return hr;
  }
};

// --- 挂钩函数实现 ---

#define DIRECTSOUNDCREATE
FAKE(HRESULT, WINAPI, DirectSoundCreate, LPCGUID pcGuidDevice,
     LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter) {
  HRESULT hr = DirectSoundCreate_real(pcGuidDevice, ppDS, pUnkOuter);
  if (SUCCEEDED(hr) && ppDS && *ppDS) {
    MyDirectSound *pMyDS = new MyDirectSound(*ppDS);
    *ppDS = pMyDS;
  }
  return hr;
}

#define DIRECTSOUNDCREATE8
FAKE(HRESULT, WINAPI, DirectSoundCreate8, LPCGUID pcGuidDevice,
     LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter) {
  HRESULT hr = DirectSoundCreate8_real(pcGuidDevice, ppDS8, pUnkOuter);
  if (SUCCEEDED(hr) && ppDS8 && *ppDS8) {
    MyDirectSound8 *pMyDS8 = new MyDirectSound8(*ppDS8);
    *ppDS8 = pMyDS8;
  }
  return hr;
}

#endif // _EMPTY_H_