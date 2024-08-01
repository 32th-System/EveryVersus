#include <Windows.h>

#include "pipe.h"


#if 0
HANDLE hPipe;
HANDLE hWriteSignal;

void PipeInit() {
    hWriteSignal = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    hPipe = CreateNamedPipeW(
        L"\\\\.\\pipe\\EveryVersus_pipe",
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_WAIT,
        1,
        2048, 2048,
        INFINITE,
        nullptr
    );

}
#endif