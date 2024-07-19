#pragma once
#include <windows.h>



/*
[???]
1.Should i call: 
WSASend, WSARecv 
or
BOOL bSuccess = PostQueuedCompletionStatus(m_hCompletionPort, 
                       pOverlapBuff->GetUsed(), 
                       (DWORD) pContext, &pOverlapBuff->m_ol);

For this WSABUF is needed

2.



*/

HANDLE IOCP_handle = CreateIoCompletionPort( INVALID_HANDLE_VALUE,NULL, NULL, NULL);
if(!IOCP_handle)
{
GetLastError();
}
CompletionKey=1;
if(!CreateIoCompletionPort( SOCKET1,IOCP_handle, CompletionKey++, NULL))
{
GetLastError();
}
 

if(!CreateIoCompletionPort( SOCKET2,IOCP_handle, CompletionKey++, NULL))
{
GetLastError();
}
 
/*  Can be related to many threads */
if(!GetQueuedCompletionStatus(IOCP_handle, &lpNumberOfBytesTransferred,&lpCompletionKey,&lpOverlapped , INFINITE))
{
    GetLastError();
}
 

WSARecv

WSASend
int WSAAPI WSASend(
  [in]  SOCKET                             s,
  [in]  LPWSABUF                           lpBuffers,
  [in]  DWORD                              dwBufferCount,
  [out] LPDWORD                            lpNumberOfBytesSent,
  [in]  DWORD                              dwFlags,
  [in]  LPWSAOVERLAPPED                    lpOverlapped,
  [in]  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef struct _WSAOVERLAPPED {
  DWORD    Internal;
  DWORD    InternalHigh;
  DWORD    Offset;
  DWORD    OffsetHigh;
  WSAEVENT hEvent;
} WSAOVERLAPPED, *LPWSAOVERLAPPED;
 
/* probably cause that GetQueuedCompletionStatus wakes up with (completionKey == 0 && lpOverlapped == NULL) */
PostQueuedCompletionStatus(hCompletionPort, 0, 0, NULL);




CloseHandle(IOCP_handle);