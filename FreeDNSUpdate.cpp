// Include all the libraries we need
#pragma comment (lib, "ws2_32.lib")

#ifndef UNICODE
#define UNICODE 1
#endif

#ifndef _UNICODE
#define _UNICODE 1
#endif

#define WIN32_LEAN_AND_MEAN 1

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

std::map<int, TCHAR *> g_wsaErrors;

void SetupErrorMap(void)
{
   g_wsaErrors[WSA_INVALID_HANDLE] = _T("Specified event object handle is invalid");
   g_wsaErrors[WSA_INVALID_PARAMETER] = _T("One or more parameters are invalid");
   g_wsaErrors[WSA_OPERATION_ABORTED] = _T("Overlapped operation aborted");
   g_wsaErrors[WSA_NOT_ENOUGH_MEMORY] = _T("There was insufficient memory to perform the operation");
   g_wsaErrors[WSAEAFNOSUPPORT] = _T("An address incompatible with the requested protocol was used");
   g_wsaErrors[WSAEINVAL] = _T("An invalid argument was supplied");
   g_wsaErrors[WSAESOCKTNOSUPPORT] = _T("The support for the specified socket type does not exist in this address family");
   g_wsaErrors[WSAEADDRNOTAVAIL] = _T("Cannot assign requested address");
   g_wsaErrors[WSAHOST_NOT_FOUND] = _T("No such host is known");
   g_wsaErrors[WSANOTINITIALISED] = _T("A successful WSAStartup call must occur before using this function");
   g_wsaErrors[WSATYPE_NOT_FOUND] = _T("The specified class was not found");
   g_wsaErrors[WSANO_DATA] = _T("The requested name is valid, but no data of the requested type was found");
   g_wsaErrors[WSANO_RECOVERY] = _T("A nonrecoverable error occurred during a database lookup");
}

void LogWSAError(FILE **logFile, int error)
{
   TCHAR *message;

   if (g_wsaErrors.count(error))
      message = g_wsaErrors[error];
   else
      message = _T("Unknown error");

   _ftprintf_s(*logFile, _T("%s (%d)\n"), message, error);
}

int WINAPI _tWinMain(HINSTANCE thisInstance, HINSTANCE prevInstance, LPTSTR commandLine, int showCode)
{
   char updateKey[1024];
   unsigned long ukLen;

   // Zero the memory
   memset(updateKey, 0, sizeof(updateKey));

   // Get the information from the INI file
   ukLen = GetPrivateProfileStringA("FREEDNS", "UKEY", "", updateKey, sizeof(updateKey), ".\\freedns.ini");

   // Create the log file
   FILE *logFile;

   _tfopen_s(&logFile, _T("freedns.log"), _T("a"));
   SetupErrorMap();
   _ftprintf_s(logFile, _T("Started DNS update\n"));

   if (strnlen_s(updateKey, sizeof(updateKey)) == 0)
   {
      _ftprintf_s(logFile, _T("No update key found in freedns.ini file\n"));
      exit(0);
   }

   if (ukLen == sizeof(updateKey))
   {
      _ftprintf_s(logFile, _T("WARNING: Update key is longer than the buffer. The end will be truncated.\n"));
      updateKey[sizeof(updateKey) - 1] = '\0';
   }

   WSADATA wsaData;
   struct addrinfo *result = NULL;

   WSAStartup(MAKEWORD(2, 2), &wsaData);

   if (getaddrinfo("freedns.afraid.org", "80", NULL, &result) != 0)
   {
      _ftprintf_s(logFile, _T("Failed to get address info for hostname: "));
      LogWSAError(&logFile, WSAGetLastError());
      WSACleanup();
      exit(0);
   }

   SOCKET sck = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

   _ftprintf_s(logFile, _T("Attempting to connect to http://freedns.afraid.org...\n"));

   if (connect(sck, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
   {
      _ftprintf_s(logFile, _T("Unable to connect to remote server: "));
      LogWSAError(&logFile, WSAGetLastError());
      WSACleanup();
      exit(0);
   }

   // We don't need this after we've connected
   freeaddrinfo(result);

   // A connection was established. Now set up the request.
   char request[1024];
   char response[1024];
   
   memset(request, 0, sizeof(request));
   memset(response, 0, sizeof(response));

   snprintf(request, sizeof(request), "GET http://freedns.afraid.org/dynamic/update.php?%s\r\n\r\n", updateKey);

   _ftprintf_s(logFile, _T("%S\n"), request);

   if (send(sck, request, strnlen_s(request, sizeof(request)), 0) == SOCKET_ERROR)
   {
      // NOTE(william): Should we have some sort of retry count?
      _ftprintf_s(logFile, _T("Failed to send request to remote server: "));
      LogWSAError(&logFile, WSAGetLastError());
      closesocket(sck);
      WSACleanup();
      exit(0);
   }

   int bytesReceived = recv(sck, response, sizeof(response), 0);

   if (bytesReceived == SOCKET_ERROR)
   {
      _ftprintf_s(logFile, _T("Failed to receive data from the remote server: "));
      LogWSAError(&logFile, WSAGetLastError());
      closesocket(sck);
      WSACleanup();
      exit(0);
   }

   char outBuf[sizeof(response) + 1];

   while (bytesReceived > 0)
   {
      memset(outBuf, 0, sizeof(outBuf));
      strncpy_s(outBuf, response, bytesReceived);
      fprintf_s(logFile, (const char *)outBuf);
      bytesReceived = recv(sck, response, sizeof(response), 0);
   }

   _ftprintf_s(logFile, _T("\n"));

   closesocket(sck);
   WSACleanup();
   fclose(logFile);

   return 0;
}