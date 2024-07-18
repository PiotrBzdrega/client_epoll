#pragma once

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
 
 
PostQueuedCompletionStatus
 
 
CloseHandle(IOCP_handle);