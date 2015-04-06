#include "Sound.h"
#include "Usefull.h"
#include "Globals.h"

const std::string directory = "sounds\\";

ALuint TSoundsManager::GetFromName(char *Name, bool Dynamic,
                                                           float *fSamplingRate)
{ // wyszukanie dźwięku w pamięci albo wczytanie z pliku
    AnsiString file;
    if (Dynamic)
    { // próba wczytania z katalogu pojazdu
        file = Global::asCurrentDynamicPath + AnsiString(Name);
        if (FileExists(file))
            Name = file.c_str(); // nowa nazwa
        else
            Dynamic = false; // wczytanie z "sounds/"
    }
    TSoundContainer *Next = First;
    DWORD dwStatus;
    for (int i = 0; i < Count; i++)
    {
        if (strcmp(Name, Next->Name) == 0)
        {
            if (fSamplingRate)
                *fSamplingRate = Next->fSamplingRate; // częstotliwość
            return (Next->GetUnique(pDS));
            //      DSBuffers.
            /*
                Next->pDSBuffer[Next->Oldest]->Stop();
                Next->pDSBuffer[Next->Oldest]->SetCurrentPosition(0);
                if (Next->Oldest<Next->Concurrent-1)
                {
                    Next->Oldest++;
                    return (Next->pDSBuffer[Next->Oldest-1]);
                }
                else
                {
                    Next->Oldest= 0;
                    return (Next->pDSBuffer[Next->Concurrent-1]);
                };

       /*         for (int j=0; j<Next->Concurrent; j++)
                {

                    Next->pDSBuffer[j]->GetStatus(&dwStatus);
                    if ((dwStatus & DSBSTATUS_PLAYING) != DSBSTATUS_PLAYING)
                        return (Next->pDSBuffer[j]);
                }                                   */
        }
        else
            Next = Next->Next;
    };
    if (Dynamic) // wczytanie z katalogu pojazdu
        Next = LoadFromFile("", Name, 1);
    else
        Next = LoadFromFile(Directory, Name, 1);
    if (Next)
    { //
        if (fSamplingRate)
            *fSamplingRate = Next->fSamplingRate; // częstotliwość
        return Next->GetUnique(pDS);
    }
    ErrorLog("Missed sound: " + AnsiString(Name));
    return (NULL);
};

void TSoundsManager::RestoreAll()
{
    TSoundContainer *Next = First;

    HRESULT hr;

    DWORD dwStatus;

    for (int i = 0; i < Count; i++)
    {

        if (FAILED(hr = Next->DSBuffer->GetStatus(&dwStatus)))
            ;
        //        return hr;

        if (dwStatus & DSBSTATUS_BUFFERLOST)
        {
            // Since the app could have just been activated, then
            // DirectSound may not be giving us control yet, so
            // the restoring the buffer may fail.
            // If it does, sleep until DirectSound gives us control.
            do
            {
                hr = Next->DSBuffer->Restore();
                if (hr == DSERR_BUFFERLOST)
                    Sleep(10);
            } while ((hr = Next->DSBuffer->Restore()) != NULL);

            //          char *Name= Next->Name;
            //          int cc= Next->Concurrent;
            //          delete Next;
            //          Next= new TSoundContainer(pDS, Directory, Name, cc);
            //          if( FAILED( hr = FillBuffer() ) );
            //           return hr;
        };
        Next = Next->Next;
    };
};

// void TSoundsManager::Init(char *Name, int Concurrent)
//__fastcall TSoundsManager::TSoundsManager(HWND hWnd)
// void TSoundsManager::Init(HWND hWnd, char *NDirectory)

void TSoundsManager::Init(HWND hWnd)
{

    First = NULL;
    Count = 0;
    pDS = NULL;
    pDSNotify = NULL;

    HRESULT hr; //=222;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    //    strcpy(Directory, NDirectory);

    // Create IDirectSound using the primary sound device
    hr = DirectSoundCreate(NULL, &pDS, NULL);
    if (hr != DS_OK)
    {

        if (hr == DSERR_ALLOCATED)
            return;
        if (hr == DSERR_INVALIDPARAM)
            return;
        if (hr == DSERR_NOAGGREGATION)
            return;
        if (hr == DSERR_NODRIVER)
            return;
        if (hr == DSERR_OUTOFMEMORY)
            return;

        //  hr=0;
    };
    //    return ;

    // Set coop level to DSSCL_PRIORITY
    //    if( FAILED( hr = pDS->SetCooperativeLevel( hWnd, DSSCL_PRIORITY ) ) );
    //      return ;
    pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);

    // Get the primary buffer
    DSBUFFERDESC dsbd;
    ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    if (!Global::bInactivePause) // jeśli przełączony w tło ma nadal działać
        dsbd.dwFlags |= DSBCAPS_GLOBALFOCUS; // to dźwięki mają być również słyszalne
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat = NULL;

    if (FAILED(hr = pDS->CreateSoundBuffer(&dsbd, &pDSBPrimary, NULL)))
        return;

    // Set primary buffer format to 22kHz and 16-bit output.
    WAVEFORMATEX wfx;
    ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (FAILED(hr = pDSBPrimary->SetFormat(&wfx)))
        return;

    SAFE_RELEASE(pDSBPrimary);
};

//---------------------------------------------------------------------------
#pragma package(smart_init)
