/**
  ******************************************************************************
  * @file    tcpsocketconnection.c
  * @author  MCD Application Team
  * @brief   Azure mbed socket abstraction implementation.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>  /* Required by tcpsocketconnection_c.h */
#include "azure_c_shared_utility/tcpsocketconnection_c.h"
#include "net_connect.h"
#include "msg.h"
#include "main.h" // hnet
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define AZ_TSC_REMOVE_NOT_USED  /**< Remove the unused - and not properly implemented - functions. */

/* structure for adaptation between Azure mbed socket IO and ST Connectivity framework */
typedef struct
{
  int32_t socket;
} TCPSOCKETCONNECTION_CONNECTIVITY;

/* Exported functions --------------------------------------------------------*/
TCPSOCKETCONNECTION_HANDLE tcpsocketconnection_create(void)
{
  TCPSOCKETCONNECTION_CONNECTIVITY *psock = NULL;

  psock = (TCPSOCKETCONNECTION_CONNECTIVITY *)malloc(sizeof(TCPSOCKETCONNECTION_CONNECTIVITY));
  if (psock != NULL)
  {

    psock->socket = net_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
    if (psock->socket < 0)
    {
      free(psock);
      psock = NULL;
    }
  }
  return (TCPSOCKETCONNECTION_HANDLE)psock;
}

void tcpsocketconnection_set_blocking(TCPSOCKETCONNECTION_HANDLE tcpSocketConnectionHandle, bool blocking, unsigned int timeout)
{
  TCPSOCKETCONNECTION_CONNECTIVITY *psock = (TCPSOCKETCONNECTION_CONNECTIVITY *)(tcpSocketConnectionHandle);
  int32_t option_timeout = 0;
  int ret = NET_OK;
  
  if (blocking)
  {
    option_timeout = timeout;
  }
  else
  {
    option_timeout = 0;
  }
  
  ret = net_setsockopt(psock->socket, NET_SOL_SOCKET, NET_SO_RCVTIMEO, (void *)&option_timeout, sizeof(option_timeout));
  if ( ret != NET_OK)
  {
    msg_error("Error setting the socket receive timeout option (%d).\n", ret);
  }
  else
  {
    ret = net_setsockopt(psock->socket, NET_SOL_SOCKET, NET_SO_SNDTIMEO, (void *)&option_timeout, sizeof(option_timeout));
    if ( ret != NET_OK)
    {
      msg_error("Error setting the socket send timeout option (%d).\n", ret);
    }
  }
}

void tcpsocketconnection_destroy(TCPSOCKETCONNECTION_HANDLE tcpSocketConnectionHandle)
{
  free(tcpSocketConnectionHandle);
}

int tcpsocketconnection_connect(TCPSOCKETCONNECTION_HANDLE tcpSocketConnectionHandle, const char* host, const int port)
{
  TCPSOCKETCONNECTION_CONNECTIVITY *psock = (TCPSOCKETCONNECTION_CONNECTIVITY *)(tcpSocketConnectionHandle);
  sockaddr_in_t addr;
  int rc = NET_OK;
  
  addr.sin_len    = sizeof(sockaddr_in_t);

  rc = net_if_gethostbyname(NULL, (sockaddr_t *)&addr, (char_t *)host);
  if (rc != NET_OK)
  {
    msg_error("Could not find hostname ipaddr %s (%d)\n", host, rc);
  }
  else
  {
    addr.sin_port = NET_HTONS(port);
    rc = net_connect(psock->socket, (sockaddr_t *)&addr, sizeof(addr));
    if (rc != NET_OK)
    {
      msg_error("Could not connect to %s:%d (%d)\n", host, port, rc);
    }
  }
  
  return rc;
}

#ifndef AZ_TSC_REMOVE_NOT_USED
bool tcpsocketconnection_is_connected(TCPSOCKETCONNECTION_HANDLE tcpSocketConnectionHandle)
{
  // TODO: Need to extend the NET API?
  return true;
}
#endif /* AZ_TSC_REMOVE_NOT_USED */

void tcpsocketconnection_close(TCPSOCKETCONNECTION_HANDLE tcpSocketConnectionHandle)
{
  TCPSOCKETCONNECTION_CONNECTIVITY *psock = (TCPSOCKETCONNECTION_CONNECTIVITY *)(tcpSocketConnectionHandle);
  /* net_shutdown(sock, NET_SHUTDOWN_RW); */
  net_closesocket(psock->socket);
}

int tcpsocketconnection_send(TCPSOCKETCONNECTION_HANDLE tcpSocketConnectionHandle, const char* data, int length)
{
  // TODO: Check the return code
  TCPSOCKETCONNECTION_CONNECTIVITY *psock = (TCPSOCKETCONNECTION_CONNECTIVITY *)(tcpSocketConnectionHandle);
  return net_send(psock->socket, (uint8_t *)data, length, 0);
}

#ifndef AZ_TSC_REMOVE_NOT_USED
int tcpsocketconnection_send_all(TCPSOCKETCONNECTION_HANDLE tcpSocketConnectionHandle, const char* data, int length)
{
  // TODO: Check the return code
  // TODO: May need to loop until everything is sent, or the timeout is reached
  TCPSOCKETCONNECTION_CONNECTIVITY *psock = (TCPSOCKETCONNECTION_CONNECTIVITY *)(tcpSocketConnectionHandle);
  return net_send(psock->socket, (uint8_t *)data, length, 0);
}
#endif /* AZ_TSC_REMOVE_NOT_USED */

int tcpsocketconnection_receive(TCPSOCKETCONNECTION_HANDLE tcpSocketConnectionHandle, char* data, int length)
{
  // TODO: Check the return code
  TCPSOCKETCONNECTION_CONNECTIVITY *psock = (TCPSOCKETCONNECTION_CONNECTIVITY *)(tcpSocketConnectionHandle);
  return net_recv(psock->socket, (uint8_t *)data, length, 0);
}

#ifndef AZ_TSC_REMOVE_NOT_USED
int tcpsocketconnection_receive_all(TCPSOCKETCONNECTION_HANDLE tcpSocketConnectionHandle, char* data, int length)
{
  // TODO: Check the return code
  // TODO: May need to loop until everything is received, or the timeout is reached
  TCPSOCKETCONNECTION_CONNECTIVITY *psock = (TCPSOCKETCONNECTION_CONNECTIVITY *)(tcpSocketConnectionHandle);
  return net_recv(psock->socket, (uint8_t *)data, length, 0);
}
#endif /* AZ_TSC_REMOVE_NOT_USED */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
