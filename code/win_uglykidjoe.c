#include<windows.h>
#include<stdbool.h>
#include<stdio.h>
#include<xinput.h>
#include<dsound.h>
#include<math.h>

#include "win_uglykidjoe.h"

X_INPUT_GET_STATE(XInputGetStateStub) 
{ 
    return (ERROR_DEVICE_NOT_CONNECTED); 
}
X_INPUT_SET_STATE(XInputSetStateStub) 
{ 
    return (ERROR_DEVICE_NOT_CONNECTED); 
}

global_variable bool GlobalRunning;
global_variable Win32_OffScreenBuffer GlobalBackBuffer;
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub; 
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
global_variable IDirectSoundBuffer *GlobalSecondaryBuffer;

internal void
Win32FillSoundBuffer(Win32_SoundOutput *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(IDirectSoundBuffer_Lock(GlobalSecondaryBuffer, ByteToLock, BytesToWrite,
					  &Region1, &Region1Size,
					  &Region2, &Region2Size, 
					  0)))
    {
	i16 *SamplesOut = (i16 *)Region1;
	DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
	for (DWORD SampleIndex = 0;
	     SampleIndex < Region1SampleCount;
	     ++SampleIndex)
	{
	    f32 SineValue = sinf(SoundOutput->TSine);
	    i16 SampleValue = (i16)(SineValue * SoundOutput->ToneVolume);
			    
	    *SamplesOut++ = SampleValue;
	    *SamplesOut++ = SampleValue;

	    SoundOutput->TSine += (2.0f * Pi32 * 1.0f) / (f32)SoundOutput->WavePeriod;
	    ++SoundOutput->RunningSampleIndex;
	}
		    
	SamplesOut = (i16 *)Region2;
	DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
	for (DWORD SampleIndex = 0;
	     SampleIndex < Region2SampleCount;
	     ++SampleIndex)
	{
	    f32 SineValue = sinf(SoundOutput->TSine);
	    i16 SampleValue = (i16)(SineValue * SoundOutput->ToneVolume);
			    
	    *SamplesOut++ = SampleValue;
	    *SamplesOut++ = SampleValue;
	    
	    SoundOutput->TSine += (2.0f * Pi32 * 1.0f) / (f32)SoundOutput->WavePeriod;
	    ++SoundOutput->RunningSampleIndex;
	}
    }
    IDirectSoundBuffer_Unlock(GlobalSecondaryBuffer, Region1, Region1Size, Region2, Region2Size);
}

internal void
Win32InitDSound(HWND Window, int SamplesPerSecond, int BufferSize)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if(DSoundLibrary)
    {
	direct_sound_create *DirectSoundCreate = (direct_sound_create*)
	    GetProcAddress(DSoundLibrary, "DirectSoundCreate");

	IDirectSound *DirectSound;
	if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
	{
	    WAVEFORMATEX WaveFormat = {0};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8; // 4 under current settings
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
	    
	    if (SUCCEEDED(IDirectSound_SetCooperativeLevel(DirectSound, Window, DSSCL_PRIORITY)))
	    {
		DSBUFFERDESC BufferDescription = {0};
		BufferDescription.dwSize = sizeof(BufferDescription);
		BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

		IDirectSoundBuffer *PrimaryBuffer; 
		if(SUCCEEDED(IDirectSound_CreateSoundBuffer(DirectSound, &BufferDescription, &PrimaryBuffer, 0)))
		{
		    if (SUCCEEDED(IDirectSoundBuffer_SetFormat(PrimaryBuffer, &WaveFormat)))
                    {
			OutputDebugStringA("Primary buffer format was set.\n");
		    }
		}
	    }
	    
	    DSBUFFERDESC BufferDescription = {0};
	    BufferDescription.dwSize = sizeof(BufferDescription);      
	    BufferDescription.dwBufferBytes = BufferSize;
	    BufferDescription.lpwfxFormat = &WaveFormat;
	    if(SUCCEEDED(IDirectSound_CreateSoundBuffer(DirectSound, &BufferDescription, &GlobalSecondaryBuffer, 0)))
	    {
		OutputDebugStringA("Secondary buffer created Successfully.\n");
	    }
	    else
	    {
		OutputDebugStringA("Secondary buffer not created\n");
	    }
	}
    }
}

internal void
Win32LoadXInput()
{
    HMODULE XInputLibrary = LoadLibraryA("Xinput1_4.dll");
    if(!XInputLibrary)
    {
	XInputLibrary = LoadLibraryA("Xinput1_3.dll");
    }
  
    if(!XInputLibrary)
    {
	XInputLibrary = LoadLibraryA("Xinput9_1_0.dll");
    }
  
    if(XInputLibrary)
    {
	XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
	if(!XInputGetState)
	{
	    XInputGetState = XInputGetStateStub;
	}
	XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	if(!XInputSetState)
	{
	    XInputSetState = XInputSetStateStub;
	}
    }
    else
    {
	XInputGetState = XInputGetStateStub;
	XInputSetState = XInputSetStateStub;
    }
}

internal Win32_WindowDimension
Win32GetWindowDimension(HWND Window)
{
    Win32_WindowDimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return(Result);
}

internal void
RenderGradient(Win32_OffScreenBuffer *Buffer,
	       int XOffset, int YOffset)
{
    u8 *Row = (u8 *)Buffer->Memory;
    for(int Y = 0;
	Y < Buffer->Height;
	++Y)
    {
	u32 *Pixel = (u32 *)Row;
	for(int X = 0;
	    X < Buffer->Width;
	    ++X)
	{
	    u8 Red = 0;
	    u8 Green = Y + YOffset;
	    u8 Blue = X + XOffset;
      
	    *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
	}
	Row += Buffer->Pitch;
    }  
}

internal void
Win32ResizeDIBSection(Win32_OffScreenBuffer *Buffer,
		      int Width, int Height)
{
    Buffer->BytesPerPixel = 4;
    if(Buffer->Memory)
    {
	VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }
    Buffer->Width = Width;
    Buffer->Height = Height;
  
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader); 
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

  
    int BitmapMemorySize = Buffer->BytesPerPixel * (Buffer->Width * Buffer->Height);
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width*Buffer->BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(Win32_OffScreenBuffer Buffer,
			   HDC DeviceContext, int WindowWidth,
			   int WindowHeight)
{
    StretchDIBits(DeviceContext, 
		  0, 0, WindowWidth, WindowHeight,
		  0, 0, Buffer.Width, Buffer.Height,
		  Buffer.Memory,
		  &Buffer.Info,
		  DIB_RGB_COLORS, SRCCOPY);    
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window, 
			UINT Message,
			WPARAM WParam,
			LPARAM LParam){
    LRESULT Result = 0;
    switch(Message)
    {
    case WM_DESTROY:
    {
	OutputDebugStringA("WM_DESTROY\n");
	GlobalRunning = false;
    } break;
    case WM_SIZE:
    {
    } break;
    case WM_CLOSE:
    {
	OutputDebugStringA("WM_CLOSE\n");
	GlobalRunning = false;
    } break;
    case WM_ACTIVATEAPP:
    {
    } break;
    
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        bool IsDown = ((LParam & (1 << 31)) == 0);
        bool WasDown = ((LParam & (1 << 30)) != 0);
	u32 VKCode = WParam;
	if(IsDown != WasDown)
	{
	    if (VKCode == 'W')
	    {
	    } 
	    else if (VKCode == 'A')
	    {

	    } 
	    else if (VKCode == 'S')
	    {
	
	    } 
	    else if (VKCode == 'D')
	    {
        
	    } 
	    else if (VKCode == 'Q')
	    {
		OutputDebugStringA("Q\n");
	    } 
	    else if (VKCode == 'E')
	    {
	    } 
	    else if (VKCode == VK_UP)
	    {
		
	    } 
	    else if (VKCode == VK_DOWN)
	    {
	    } 
	    else if (VKCode == VK_LEFT)
	    {
	    } 
	    else if (VKCode == VK_RIGHT)
	    {
	    } 
	    else if (VKCode == VK_ESCAPE)
	    {
	    } 
	    else if (VKCode == VK_SPACE)
	    {
	    }
	    else if (VKCode == VK_ESCAPE)
	    {
		OutputDebugStringA("ESCAPE: ");
		if (IsDown)
		{
		    OutputDebugStringA("IsDown ");
		}
		if (WasDown)
		{
		    OutputDebugStringA("WasDown");
		}
		OutputDebugStringA("\n");
	    }
	    
	    bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
	    if((VKCode == VK_F4) && AltKeyWasDown)
	    {
		GlobalRunning = false;
	    }
	}
      
    } break;
    
    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);
	
	Win32_WindowDimension Dimension = Win32GetWindowDimension(Window);
	Win32DisplayBufferInWindow(GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
				 
	EndPaint(Window, &Paint);
    } break;
    default:
    {
	Result = DefWindowProc(Window, Message, WParam, LParam);
    } break;
    }

    return(Result);
}

int CALLBACK
WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd)
{
    WNDCLASSA WindowClass = {0};
    Win32LoadXInput();
  
    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
    
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    i64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
  
    WindowClass.style = CS_HREDRAW | CS_VREDRAW ;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "Uglykidjoe";

    if(RegisterClassA(&WindowClass))
    {
	HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Uglykidjoe",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, Instance, 0);
	if(Window)
	{
	    HDC DeviceContext = GetDC(Window);
	    Win32_SoundOutput SoundOutput = {0};
	    
	    SoundOutput.SamplesPerSecond = 4800;
	    SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * sizeof(i16) * 2;
	    SoundOutput.BytesPerSample = sizeof(i16) * 2;
	    SoundOutput.RunningSampleIndex = 0;
	    SoundOutput.ToneHz = 256; 
	    SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
	    SoundOutput.ToneVolume = 30000;
	    SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
	    
	    Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
	    Win32FillSoundBuffer(&SoundOutput, 0,(SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample));
	    IDirectSoundBuffer_Play(GlobalSecondaryBuffer, 0, 0, DSBPLAY_LOOPING);
	    
	    int XOffset = 0;
	    int YOffset = 0;
	    GlobalRunning = true;
	    
	    LARGE_INTEGER LastCounter;
	    QueryPerformanceCounter(&LastCounter);
	    u64 LastCycleCount = __rdtsc();
	    
	    while(GlobalRunning)
	    {
		MSG Message;
		while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
		{
		    if(Message.message == WM_QUIT)
		    {
			GlobalRunning = false;
		    }
	   
		    TranslateMessage(&Message);
		    DispatchMessageA(&Message);
		}

		for (DWORD ControllerIndex = 0;
		     ControllerIndex < XUSER_MAX_COUNT;
		     ++ControllerIndex)
		{
		    XINPUT_STATE ControllerState;
		    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
		    {
			XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
                        
			bool Up            = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
			bool Down          = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
			bool Left          = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
			bool Right         = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
			bool Start         = Pad->wButtons & XINPUT_GAMEPAD_START;
			bool Back          = Pad->wButtons & XINPUT_GAMEPAD_BACK;
			bool LeftShoulder  = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
			bool RightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
			bool AButton       = Pad->wButtons & XINPUT_GAMEPAD_A;
			bool BButton       = Pad->wButtons & XINPUT_GAMEPAD_B;
			bool XButton       = Pad->wButtons & XINPUT_GAMEPAD_X;
			bool YButton       = Pad->wButtons & XINPUT_GAMEPAD_Y;

			i16 StickX = Pad->sThumbLX;
			i16 StickY = Pad->sThumbLY;

			if(AButton)
			{
			    SoundOutput.ToneHz += 10;
			    SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
			    YOffset += 2;
			}
			
			if(BButton)
			{
			    SoundOutput.ToneHz -= 10;
			    SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
			    YOffset -= 2;
			}
	       
			XOffset += StickX >> 12;
			YOffset += StickY >> 12;
			
			// SoundOutput.ToneHz = 512 + (int)(256.0f * ((f32)StickY / 30000.0f));
			// SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
		    }
		    else
		    {
			// Controller not plugged
		    }
		}
		RenderGradient(&GlobalBackBuffer, XOffset, YOffset);

		DWORD PlayCursor;
		DWORD WriteCursor;
		if(SUCCEEDED(IDirectSoundBuffer_GetCurrentPosition(GlobalSecondaryBuffer, &PlayCursor, &WriteCursor)))
		{
		    DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
		    DWORD BytesToWrite = 0;
		    DWORD TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample))
                                          % SoundOutput.SecondaryBufferSize);
		    
		    if(ByteToLock > TargetCursor)
		    {
			BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
			BytesToWrite += TargetCursor;
		    }
		    else
		    {
			BytesToWrite = TargetCursor - ByteToLock;
		    }
		    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);
		}
		Win32_WindowDimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
		
		LARGE_INTEGER EndCounter;
		QueryPerformanceCounter(&EndCounter);
		u64 EndCycleCount = __rdtsc();
		
		i64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
		i64 CyclesElapsed = EndCycleCount - LastCycleCount;
		
		f32 MSPerFrame = (f32)((1000*CounterElapsed) / (f32)PerfCountFrequency);
		f32 FPS = (f32)PerfCountFrequency / (f32)CounterElapsed;
		f32 MegaCyclesPerFrame = (f32)CyclesElapsed / (1000.0f * 1000.0f);
		
		char Buffer[256];
		sprintf(Buffer, "%.02fms/f, %.02ff/s, %.02fMc/f\n", MSPerFrame, FPS, MegaCyclesPerFrame);
		OutputDebugStringA(Buffer);
		
		LastCounter = EndCounter;
		LastCycleCount = EndCycleCount;
	    }
	}
	else
	{
	}
    }
    else
    {
    }
    return(0);
}
