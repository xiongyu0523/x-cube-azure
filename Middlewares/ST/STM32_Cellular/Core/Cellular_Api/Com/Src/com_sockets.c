/**
  ******************************************************************************
  * @file    com_sockets.c
  * @author  MCD Application Team
  * @brief   This file implements Communication Socket
  *          main entry module Com sockets
  *          route to MODEM or LwIP services
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
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
#include "com_sockets.h"
#include "plf_config.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#include "com_sockets_ip_modem.h"
#else /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
#include "com_sockets_lwip_mcu.h"
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */

#include "com_sockets_statistic.h"
#include "cellular_service_os.h"

#if (USE_CMD_CONSOLE == 1)
#include <string.h>
#include "cmd.h"
#endif /* USE_CMD_CONSOLE == 1 */

/* Private defines -----------------------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintForce(format, args...) \
  TracePrintForce(DBG_CHAN_COM, DBL_LVL_P0, format "\n\r", ## args)

#else /* USE_PRINTF == 1U */
#include <stdio.h>
#define PrintForce(format, args...)  \
  printf(format "\n\r", ## args);
#endif /* USE_PRINTF == 0U */
#endif /* USE_CMD_CONSOLE == 1 */


/* Private typedef -----------------------------------------------------------*/
typedef char COM_CHAR_t; /* used in stdio.h and string.h service call */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
#if (USE_CMD_CONSOLE == 1)
static cmd_status_t com_sockets_cmd(uint8_t *cmd_line_p);
#endif /* USE_CMD_CONSOLE == 1 */

/* Private function Definition -----------------------------------------------*/
/**
  * @brief  console cmd management
  * @param  cmd_line_p - command line
  * @note   command parameters
  * @retval None
  */
#if (USE_CMD_CONSOLE == 1)

static void com_help_cmd(void)
{
  CMD_print_help((uint8_t *)"com");
  PrintForce("comsocket help")
  PrintForce("comsocket stat\n\r")
}

static cmd_status_t com_sockets_cmd(uint8_t *cmd_line_p)
{
  uint32_t argc;
  uint8_t  *argv_p[10];
  const uint8_t *cmd_p;

  PrintForce("")
  cmd_p = (uint8_t *)strtok((COM_CHAR_t *)cmd_line_p, " \t");

  if (strncmp((const COM_CHAR_t *)cmd_p,
              "comsocket",
              strlen((const COM_CHAR_t *)cmd_p))
      == 0)
  {
    /* parameters parsing */
    for (argc = 0U; argc < 10U; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        break;
      }
    }

    if (argc == 0U)
    {
      com_help_cmd();
    }

    /*  1st parameter analysis */
    else if (strncmp((COM_CHAR_t *)argv_p[0],
                     "help",
                     strlen((COM_CHAR_t *)argv_p[0]))
             == 0)
    {
    }
    else if (strncmp((COM_CHAR_t *)argv_p[0],
                     "stat",
                     strlen((COM_CHAR_t *)argv_p[0]))
             == 0)
    {
      //com_sockets_statistic_display();
    }
    else
    {
      PrintForce("comsocket: Unrecognized command. Usage:")
      com_help_cmd();
    }
  }
  return CMD_OK;
}
#endif /* USE_CMD_CONSOLE == 1 */

/* Functions Definition ------------------------------------------------------*/

/*** Socket management ********************************************************/

/**
  * @brief  Socket handle creation
  * @note   Create a communication endpoint called socket
  * @param  family   - address family
  * @param  type     - connection type
  * @param  protocol - protocol type
  * @retval int32_t  - socket handle or error value
  */
int32_t com_socket(int32_t family, int32_t type, int32_t protocol)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_socket_ip_modem(family, type, protocol);
#else
  return com_socket_lwip_mcu(family, type, protocol);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Socket option set
  * @note   Set option for the socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  level     - level at which the option is defined
  * @param  optname   - option name for which the value is to be set
  * @param  optval    - pointer to the buffer containing the option value
  * @param  optlen    - size of the buffer containing the option value
  * @retval int32_t   - ok or error value
  */
int32_t com_setsockopt(int32_t sock, int32_t level, int32_t optname,
                       const void *optval, int32_t optlen)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_setsockopt_ip_modem(sock, level, optname, optval, optlen);
#else
  return com_setsockopt_lwip_mcu(sock, level, optname, optval, optlen);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Socket option get
  * @note   Get option for a socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  level     - level at which option is defined
  * @param  optname   - option name for which the value is requested
  * @param  optval    - pointer to the buffer that will contain the option value
  * @param  optlen    - size of the buffer that will contain the option value
  * @retval int32_t   - ok or error value
  */
int32_t com_getsockopt(int32_t sock, int32_t level, int32_t optname,
                       void *optval, int32_t *optlen)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_getsockopt_ip_modem(sock, level, optname, optval, optlen);
#else
  return com_getsockopt_lwip_mcu(sock, level, optname, optval, optlen);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Socket bind
  * @note   Assign a local address and port to a socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - local IP address and port
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_bind(int32_t sock,
                 const com_sockaddr_t *addr, int32_t addrlen)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_bind_ip_modem(sock, addr, addrlen);
#else
  return com_bind_lwip_mcu(sock, addr, addrlen);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Socket close
  * @note   Close a socket and release socket handle
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @retval int32_t   - ok or error value
  */
int32_t com_closesocket(int32_t sock)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_closesocket_ip_modem(sock);
#else
  return com_closesocket_lwip_mcu(sock);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}


/*** Client functionalities ***************************************************/

/**
  * @brief  Socket connect
  * @note   Connect socket to a remote host
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - remote IP address and port
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_connect(int32_t sock,
                    const com_sockaddr_t *addr, int32_t addrlen)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_connect_ip_modem(sock, addr, addrlen);
#else
  return com_connect_lwip_mcu(sock, addr, addrlen);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Socket send data
  * @note   Send data on already connected socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to send
  * @param  len       - size of the data to send (in bytes)
  * @param  flags     - options
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_send(int32_t sock,
                 const com_char_t *buf, int32_t len,
                 int32_t flags)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_send_ip_modem(sock, buf, len, flags);
#else
  return com_send_lwip_mcu(sock, buf, len, flags);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Socket receive data
  * @note   Receive data on already connected socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to store the data to
  * @param  len       - size of application data buffer (in bytes)
  * @param  flags     - options
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recv(int32_t sock,
                 com_char_t *buf, int32_t len,
                 int32_t flags)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_recv_ip_modem(sock, buf, len, flags);
#else
  return com_recv_lwip_mcu(sock, buf, len, flags);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}


/*** Client - Server functionalities ******************************************/

/**
  * @brief  Socket send to data
  * @note   Send data to a remote host
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to send
  * @param  len       - length of the data to send (in bytes)
  * @param  flags     - options
  * @param  addr      - remote IP address and port number
  * @param  len       - addr length
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_sendto(int32_t sock,
                   const com_char_t *buf, int32_t len,
                   int32_t flags,
                   const com_sockaddr_t *to, int32_t tolen)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_sendto_ip_modem(sock, buf, len, flags, to, tolen);
#else
  return com_sendto_lwip_mcu(sock, buf, len, flags, to, tolen);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Socket receive from data
  * @note   Receive data from a remote host
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to store the data to
  * @param  len       - size of application data buffer (in bytes)
  * @param  flags     - options
  * @param  addr      - remote IP address and port number
  * @param  len       - addr length
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recvfrom(int32_t sock,
                     com_char_t *buf, int32_t len,
                     int32_t flags,
                     com_sockaddr_t *from, int32_t *fromlen)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_recvfrom_ip_modem(sock, buf, len, flags, from, fromlen);
#else
  return com_recvfrom_lwip_mcu(sock, buf, len, flags, from, fromlen);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}


/*** Server functionalities ***************************************************/

/**
  * @brief  Socket listen
  * @note   Set socket in listening mode
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  backlog   - number of connection requests that can be queued
  * @retval int32_t   - ok or error value
  */
int32_t com_listen(int32_t sock,
                   int32_t backlog)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_listen_ip_modem(sock, backlog);
#else
  return com_listen_lwip_mcu(sock, backlog);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Socket accept
  * @note   Accept a connect request for a listening socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - IP address and port number of the accepted connection
  * @param  len       - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_accept(int32_t sock,
                   com_sockaddr_t *addr, int32_t *addrlen)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_accept_ip_modem(sock, addr, addrlen);
#else
  return com_accept_lwip_mcu(sock, addr, addrlen);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}


/*** Other functionalities ****************************************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time and
            before using any other functions of com_*
  * @param  None
  * @retval bool      - true/false init ok/nok
  */
com_bool_t com_init(void)
{
  com_bool_t result;

  result = COM_SOCKETS_FALSE;
  if (osCDS_cellular_service_init()
      == CELLULAR_TRUE)
  {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    result = com_init_ip_modem();
#else
    result = com_init_lwip_mcu();
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
    //com_sockets_statistic_init();
  }

  return result;
}

/**
  * @brief  Component start
  * @note   must be called only one time but
            after com_init and dc_start
            and before using any other functions of com_*
  * @param  None
  * @retval None
  */
void com_start(void)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  com_start_ip_modem();
#else
  com_start_lwip_mcu();
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */

#if (USE_CMD_CONSOLE == 1)
  CMD_Declare((uint8_t *)"comsocket", com_sockets_cmd, (uint8_t *)"com socket commands");
#endif /* USE_CMD_CONSOLE == 1 */
}

/**
  * @brief  Get host IP from host name
  * @note   Retrieve host IP address from host name
  * @param  name      - host name
  * @param  addr      - host IP corresponding to host name
  * @retval int32_t   - ok or error value
  */
int32_t com_gethostbyname(const com_char_t *name,
                          com_sockaddr_t   *addr)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_gethostbyname_ip_modem(name, addr);
#else
  return com_gethostbyname_lwip_mcu(name, addr);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Get peer name
  * @note   Retrieve IP address and port number
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  name      - IP address and port number of the peer
  * @param  namelen   - name length
  * @retval int32_t   - ok or error value
  */
int32_t com_getpeername(int32_t sock,
                        com_sockaddr_t *name, int32_t *namelen)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_getpeername_ip_modem(sock, name, namelen);
#else
  return com_getpeername_lwip_mcu(sock, name, namelen);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Get sock name
  * @note   Retrieve local IP address and port number
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  name      - IP address and port number
  * @param  namelen   - name length
  * @retval int32_t   - ok or error value
  */
int32_t com_getsockname(int32_t sock,
                        com_sockaddr_t *name, int32_t *namelen)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_getsockname_ip_modem(sock, name, namelen);
#else
  return com_getsockname_lwip_mcu(sock, name, namelen);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

#if (USE_COM_PING == 1)
/**
  * @brief  Ping handle creation
  * @note   Create a ping session
  * @param  None
  * @retval int32_t  - ping handle or error value
  */
int32_t com_ping(void)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_ping_ip_modem();
#else
  return com_ping_lwip_mcu();
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Ping process request
  * @note   Create a ping session
  * @param  ping     - ping handle obtained with com_ping
  * @note   ping handle on which operation has to be done
  * @param  addr     - remote IP address and port
  * @param  addrlen  - addr length
  * @param  timeout  - timeout for ping response (in sec)
  * @param  rsp      - ping response
  * @retval int32_t  - ok or error value
  */
int32_t com_ping_process(int32_t ping,
                         const com_sockaddr_t *addr, int32_t addrlen,
                         uint8_t timeout, com_ping_rsp_t *rsp)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_ping_process_ip_modem(ping, addr, addrlen, timeout, rsp);
#else
  return com_ping_process_lwip_mcu(ping, addr, addrlen, timeout, rsp);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

/**
  * @brief  Ping close
  * @note   Close a ping session and release ping handle
  * @param  ping      - ping handle obtained with com_socket
  * @note   ping handle on which operation has to be done
  * @retval int32_t   - ok or error value
  */
int32_t com_closeping(int32_t ping)
{
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  return com_closeping_ip_modem(ping);
#else
  return com_closeping_lwip_mcu(ping);
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */
}

#endif /* USE_COM_PING == 1 */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
