/**
  ******************************************************************************
  * @file    com_sockets_ip_modem.c
  * @author  MCD Application Team
  * @brief   This file implements Socket IP on MODEM side
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
#include "com_sockets_ip_modem.h"
#include "plf_config.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "com_sockets_net_compat.h"
#include "com_sockets_err_compat.h"
#include "com_sockets_statistic.h"

#include "cellular_service_os.h"
#include "cellular_runtime_custom.h"

#if (USE_DATACACHE == 1)
#include "dc_common.h"
#include "dc_time.h"
#include "dc_cellular.h"
#endif /* USE_DATACACHE == 1 */

//#include "rng.h"

/* Private defines -----------------------------------------------------------*/
#if (USE_TRACE_COM_SOCKETS == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintINFO(format, args...) \
  TracePrint(DBG_CHAN_COM, DBL_LVL_P0, "COM: " format "\n\r", ## args)
#define PrintDBG(format, args...)  \
  TracePrint(DBG_CHAN_COM, DBL_LVL_P1, "COM: " format "\n\r", ## args)
#define PrintERR(format, args...)  \
  TracePrint(DBG_CHAN_COM, DBL_LVL_ERR, "COM ERROR: " format "\n\r", ## args)

#else /* USE_PRINTF == 1 */
#define PrintINFO(format, args...)  \
  printf("COM: " format "\n\r", ## args);
/* To reduce trace PrintDBG is deactivated when using printf */
#define PrintDBG(format, args...)   do {} while(0);
#define PrintERR(format, args...)   \
  printf("COM ERROR: " format "\n\r", ## args);

#endif /* USE_PRINTF == 0U */

#else /* USE_TRACE_COM_SOCKETS == 0U */
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintERR(format, args...)   do {} while(0);
#endif /* USE_TRACE_COM_SOCKETS == 1U */

/* Need a function in low level to obtain these values according to the modem */
/* Maximum data that can be passed between COM and low level */
#define MODEM_MAX_TX_DATA_SIZE CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE
#define MODEM_MAX_RX_DATA_SIZE CONFIG_MODEM_MAX_SOCKET_RX_DATA_SIZE

#define COM_SOCKET_INVALID_ID (-1)

/* Socket local id number : 1 for ping */
#define COM_SOCKET_LOCAL_ID_NB 1U

#define LOCAL_PORT_BEGIN  0xc000U /* 49152 */
#define LOCAL_PORT_END    0xffffU /* 65535 */

/* Private typedef -----------------------------------------------------------*/
typedef char CSIP_CHAR_t; /* used in stdio.h and string.h service call */

typedef uint16_t socket_msg_type_t;
#define SOCKET_MSG        (socket_msg_type_t)1    /* MSG is SOCKET type       */
#define PING_MSG          (socket_msg_type_t)2    /* MSG is PING type         */

typedef uint16_t socket_msg_id_t;
#define DATA_RCV          (socket_msg_id_t)1      /* MSG id is DATA_RCV       */
#define CLOSING_RCV       (socket_msg_id_t)2      /* MSG id is CLOSING_RCV    */

typedef uint32_t socket_msg_t;

typedef enum
{
  COM_SOCKET_INVALID = 0,
  COM_SOCKET_CREATING,
  COM_SOCKET_CREATED,
  COM_SOCKET_CONNECTED,
  COM_SOCKET_SENDING,
  COM_SOCKET_WAITING_RSP,
  COM_SOCKET_WAITING_FROM,
  COM_SOCKET_CLOSING
} com_socket_state_t;

typedef struct _socket_desc_t
{
  com_socket_state_t    state;       /* socket state */
  com_bool_t            local;       /*   internal id - e.g for ping
                                       or external id - e.g modem    */
  com_bool_t            closing;     /* close recv from remote  */
  uint8_t               type;        /* Socket Type TCP/UDP/RAW */
  int32_t               error;       /* last command status     */
  int32_t               id;          /* identifier */
  uint16_t              local_port;  /* local port */
  uint16_t              remote_port; /* remote port */
  com_ip_addr_t         remote_addr; /* remote addr */
  uint32_t              snd_timeout; /* timeout for send cmd    */
  uint32_t              rcv_timeout; /* timeout for receive cmd */
  osMessageQId          queue;       /* message queue for URC   */
  com_ping_rsp_t        *rsp;
  struct _socket_desc_t *next;       /* chained list            */
} socket_desc_t;

typedef struct
{
  CS_IPaddrType_t ip_type;
  /* In cellular_service socket_configure_remote()
     memcpy from char *addr to addr[] without knowing the length
     and by using strlen(char *addr) so ip_value must contain /0 */
  /* IPv4 : xxx.xxx.xxx.xxx=15+/0*/
  /* IPv6 : xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx = 39+/0*/
  uint8_t         ip_value[40];
  uint16_t        port;
} socket_addr_t;

/* Private macros ------------------------------------------------------------*/

#define COM_MIN(a,b) (((a)<(b)) ? (a) : (b))

#define SOCKET_SET_ERROR(socket, val) do {\
                                           if ((socket) != NULL) {\
                                           (socket)->error = (val); }\
                                         } while(0)

#define SOCKET_GET_ERROR(socket, val) do {\
                                           if ((socket) != NULL) {\
                                             (*(int32_t *)(val)) = ((((socket)->error) == COM_SOCKETS_ERR_OK) ? \
                                             COM_SOCKETS_ERR_OK : \
                                           com_sockets_err_to_errno((com_sockets_err_t)((socket)->error))); }\
                                           else {\
                                             (*(int32_t *)(val)) = \
                                           com_sockets_err_to_errno(COM_SOCKETS_ERR_DESCRIPTOR); }\
                                         } while(0)

/* Description
socket_msg_t
{
  socket_msg_type_t type;
  socket_msg_id_t   id;
} */
#define SET_SOCKET_MSG_TYPE(msg, type) ((msg) = ((msg)&0xFFFF0000U) | (type))
#define SET_SOCKET_MSG_ID(msg, id)     ((msg) = ((msg)&0x0000FFFFU) | ((id)<<16))
#define GET_SOCKET_MSG_TYPE(msg)       ((socket_msg_type_t)((msg)&0x0000FFFFU))
#define GET_SOCKET_MSG_ID(msg)         ((socket_msg_id_t)(((msg)&0xFFFF0000U)>>16))

/* Private variables ---------------------------------------------------------*/

/* Mutex to protect access to :
   socket descriptor list,
   socket local_id array */
static osMutexId ComSocketsMutexHandle;
/* List socket descriptor */
static socket_desc_t *socket_desc_list;
/* Provide a socket local id */
static com_bool_t socket_local_id[COM_SOCKET_LOCAL_ID_NB]; /* false : unused
                                                              true  : in use  */
#if (USE_COM_PING == 1)
/* Ping socket id */
static int32_t ping_socket_id;
#endif /* USE_COM_PING  == 1 */

#if (USE_DATACACHE == 1)
static com_bool_t network_is_up;
#endif /* USE_DATACACHE == 1 */

/* local port allocated */
static uint16_t local_port;

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/* Callback prototype */

static void com_ip_modem_data_ready_cb(socket_handle_t sock);
static void com_ip_modem_closing_cb(socket_handle_t sock);

#if (USE_DATACACHE == 1)
static void com_socket_datacache_cb(dc_com_event_id_t dc_event_id,
                                    const void *private_gui_data);
#endif /* USE_DATACACHE == 1 */

#if (USE_COM_PING == 1)
static void com_ip_modem_ping_rsp_cb(CS_Ping_response_t ping_rsp);
#endif /* USE_COM_PING == 1 */

static void com_ip_modem_init_socket_desc(socket_desc_t *socket_desc);
static socket_desc_t *com_ip_modem_create_socket_desc(void);
static socket_desc_t *com_ip_modem_provide_socket_desc(com_bool_t local);
static void com_ip_modem_delete_socket_desc(int32_t    sock,
                                            com_bool_t local);
static socket_desc_t *com_ip_modem_find_socket(int32_t    sock,
                                               com_bool_t local);
static com_bool_t com_translate_ip_address(const com_sockaddr_t *addr,
                                           int32_t       addrlen,
                                           socket_addr_t *socket_addr);
static com_bool_t com_convert_IPString_to_sockaddr(uint16_t       ipaddr_port,
                                                   com_char_t     *ipaddr_str,
                                                   com_sockaddr_t *sockaddr);
static void com_convert_ipaddr_port_to_sockaddr(const com_ip_addr_t *ip_addr,
                                                uint16_t port,
                                                com_sockaddr_in_t *sockaddr_in);
static void com_convert_sockaddr_to_ipaddr_port(const com_sockaddr_in_t *sockaddr_in,
                                                com_ip_addr_t *ip_addr,
                                                uint16_t *port);

static com_bool_t com_ip_modem_is_network_up(void);

static uint16_t com_ip_modem_new_local_port(void);

static int32_t com_ip_modem_connect_udp_service(socket_desc_t *socket_desc);

/* Private function Definition -----------------------------------------------*/

static void com_ip_modem_init_socket_desc(socket_desc_t *socket_desc)
{
  socket_desc->state            = COM_SOCKET_INVALID;
  socket_desc->local            = COM_SOCKETS_FALSE;
  socket_desc->closing          = COM_SOCKETS_FALSE;
  socket_desc->id               = COM_SOCKET_INVALID_ID;
  socket_desc->local_port       = 0U;
  socket_desc->remote_port      = 0U;
  socket_desc->remote_addr.addr = 0U;
  (void) memset((void *)&socket_desc->remote_addr, 0, sizeof(socket_desc->remote_addr));
  socket_desc->rcv_timeout      = RTOS_WAIT_FOREVER;
  socket_desc->snd_timeout      = RTOS_WAIT_FOREVER;
  socket_desc->error            = COM_SOCKETS_ERR_OK;
}

static socket_desc_t *com_ip_modem_create_socket_desc(void)
{
  socket_desc_t *socket_desc;

  socket_desc = (socket_desc_t *)pvPortMalloc(sizeof(socket_desc_t));
  if (socket_desc != NULL)
  {
    osMessageQDef(SOCKET_DESC_QUEUE, 2, uint32_t);
    socket_desc->queue = osMessageCreate(osMessageQ(SOCKET_DESC_QUEUE), NULL);
    if (socket_desc->queue == NULL)
    {
      vPortFree(socket_desc);
    }
    else
    {
      socket_desc->next = NULL;
      com_ip_modem_init_socket_desc(socket_desc);
    }
  }

  return socket_desc;
}

static socket_desc_t *com_ip_modem_provide_socket_desc(com_bool_t local)
{
  (void)osMutexWait(ComSocketsMutexHandle, RTOS_WAIT_FOREVER);

  com_bool_t found;
  uint8_t i;
  socket_desc_t *socket_desc;
  socket_desc_t *socket_desc_previous;

  i = 0U;
  found = COM_SOCKETS_FALSE;
  socket_desc = socket_desc_list;

  /* If socket is local first check an id is still available in the table */
  if (local == COM_SOCKETS_TRUE)
  {
    while ((i < COM_SOCKET_LOCAL_ID_NB)
           && (found == COM_SOCKETS_FALSE))
    {
      if (socket_local_id[i] == COM_SOCKETS_FALSE)
      {
        /* an unused local id has been found */
        found = COM_SOCKETS_TRUE;
        /* com_socket_local_id book is done only if socket is really created */
      }
    }
  }
  else
  {
    /* if local == false no specific treatment to do */
    found = COM_SOCKETS_TRUE;
  }

  if (found == COM_SOCKETS_TRUE)
  {
    /* Need to create a new socket_desc ? */
    while ((socket_desc->state != COM_SOCKET_INVALID)
           && (socket_desc->next != NULL))
    {
      /* Check next descriptor */
      socket_desc = socket_desc->next;
    }
    /* Find an empty socket ? */
    if (socket_desc->state != COM_SOCKET_INVALID)
    {
      /* No empty socket */
      /* Save the last socket descriptor to attach the new one */
      socket_desc_previous = socket_desc;
      /* Create new socket descriptor */
      socket_desc = com_ip_modem_create_socket_desc();
      if (socket_desc != NULL)
      {
        PrintDBG("socket desc created %ld queue %p", socket_desc->id, socket_desc->queue)
        /* Before to attach this new socket to the descriptor list
           finalize its intialization */
        socket_desc->state = COM_SOCKET_CREATING;
        socket_desc->local = local;
        if (local == COM_SOCKETS_TRUE)
        {
          /* even if an application create two sockets one local - one distant
             no issue if id is same because it will be stored
             in two variables at application level */
          /* Don't need an OFFSET to not overlap Modem id */
          socket_desc->id = (int32_t)i;
          /*  Socket is really created - book com_socket_local_id */
          socket_local_id[i] = COM_SOCKETS_TRUE;
        }
        /* Initialization is finalized, socket can be attached to the list
           and so visible to other functions
           even the one accessing in read mode to socket descriptor list */
        socket_desc_previous->next = socket_desc;
      }
    }
    else
    {
      /* Find an empty place */
      socket_desc->state = COM_SOCKET_CREATING;
      socket_desc->local = local;
      if (local == COM_SOCKETS_TRUE)
      {
        /* even if an application create two sockets one local - one distant
           no issue if id is same because it will be stored
           in two variable s at application level */
        /* Don't need an OFFSET to not overlap Modem id */
        socket_desc->id = (int32_t)i;
        /*  Socket is really created - book com_socket_local_id */
        socket_local_id[i] = COM_SOCKETS_TRUE;
      }
    }
  }

  (void)osMutexRelease(ComSocketsMutexHandle);

  /* If provide == false then socket_desc = NULL */
  return socket_desc;
}

static void com_ip_modem_delete_socket_desc(int32_t    sock,
                                            com_bool_t local)
{
  (void)osMutexWait(ComSocketsMutexHandle, RTOS_WAIT_FOREVER);

  com_bool_t found;
  socket_desc_t *socket_desc;

  found = COM_SOCKETS_FALSE;

  socket_desc = socket_desc_list;

  while ((socket_desc != NULL)
         && (found != COM_SOCKETS_TRUE))
  {
    if ((socket_desc->id == sock)
        && (socket_desc->local == local))
    {
      found = COM_SOCKETS_TRUE;
    }
    else
    {
      socket_desc = socket_desc->next;
    }
  }
  if (found == COM_SOCKETS_TRUE)
  {
    /* Always keep a created socket */
    com_ip_modem_init_socket_desc(socket_desc);
    if (local == COM_SOCKETS_TRUE)
    {
      /* Free com_socket_local_id */
      socket_local_id[sock] = COM_SOCKETS_FALSE;
    }
  }

  (void)osMutexRelease(ComSocketsMutexHandle);
}

static socket_desc_t *com_ip_modem_find_socket(int32_t    sock,
                                               com_bool_t local)
{
  socket_desc_t *socket_desc;
  com_bool_t found;

  socket_desc = socket_desc_list;
  found = COM_SOCKETS_FALSE;

  while ((socket_desc != NULL)
         && (found != COM_SOCKETS_TRUE))
  {
    if ((socket_desc->id == sock)
        && (socket_desc->local == local))
    {
      found = COM_SOCKETS_TRUE;
    }
    else
    {
      socket_desc = socket_desc->next;
    }
  }

  /* If found == false then socket_desc = NULL */
  return socket_desc;
}

static com_bool_t com_translate_ip_address(const com_sockaddr_t *addr,
                                           int32_t       addrlen,
                                           socket_addr_t *socket_addr)
{
  com_bool_t result;
  const com_sockaddr_in_t *p_sockaddr_in;

  result = COM_SOCKETS_FALSE;

  if (addrlen == (int32_t)sizeof(com_sockaddr_in_t))
  {
    p_sockaddr_in = (const com_sockaddr_in_t *)addr;

    if ((addr != NULL)
        && (socket_addr != NULL))
    {
      if (addr->sa_family == (uint8_t)COM_AF_INET)
      {
        socket_addr->ip_type = CS_IPAT_IPV4;
        if (p_sockaddr_in->sin_addr.s_addr == COM_INADDR_ANY)
        {
          (void)memcpy(&socket_addr->ip_value[0], "0.0.0.0", strlen("0.0.0.0"));
        }
        else
        {
          com_ip_addr_t com_ip_addr;

          com_ip_addr.addr = ((const com_sockaddr_in_t *)addr)->sin_addr.s_addr;
          (void)sprintf((CSIP_CHAR_t *)socket_addr->ip_value,
                        "%d.%d.%d.%d",
                        COM_IP4_ADDR1(&com_ip_addr),
                        COM_IP4_ADDR2(&com_ip_addr),
                        COM_IP4_ADDR3(&com_ip_addr),
                        COM_IP4_ADDR4(&com_ip_addr));
        }
        socket_addr->port = com_ntohs(((const com_sockaddr_in_t *)addr)->sin_port);
        result = COM_SOCKETS_TRUE;
      }
      /* else if (addr->sa_family == COM_AF_INET6) */
      /* or any other value */
      /* not supported */
    }
  }

  return result;
}

static com_bool_t com_convert_IPString_to_sockaddr(uint16_t       ipaddr_port,
                                                   com_char_t     *ipaddr_str,
                                                   com_sockaddr_t *sockaddr)
{
  com_bool_t result;
  int32_t  count;
  uint8_t  begin;
  uint32_t  ip_addr[4];
  com_ip4_addr_t ip4_addr;

  result = COM_SOCKETS_FALSE;

  if (sockaddr != NULL)
  {
    begin = (ipaddr_str[0] == ((uint8_t)'"')) ? 1U : 0U;

    (void)memset(sockaddr, 0, sizeof(com_sockaddr_t));

    count = sscanf((CSIP_CHAR_t *)(&ipaddr_str[begin]),
                   "%03lu.%03lu.%03lu.%03lu",
                   &ip_addr[0], &ip_addr[1],
                   &ip_addr[2], &ip_addr[3]);

    if (count == 4)
    {
      if ((ip_addr[0] <= 255U)
          && (ip_addr[1] <= 255U)
          && (ip_addr[2] <= 255U)
          && (ip_addr[3] <= 255U))
      {
        sockaddr->sa_family = (uint8_t)COM_AF_INET;
        sockaddr->sa_len    = (uint8_t)sizeof(com_sockaddr_in_t);
        ((com_sockaddr_in_t *)sockaddr)->sin_port = com_htons(ipaddr_port);
        COM_IP4_ADDR(&ip4_addr,
                     ip_addr[0], ip_addr[1],
                     ip_addr[2], ip_addr[3]);
        ((com_sockaddr_in_t *)sockaddr)->sin_addr.s_addr = ip4_addr.addr;
        result = COM_SOCKETS_TRUE;
      }
    }
  }

  return (result);
}

static void com_convert_ipaddr_port_to_sockaddr(const com_ip_addr_t *ip_addr,
                                                uint16_t port,
                                                com_sockaddr_in_t *sockaddr_in)
{
  sockaddr_in->sin_len         = (uint8_t)sizeof(com_sockaddr_in_t);
  sockaddr_in->sin_family      = COM_AF_INET;
  sockaddr_in->sin_addr.s_addr = ip_addr->addr;
  sockaddr_in->sin_port        = com_htons(port);
  (void) memset(sockaddr_in->sin_zero, 0, COM_SIN_ZERO_LEN);
}

static void com_convert_sockaddr_to_ipaddr_port(const com_sockaddr_in_t *sockaddr_in,
                                                com_ip_addr_t *ip_addr,
                                                uint16_t *port)
{
 ip_addr->addr = sockaddr_in->sin_addr.s_addr;
 *port = com_ntohs(sockaddr_in->sin_port);
}

static com_bool_t com_ip_modem_is_network_up(void)
{
  com_bool_t result;

#if (USE_DATACACHE == 1)
  result = network_is_up;
#else /* USE_DATACACHE == 0 */
  /* Feature not supported without Datacache
     Do not block -=> consider network is up */
  result = COM_SOCKETS_TRUE;
#endif /* USE_DATACACHE == 1 */

  return result;
}

static uint16_t com_ip_modem_new_local_port(void)
{
  com_bool_t local_port_ok;
  com_bool_t found;
  uint16_t iter;
  uint16_t result;
  socket_desc_t *socket_desc;

  local_port_ok = COM_SOCKETS_FALSE;
  iter = 0U;

  while ((local_port_ok != COM_SOCKETS_TRUE)
         && (iter < (LOCAL_PORT_END - LOCAL_PORT_BEGIN)))
  {
    local_port++;
    iter++;
    if (local_port == LOCAL_PORT_END)
    {
      local_port = LOCAL_PORT_BEGIN;
    }

    socket_desc = socket_desc_list;
    found = COM_SOCKETS_FALSE;

    while ((socket_desc != NULL)
           && (found != COM_SOCKETS_TRUE))
    {
      if (socket_desc->local_port == local_port)
      {
        found = COM_SOCKETS_TRUE;
      }
      else
      {
        socket_desc = socket_desc->next;
      }
    }

    if (found == COM_SOCKETS_FALSE)
    {
      local_port_ok = COM_SOCKETS_TRUE;
    }
  }
  if (local_port_ok != COM_SOCKETS_TRUE)
  {
    result = 0U;
  }
  else
  {
    result = local_port;
  }

  return result;
}

static int32_t com_ip_modem_connect_udp_service(socket_desc_t *socket_desc)
{
  int32_t result;

  result = COM_SOCKETS_ERR_STATE;

  if (socket_desc->state == COM_SOCKET_CREATED)
  {
    result = COM_SOCKETS_ERR_OK;

    if (socket_desc->local_port == 0U)
    {
      socket_desc->local_port = com_ip_modem_new_local_port();
      if (socket_desc->local_port == 0U)
      {
        /* No local port available */
        result = COM_SOCKETS_ERR_LOCKED;
      }
    }

    /* Connect must be done with specific parameter*/
    if (result == COM_SOCKETS_ERR_OK)
    {
      result = COM_SOCKETS_ERR_GENERAL;

      if (osCDS_socket_bind(socket_desc->id,
                            socket_desc->local_port)
          == CELLULAR_OK)
      {
        PrintINFO("socket internal bind ok")
        if (osCDS_socket_connect(socket_desc->id,
                                 CS_IPAT_IPV4,
                                 CONFIG_MODEM_UDP_SERVICE_CONNECT_IP,
                                 0)
            == CELLULAR_OK)
        {
          result = COM_SOCKETS_ERR_OK;
          PrintINFO("socket internal connect ok")
          socket_desc->state = COM_SOCKET_CONNECTED;
        }
        else
        {
          PrintERR("socket internal connect NOK at low level")
        }
      }
      else
      {
        PrintERR("socket internal bind NOK at low level")
      }
    }
  }
  else if (socket_desc->state == COM_SOCKET_CONNECTED)
  {
    /* Already connected - nothing to do */
    result = COM_SOCKETS_ERR_OK;
  }
  else
  {
    PrintERR("socket internal connect - err state")
  }

  return (result);
}

/**
  * @brief  Callback called when URC data ready raised
  * @note   Managed URC data ready
  * @param  sock - socket handle
  * @note   -
  * @retval None
  */
static void com_ip_modem_data_ready_cb(socket_handle_t sock)
{
  socket_msg_t   msg;
  socket_desc_t *socket_desc;

  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);
  msg = 0U;

  if (socket_desc != NULL)
  {
    if (socket_desc->closing != COM_SOCKETS_TRUE)
    {
      if (socket_desc->state == COM_SOCKET_WAITING_RSP)
      {
        PrintINFO("cb socket %ld data ready called: waiting rsp", socket_desc->id)
        SET_SOCKET_MSG_TYPE(msg, SOCKET_MSG);
        SET_SOCKET_MSG_ID(msg, DATA_RCV);
        PrintDBG("cb socket %ld MSGput %lu queue %p", socket_desc->id, msg, socket_desc->queue)
        (void)osMessagePut(socket_desc->queue, msg, 0U);
      }
      else if (socket_desc->state == COM_SOCKET_WAITING_FROM)
      {
        PrintINFO("cb socket %ld data ready called: waiting from", socket_desc->id)
        SET_SOCKET_MSG_TYPE(msg, SOCKET_MSG);
        SET_SOCKET_MSG_ID(msg, DATA_RCV);
        PrintDBG("cb socket %ld MSGput %lu queue %p", socket_desc->id, msg, socket_desc->queue)
        (void)osMessagePut(socket_desc->queue, msg, 0U);
      }
      else
      {
        PrintINFO("cb socket data ready called: socket_state:%i NOK",
                  socket_desc->state)
      }
    }
    else
    {
      PrintERR("cb socket data ready called: socket is closing")
    }
  }
  else
  {
    PrintERR("cb socket data ready called: unknown socket")
  }
}

/**
  * @brief  Callback called when URC socket closing raised
  * @note   Managed URC socket closing
  * @param  sock - socket handle
  * @note   -
  * @retval None
  */
static void com_ip_modem_closing_cb(socket_handle_t sock)
{
  PrintDBG("callback socket closing called")

  socket_msg_t   msg;
  socket_desc_t *socket_desc;

  msg = 0U;
  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);
  if (socket_desc != NULL)
  {
    PrintINFO("cb socket closing called: close rqt")
    if (socket_desc->closing == COM_SOCKETS_FALSE)
    {
      socket_desc->closing = COM_SOCKETS_TRUE;
      PrintINFO("cb socket closing: close rqt")
    }
    if ((socket_desc->state == COM_SOCKET_WAITING_RSP)
        || (socket_desc->state == COM_SOCKET_WAITING_FROM))
    {
      PrintERR("!!! cb socket %ld closing called: data_expected !!!", socket_desc->id)
      SET_SOCKET_MSG_TYPE(msg, SOCKET_MSG);
      SET_SOCKET_MSG_ID(msg, CLOSING_RCV);
      PrintDBG("cb socket %ld MSGput %lu queue %p", socket_desc->id, msg, socket_desc->queue)
      (void)osMessagePut(socket_desc->queue, msg, 0U);
    }
  }
  else
  {
    PrintERR("cb socket closing called: unknown socket")
  }
}

#if (USE_DATACACHE == 1)
/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  private_gui_data - value provided at service subscription
  * @note   Unused
  * @retval None
  */
static void com_socket_datacache_cb(dc_com_event_id_t dc_event_id,
                                    const void *private_gui_data)
{
  UNUSED(private_gui_data);

  if (dc_event_id == DC_COM_NIFMAN_INFO)
  {
    dc_nifman_info_t  dc_nifman_rt_info;

    if (dc_com_read(&dc_com_db, DC_COM_NIFMAN_INFO,
                    (void *)&dc_nifman_rt_info,
                    sizeof(dc_nifman_rt_info))
        == DC_COM_OK)
    {
      if (dc_nifman_rt_info.rt_state == DC_SERVICE_ON)
      {
        network_is_up = COM_SOCKETS_TRUE;
        //com_sockets_statistic_update(COM_SOCKET_STAT_NWK_UP);
      }
      else
      {
        if (network_is_up == COM_SOCKETS_TRUE)
        {
          network_is_up = COM_SOCKETS_FALSE;
          //com_sockets_statistic_update(COM_SOCKET_STAT_NWK_DWN);
        }
      }
    }
  }
  else
  {
    /* Nothing to do */
  }
}
#endif /* USE_DATACACHE == 1 */

#if (USE_COM_PING == 1)
/**
  * @brief Ping response callback
  */
static void com_ip_modem_ping_rsp_cb(CS_Ping_response_t ping_rsp)
{
  PrintDBG("callback ping response")

  socket_msg_t  msg;
  socket_desc_t *socket_desc;

  msg = 0U;
  socket_desc = com_ip_modem_find_socket(ping_socket_id,
                                         COM_SOCKETS_TRUE);
  PrintDBG("Ping rsp status:%d index:%d final:%d time:%d min:%d avg:%d max:%d",
           ping_rsp.ping_status,
           ping_rsp.index, ping_rsp.is_final_report,
           ping_rsp.time, ping_rsp.min_time, ping_rsp.avg_time, ping_rsp.max_time)

  if ((socket_desc != NULL)
      && (socket_desc->closing == COM_SOCKETS_FALSE)
      && (socket_desc->state   == COM_SOCKET_WAITING_RSP))
  {
    if (ping_rsp.ping_status != CELLULAR_OK)
    {
      socket_desc->rsp->status = COM_SOCKETS_ERR_GENERAL;
      socket_desc->rsp->time   = 0U;
      socket_desc->rsp->bytes  = 0U;
      PrintINFO("callback ping data ready: error rcv - exit")
      SET_SOCKET_MSG_TYPE(msg, PING_MSG);
      SET_SOCKET_MSG_ID(msg, DATA_RCV);
      (void)osMessagePut(socket_desc->queue, msg, 0U);
    }
    else
    {
      if (ping_rsp.index == 1U)
      {
        if (ping_rsp.is_final_report == CELLULAR_FALSE)
        {
          /* Save the data */
          socket_desc->rsp->status = COM_SOCKETS_ERR_OK;
          socket_desc->rsp->time   = ping_rsp.time;
          socket_desc->rsp->bytes  = ping_rsp.bytes;
          PrintINFO("callback ping data ready: rsp rcv - wait final report")
        }
        else
        {
          socket_desc->rsp->status = COM_SOCKETS_ERR_GENERAL;
          socket_desc->rsp->time   = 0U;
          socket_desc->rsp->bytes  = 0U;
          PrintINFO("callback ping data ready: error msg - exit")
          SET_SOCKET_MSG_TYPE(msg, PING_MSG);
          SET_SOCKET_MSG_ID(msg, DATA_RCV);
          (void)osMessagePut(socket_desc->queue, msg, 0U);
        }
      }
      else
      {
        /* Must wait final report */
        if (ping_rsp.is_final_report == CELLULAR_FALSE)
        {
          PrintINFO("callback ping data ready: rsp already rcv - index %d",
                    ping_rsp.index)
        }
        else
        {
          PrintINFO("callback ping data ready: final report rcv")
          SET_SOCKET_MSG_TYPE(msg, PING_MSG);
          SET_SOCKET_MSG_ID(msg, DATA_RCV);
          (void)osMessagePut(socket_desc->queue, msg, 0U);
        }
      }
    }
  }
  else
  {
    PrintINFO("!!! PURGE callback ping data ready - index %d !!!",
              ping_rsp.index)
    if (socket_desc == NULL)
    {
      PrintERR("callback ping data ready: unknown ping")
    }
    else
    {
      if (socket_desc->closing == COM_SOCKETS_TRUE)
      {
        PrintERR("callback ping data ready: ping is closing")
      }
      if (socket_desc->state != COM_SOCKET_WAITING_RSP)
      {
        PrintDBG("callback ping data ready: ping state:%d index:%d final report:%u NOK",
                 socket_desc->state, ping_rsp.index, ping_rsp.is_final_report)
      }
    }
  }
}
#endif /* USE_COM_PING == 1 */


/* Functions Definition ------------------------------------------------------*/

/*** Socket management ********************************************************/

/**
  * @brief  Socket handle creation
  * @note   Create a communication endpoint called socket
  *         only TCP/UDP IPv4 client mode supported
  * @param  family   - address family
  * @note   only AF_INET supported
  * @param  type     - connection type
  * @note   only SOCK_STREAM or SOCK_DGRAM supported
  * @param  protocol - protocol type
  * @note   only IPPROTO_TCP or IPPROTO_UDP supported
  * @retval int32_t  - socket handle or error value
  */
int32_t com_socket_ip_modem(int32_t family, int32_t type, int32_t protocol)
{
  int32_t sock;
  int32_t result;

  CS_IPaddrType_t IPaddrType;
  CS_TransportProtocol_t TransportProtocol;
  CS_PDN_conf_id_t PDN_conf_id;

  result = COM_SOCKETS_ERR_OK;
  sock = COM_SOCKET_INVALID_ID;

  IPaddrType = CS_IPAT_IPV4;
  TransportProtocol = CS_TCP_PROTOCOL;
  PDN_conf_id = CS_PDN_CONFIG_DEFAULT;

  if (family == COM_AF_INET)
  {
    /* address family IPv4 */
    IPaddrType = CS_IPAT_IPV4;
  }
  else if (family == COM_AF_INET6)
  {
    /* address family IPv6 */
    IPaddrType = CS_IPAT_IPV6;
    result = COM_SOCKETS_ERR_PARAMETER;
  }
  else
  {
    result = COM_SOCKETS_ERR_PARAMETER;
  }

  if ((type == COM_SOCK_STREAM)
      && ((protocol == COM_IPPROTO_TCP)
          || (protocol == COM_IPPROTO_IP)))
  {
    /* IPPROTO_TCP = must be used with SOCK_STREAM */
    TransportProtocol = CS_TCP_PROTOCOL;
  }
  else if ((type == COM_SOCK_DGRAM)
           && ((protocol == COM_IPPROTO_UDP)
               || (protocol == COM_IPPROTO_IP)))
  {
    /* IPPROTO_UDP = must be used with SOCK_DGRAM */
    TransportProtocol = CS_UDP_PROTOCOL;
  }
  else
  {
    result = COM_SOCKETS_ERR_PARAMETER;
  }


  if (result == COM_SOCKETS_ERR_OK)
  {
    result = COM_SOCKETS_ERR_GENERAL;
    PrintDBG("socket create request")
    sock = osCDS_socket_create(IPaddrType,
                               TransportProtocol,
                               PDN_conf_id);

    if (sock != CS_INVALID_SOCKET_HANDLE)
    {
      socket_desc_t *socket_desc;
      PrintINFO("create socket ok low level")

      /* Need to create a new socket_desc ? */
      socket_desc = com_ip_modem_provide_socket_desc(COM_SOCKETS_FALSE);
      if (socket_desc == NULL)
      {
        result = COM_SOCKETS_ERR_NOMEMORY;
        PrintERR("create socket NOK no memory")
        /* Socket descriptor is not existing in COM
           must close directly the socket and not call com_close */
        if (osCDS_socket_close(sock, 0U)
            == CELLULAR_OK)
        {
          PrintINFO("close socket ok low level")
        }
        else
        {
          PrintERR("close socket NOK low level")
        }
      }
      else
      {
        socket_desc->id    = sock;
        socket_desc->type  = (uint8_t)type;
        socket_desc->state = COM_SOCKET_CREATED;

        if (osCDS_socket_set_callbacks(sock,
                                       com_ip_modem_data_ready_cb,
                                       NULL,
                                       com_ip_modem_closing_cb)
            == CELLULAR_OK)
        {
          result = COM_SOCKETS_ERR_OK;
        }
        else
        {
          PrintERR("rqt close socket issue at creation")
          if (com_closesocket_ip_modem(sock)
              == COM_SOCKETS_ERR_OK)
          {
            PrintINFO("close socket ok low level")
          }
          else
          {
            PrintERR("close socket NOK low level")
          }
        }
      }
    }
    else
    {
      PrintERR("create socket NOK low level")
    }
    /* Stat only socket whose parameters are supported */
    /*com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? \
                                 COM_SOCKET_STAT_CRE_OK : COM_SOCKET_STAT_CRE_NOK);*/
  }
  else
  {
    PrintERR("create socket NOK parameter NOK")
  }

  /* result == COM_SOCKETS_ERR_OK return socket handle */
  /* result != COM_SOCKETS_ERR_OK socket not created,
     no need to call SOCKET_SET_ERROR */
  return ((result == COM_SOCKETS_ERR_OK) ? sock : result);
}

/**
  * @brief  Socket option set
  * @note   Set option for the socket
  * @note   only send or receive timeout supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  level     - level at which the option is defined
  * @note   only COM_SOL_SOCKET supported
  * @param  optname   - option name for which the value is to be set
  * @note   COM_SO_SNDTIMEO : OK but value not used because there is already
  *                           a tempo at low level - risk of conflict
  *         COM_SO_RCVTIMEO : OK
  *         any other value is rejected
  * @param  optval    - pointer to the buffer containing the option value
  * @note   COM_SO_SNDTIMEO and COM_SO_RCVTIMEO : unit is ms
  * @param  optlen    - size of the buffer containing the option value
  * @retval int32_t   - ok or error value
  */
int32_t com_setsockopt_ip_modem(int32_t sock, int32_t level, int32_t optname,
                                const void *optval, int32_t optlen)
{
  int32_t result;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    if ((optval != NULL)
        && (optlen > 0))
    {
      if (level == COM_SOL_SOCKET)
      {
        switch (optname)
        {
          case COM_SO_SNDTIMEO :
          {
            if ((uint32_t)optlen <= sizeof(socket_desc->rcv_timeout))
            {
              /* A tempo already exists at low level and cannot be redefined */
              /* Ok to accept value setting but :
                 value will not be used due to conflict risk
                 if tempo differ from low level tempo value */
              socket_desc->snd_timeout = *(const uint32_t *)optval;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          case COM_SO_RCVTIMEO :
          {
            if ((uint32_t)optlen <= sizeof(socket_desc->rcv_timeout))
            {
              /* A tempo already exists at low level and cannot be redefined */
              /* Ok to accept value setting but :
                 if tempo value is shorter and data are received after
                 then if socket is not closing data still available in the modem
                 and can still be read if modem manage this feature
                 if tempo value is bigger and data are received before
                 then data will be send to application */
              socket_desc->rcv_timeout = *(const uint32_t *)optval;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          case COM_SO_ERROR :
          {
            /* Set for this option NOK */
            break;
          }
          default :
          {
            /* Other options NOT yet supported */
            break;
          }
        }
      }
      else
      {
        /* Other level than SOL_SOCKET NOT yet supported */
      }
    }
  }
  else
  {
    result = COM_SOCKETS_ERR_DESCRIPTOR;
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return result;
}

/**
  * @brief  Socket option get
  * @note   Get option for a socket
  * @note   only send timeout, receive timeout, last error supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  level     - level at which option is defined
  * @note   only COM_SOL_SOCKET supported
  * @param  optname   - option name for which the value is requested
  * @note   COM_SO_SNDTIMEO, COM_SO_RCVTIMEO, COM_SO_ERROR supported
  *         any other value is rejected
  * @param  optval    - pointer to the buffer that will contain the option value
  * @note   COM_SO_SNDTIMEO, COM_SO_RCVTIMEO: in ms for timeout (uint32_t)
  *         COM_SO_ERROR : result of last operation (int32_t)
  * @param  optlen    - size of the buffer that will contain the option value
  * @note   must be sizeof(x32_t)
  * @retval int32_t   - ok or error value
  */
int32_t com_getsockopt_ip_modem(int32_t sock, int32_t level, int32_t optname,
                                void *optval, int32_t *optlen)
{
  int32_t result;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    if ((optval != NULL)
        && (optlen != NULL))
    {
      if (level == COM_SOL_SOCKET)
      {
        switch (optname)
        {
          case COM_SO_SNDTIMEO :
          {
            /* Force optval to be on uint32_t to be compliant with lwip */
            if ((uint32_t)*optlen == sizeof(uint32_t))
            {
              *(uint32_t *)optval = socket_desc->snd_timeout;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          case COM_SO_RCVTIMEO :
          {
            /* Force optval to be on uint32_t to be compliant with lwip */
            if ((uint32_t)*optlen == sizeof(uint32_t))
            {
              *(uint32_t *)optval = socket_desc->rcv_timeout;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          case COM_SO_ERROR :
          {
            /* Force optval to be on int32_t to be compliant with lwip */
            if ((uint32_t)*optlen == sizeof(int32_t))
            {
              SOCKET_GET_ERROR(socket_desc, optval);
              socket_desc->error = COM_SOCKETS_ERR_OK;
              result = COM_SOCKETS_ERR_OK;
            }
            break;
          }
          default :
          {
            /* Other options NOT yet supported */
            break;
          }
        }
      }
      else
      {
        /* Other level than SOL_SOCKET NOT yet supported */
      }
    }
  }
  else
  {
    result = COM_SOCKETS_ERR_DESCRIPTOR;
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return result;
}

/**
  * @brief  Socket bind
  * @note   Assign a local address and port to a socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - local IP address and port
  * @note   only port value field is used as local port,
  *         but whole addr parameter must be "valid"
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_bind_ip_modem(int32_t sock,
                          const com_sockaddr_t *addr, int32_t addrlen)
{
  int32_t result;

  socket_addr_t socket_addr;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    if (socket_desc->state == COM_SOCKET_CREATED)
    {
      if (com_translate_ip_address(addr, addrlen,
                                   &socket_addr)
          == COM_SOCKETS_TRUE)
      {
        result = COM_SOCKETS_ERR_GENERAL;
        PrintDBG("socket bind request")
        if (osCDS_socket_bind(sock,
                              socket_addr.port)
            == CELLULAR_OK)
        {
          PrintINFO("socket bind ok low level")
          result = COM_SOCKETS_ERR_OK;
          socket_desc->local_port = socket_addr.port;
        }
      }
      else
      {
        PrintERR("socket bind NOK translate IP NOK")
      }
    }
    else
    {
      result = COM_SOCKETS_ERR_STATE;
      PrintERR("socket bind NOK state invalid")
    }
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return result;
}

/**
  * @brief  Socket close
  * @note   Close a socket and release socket handle
  *         For an opened socket as long as socket close is in error value
  *         socket must be considered as not closed and handle as not released
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @retval int32_t   - ok or error value
  */
int32_t com_closesocket_ip_modem(int32_t sock)
{
  int32_t result;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    /* If socket is currently under process refused to close it */
    if ((socket_desc->state == COM_SOCKET_SENDING)
        || (socket_desc->state == COM_SOCKET_WAITING_RSP))
    {
      PrintERR("close socket NOK err state")
      result = COM_SOCKETS_ERR_INPROGRESS;
    }
    else
    {
      result = COM_SOCKETS_ERR_GENERAL;
      if (osCDS_socket_close(sock, 0U)
          == CELLULAR_OK)
      {
        com_ip_modem_delete_socket_desc(sock, COM_SOCKETS_FALSE);
        result = COM_SOCKETS_ERR_OK;
        PrintINFO("close socket ok")
      }
      else
      {
        PrintINFO("close socket NOK low level")
      }
    }
    /*com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? \
                                 COM_SOCKET_STAT_CLS_OK : COM_SOCKET_STAT_CLS_NOK);*/
  }

  return (result);
}


/*** Client functionalities ***************************************************/

/**
  * @brief  Socket connect
  * @note   Connect socket to a remote host
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - remote IP address and port
  * @note   only an IPv4 address is supported
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_connect_ip_modem(int32_t sock,
                             const com_sockaddr_t *addr, int32_t addrlen)
{
  int32_t result;
  socket_addr_t socket_addr;
  socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;

  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);

  if (socket_desc != NULL)
  {
    if (com_translate_ip_address(addr, addrlen,
                                 &socket_addr)
        == COM_SOCKETS_TRUE)
    {
      if (socket_desc->type == (uint8_t)COM_SOCK_STREAM)
      {
        if (socket_desc->state == COM_SOCKET_CREATED)
        {
          /* Check Network status */
          if (com_ip_modem_is_network_up() == COM_SOCKETS_TRUE)
          {
            if (osCDS_socket_connect(socket_desc->id,
                                     socket_addr.ip_type,
                                     &socket_addr.ip_value[0],
                                     socket_addr.port)
                == CELLULAR_OK)
            {
              result = COM_SOCKETS_ERR_OK;
              PrintINFO("socket connect ok")
              socket_desc->state = COM_SOCKET_CONNECTED;
            }
            else
            {
              result = COM_SOCKETS_ERR_GENERAL;
              PrintERR("socket connect NOK at low level")
            }
          }
          else
          {
            result = COM_SOCKETS_ERR_NONETWORK;
            PrintERR("socket connect NOK no network")
          }
        }
        else
        {
          result = COM_SOCKETS_ERR_STATE;
          PrintERR("socket connect NOK err state")
        }
      }
      else /* socket_desc->type == (uint8_t)COM_SOCK_DGRAM */
      {
        if (UDP_SERVICE_SUPPORTED == 0U)
        {
          /* even if CONNECTED let MODEM decide if it is supported
             to update internal configuration */
          if ((socket_desc->state == COM_SOCKET_CREATED)
              || (socket_desc->state == COM_SOCKET_CONNECTED))
          {
            if (osCDS_socket_connect(socket_desc->id,
                                     socket_addr.ip_type,
                                     &socket_addr.ip_value[0],
                                     socket_addr.port)
                == CELLULAR_OK)
            {
              result = COM_SOCKETS_ERR_OK;
              PrintINFO("socket connect ok")
              socket_desc->state = COM_SOCKET_CONNECTED;
            }
            else
            {
              result = COM_SOCKETS_ERR_GENERAL;
              PrintERR("socket connect NOK at low level")
            }
          }
          else
          {
            result = COM_SOCKETS_ERR_STATE;
            PrintERR("socket connect NOK err state")
          }
        }
        else /* UDP_SERVICES_SUPPORTED == 1U */
        {
          result = com_ip_modem_connect_udp_service(socket_desc);
        }
      }
    }
    else
    {
      result = COM_SOCKETS_ERR_PARAMETER;
    }

    /*com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? \
                                 COM_SOCKET_STAT_CNT_OK : COM_SOCKET_STAT_CNT_NOK);*/

    SOCKET_SET_ERROR(socket_desc, result);
  }

  if (result == COM_SOCKETS_ERR_OK)
  {
    /* Save remote addr - port */
    com_ip_addr_t remote_addr;
    uint16_t remote_port;

    com_convert_sockaddr_to_ipaddr_port((const com_sockaddr_in_t *)addr,
                                        &remote_addr,
                                        &remote_port);
    socket_desc->remote_addr.addr = remote_addr.addr;
    socket_desc->remote_port = remote_port;
  }

  return result;
}


/**
  * @brief  Socket send data
  * @note   Send data on already connected socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to send
  * @note   see below
  * @param  len       - size of the data to send (in bytes)
  * @note   see below
  * @param  flags     - options
  * @note   - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         if len > interface between COM and low level
  *         a maximum of MODEM_MAX_TX_DATA_SIZE can be send
  *         - if flags = COM_MSG_WAIT, application accept to wait result
  *         of possible multiple send between COM and low level
  *         a buffer whose len > interface can be send
  *         COM will fragment the buffer according to the interface
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_send_ip_modem(int32_t sock,
                          const com_char_t *buf, int32_t len,
                          int32_t flags)
{
  com_bool_t is_network_up;
  socket_desc_t *socket_desc;
  int32_t result;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);

  if ((socket_desc != NULL)
      && (buf != NULL)
      && (len > 0))
  {
    if (socket_desc->state == COM_SOCKET_CONNECTED)
    {
      /* closing maybe received, refuse to send data */
      if (socket_desc->closing == COM_SOCKETS_FALSE)
      {
        /* network maybe down, refuse to send data */
        if (com_ip_modem_is_network_up() == COM_SOCKETS_FALSE)
        {
          result = COM_SOCKETS_ERR_NONETWORK;
          PrintERR("snd data NOK no network")
        }
        else
        {
          /* if UDP_SERVICE supported,
             Connect already done by Appli => send may be changed to sendto
             or by COM to use sendto/recvfrom services => send must be changed to sendto */
          if ((socket_desc->type == (uint8_t)COM_SOCK_DGRAM)
              && (UDP_SERVICE_SUPPORTED == 1U))
          {
            result = com_sendto_ip_modem(sock, buf, len, flags, NULL, 0);
          }
          else
          {
            uint32_t length_to_send;
            uint32_t length_send;

            result = COM_SOCKETS_ERR_GENERAL;
            length_send = 0U;
            socket_desc->state = COM_SOCKET_SENDING;

            if (flags == COM_MSG_DONTWAIT)
            {
              length_to_send = COM_MIN((uint32_t)len, MODEM_MAX_TX_DATA_SIZE);
              if (osCDS_socket_send(socket_desc->id,
                                    buf, length_to_send)
                  == CELLULAR_OK)
              {
                length_send = length_to_send;
                result = (int32_t)length_send;
                PrintINFO("snd data DONTWAIT ok")
              }
              else
              {
                PrintERR("snd data DONTWAIT NOK at low level")
              }
              socket_desc->state = COM_SOCKET_CONNECTED;
            }
            else
            {
              is_network_up = com_ip_modem_is_network_up();
              /* Send all data of a big buffer - Whatever the size */
              while ((length_send != (uint32_t)len)
                     && (socket_desc->closing == COM_SOCKETS_FALSE)
                     && (is_network_up == COM_SOCKETS_TRUE)
                     && (socket_desc->state == COM_SOCKET_SENDING))
              {
                length_to_send = COM_MIN((((uint32_t)len) - length_send),
                                         MODEM_MAX_TX_DATA_SIZE);
                /* A tempo is already managed at low-level */
                if (osCDS_socket_send(socket_desc->id,
                                      buf + length_send,
                                      length_to_send)
                    == CELLULAR_OK)
                {
                  length_send += length_to_send;
                  PrintINFO("snd data ok")
                  /* Update Network status */
                  is_network_up = com_ip_modem_is_network_up();
                }
                else
                {
                  socket_desc->state = COM_SOCKET_CONNECTED;
                  PrintERR("snd data NOK at low level")
                }
              }
              socket_desc->state = COM_SOCKET_CONNECTED;
              result = (int32_t)length_send;
            }
          }
        }
      }
      else
      {
        PrintERR("snd data NOK socket closing")
        result = COM_SOCKETS_ERR_CLOSING;
      }
    }
    else
    {
      PrintERR("snd data NOK err state")
      if (socket_desc->state < COM_SOCKET_CONNECTED)
      {
        result = COM_SOCKETS_ERR_STATE;
      }
      else
      {
        result = (socket_desc->state == COM_SOCKET_CLOSING) ? \
                 COM_SOCKETS_ERR_CLOSING : COM_SOCKETS_ERR_INPROGRESS;
      }
    }

    /* Do not count twice with sendto */
    if ((socket_desc->type == (uint8_t)COM_SOCK_STREAM)
        || (UDP_SERVICE_SUPPORTED == 0U))
    {
      /*com_sockets_statistic_update((result >= 0) ? \
                                   COM_SOCKET_STAT_SND_OK : COM_SOCKET_STAT_SND_NOK);*/
    }
  }

  if (result >= 0)
  {
    SOCKET_SET_ERROR(socket_desc, COM_SOCKETS_ERR_OK);
  }
  else
  {
    SOCKET_SET_ERROR(socket_desc, result);
  }

  return (result);
}

/**
  * @brief  Socket receive data
  * @note   Receive data on already connected socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to store the data to
  * @note   see below
  * @param  len       - size of application data buffer (in bytes)
  * @note   even if len > interface between COM and low level
  *         a maximum of the interface capacity can be received
  *         at each function call
  * @param  flags     - options
  * @note   - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         until data are available at low level
  *         - if flags = COM_MSG_WAIT, application accept to wait
  *         until data are available at low level with respect of potential
  *         timeout COM_SO_RCVTIMEO setting
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recv_ip_modem(int32_t sock,
                          com_char_t *buf, int32_t len,
                          int32_t flags)
{
  int32_t result;
  int32_t len_rcv;
  osEvent event;
  socket_desc_t *socket_desc;
  socket_msg_t   msg;

  result = COM_SOCKETS_ERR_PARAMETER;
  len_rcv = 0;
  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);

  if ((socket_desc != NULL)
      && (buf != NULL)
      && (len > 0))
  {
    /* Closing maybe received or Network maybe done
       but still some data to read */
    if (socket_desc->state == COM_SOCKET_CONNECTED)
    {
      uint32_t length_to_read;
      length_to_read = COM_MIN((uint32_t)len, MODEM_MAX_RX_DATA_SIZE);
      socket_desc->state = COM_SOCKET_WAITING_RSP;

      do
      {
        event = osMessageGet(socket_desc->queue, 0U);
        if (event.status == osEventMessage)
        {
          PrintDBG("rcv cleanup MSGqueue")
        }
      } while (event.status == osEventMessage);

      if (flags == COM_MSG_DONTWAIT)
      {
        /* Application don't want to wait if there is no data available */
        len_rcv = osCDS_socket_receive(socket_desc->id,
                                       buf, length_to_read);
        result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
        socket_desc->state = COM_SOCKET_CONNECTED;
        PrintINFO("rcv data DONTWAIT")
      }
      else
      {
        /* Maybe still some data available
           because application don't read all data with previous calls */
        PrintDBG("rcv data waiting")
        len_rcv = osCDS_socket_receive(socket_desc->id,
                                       buf, length_to_read);
        PrintDBG("rcv data waiting exit")

        if (len_rcv == 0)
        {
          /* Waiting for Distant response or Closure Socket or Timeout */
          event = osMessageGet(socket_desc->queue,
                               socket_desc->rcv_timeout);
          if (event.status == osEventTimeout)
          {
            result = COM_SOCKETS_ERR_TIMEOUT;
            socket_desc->state = COM_SOCKET_CONNECTED;
            PrintINFO("rcv data exit timeout")
          }
          else
          {
            msg = event.value.v;

            if (GET_SOCKET_MSG_TYPE(msg) == SOCKET_MSG)
            {
              switch (GET_SOCKET_MSG_ID(msg))
              {
                case DATA_RCV :
                {
                  len_rcv = osCDS_socket_receive(socket_desc->id,
                                                 buf, (uint32_t)length_to_read);
                  result = (len_rcv < 0) ? \
                           COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
                  socket_desc->state = COM_SOCKET_CONNECTED;
                  if (len_rcv == 0)
                  {
                    PrintDBG("rcv data exit with no data")
                  }
                  PrintINFO("rcv data exit with data")
                  break;
                }
                case CLOSING_RCV :
                {
                  result = COM_SOCKETS_ERR_CLOSING;
                  socket_desc->state = COM_SOCKET_CLOSING;
                  PrintINFO("rcv data exit socket closing")
                  break;
                }
                default :
                {
                  /* Impossible case */
                  result = COM_SOCKETS_ERR_GENERAL;
                  socket_desc->state = COM_SOCKET_CONNECTED;
                  PrintERR("rcv data exit NOK impossible case")
                  break;
                }
              }
            }
            else
            {
              /* Impossible case */
              result = COM_SOCKETS_ERR_GENERAL;
              socket_desc->state = COM_SOCKET_CONNECTED;
              PrintERR("rcv data msg NOK impossible case")
            }
          }
        }
        else
        {
          result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
          socket_desc->state = COM_SOCKET_CONNECTED;
          PrintINFO("rcv data exit data available or err low level")
        }
      }

      do
      {
        event = osMessageGet(socket_desc->queue, 0U);
        if (event.status == osEventMessage)
        {
          PrintDBG("rcv data exit cleanup MSGqueue")
        }
      } while (event.status == osEventMessage);
    }
    else
    {
      PrintERR("rcv data NOK err state")
      if (socket_desc->state < COM_SOCKET_CONNECTED)
      {
        result = COM_SOCKETS_ERR_STATE;
      }
      else
      {
        result = (socket_desc->state == COM_SOCKET_CLOSING) ? \
                 COM_SOCKETS_ERR_CLOSING : COM_SOCKETS_ERR_INPROGRESS;
      }
    }

    /*com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? \
                                 COM_SOCKET_STAT_RCV_OK : COM_SOCKET_STAT_RCV_NOK);*/
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return ((result == COM_SOCKETS_ERR_OK) ? len_rcv : result);
}


/*** Client - Server functionalities ******************************************/

/**
  * @brief  Socket send data to
  * @note   Send data on already connected socket
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to send
  * @note   see below
  * @param  len       - size of the data to send (in bytes)
  * @note   see below
  * @param  flags     - options
  * @note   - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         if len > interface between COM and low level
  *         a maximum of MODEM_MAX_TX_DATA_SIZE can be send
  *         - if flags = COM_MSG_WAIT, application accept to wait result
  *         of possible multiple send between COM and low level
  *         a buffer whose len > interface can be send
  *         COM will fragment the buffer according to the interface
  * @param  to        - remote IP address and port
  * @note   only an IPv4 address is supported
  * @param  tolen     - addr length
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_sendto_ip_modem(int32_t sock,
                            const com_char_t *buf, int32_t len,
                            int32_t flags,
                            const com_sockaddr_t *to, int32_t tolen)
{
  com_bool_t is_network_up;
  socket_addr_t socket_addr;
  socket_desc_t *socket_desc;
  int32_t result;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);

  if ((socket_desc != NULL)
      && (buf != NULL)
      && (len > 0))
  {
    if (socket_desc->type == (uint8_t)COM_SOCK_STREAM)
    {
      result = com_send_ip_modem(sock, buf, len, flags);
    }
    else /* socket_desc->type == (uint8_t)COM_SOCK_DGRAM */
    {
      if (UDP_SERVICE_SUPPORTED == 0U)
      {
        result = COM_SOCKETS_ERR_UNSUPPORTED;
      }
      else
      {
        /* Check remote addr is valid */
        if ((to != NULL) && (tolen != 0))
        {
          if (com_translate_ip_address(to, tolen,
                                       &socket_addr)
              == COM_SOCKETS_TRUE)
          {
            result = COM_SOCKETS_ERR_OK;
          }
          /* else result = COM_SOCKETS_ERR_PARAMETER */
        }
        else if ((to == NULL) && (tolen == 0)
                 && (socket_desc->remote_addr.addr != 0U))
        {
          com_sockaddr_in_t sockaddr_in;
          com_ip_addr_t remote_addr;
          uint16_t remote_port;

          remote_addr.addr = socket_desc->remote_addr.addr;
          remote_port = socket_desc->remote_port;
          com_convert_ipaddr_port_to_sockaddr(&remote_addr,
                                              remote_port,
                                              &sockaddr_in);

          if (com_translate_ip_address((com_sockaddr_t *)&sockaddr_in,
                                       (int32_t)sizeof(sockaddr_in),
                                       &socket_addr)
              == COM_SOCKETS_TRUE)
          {
            result = COM_SOCKETS_ERR_OK;
          }
          else
          {
            /* else result = COM_SOCKETS_ERR_PARAMETER */
          }
        }
        else
        {
          /* else result = COM_SOCKETS_ERR_PARAMETER */
        }

        if (result == COM_SOCKETS_ERR_OK)
        {
          /* If socket state == CREATED implicit bind and connect must be done */
          /* Without updating internal parameters
             => com_ip_modem_connect must not be called */
          result = com_ip_modem_connect_udp_service(socket_desc);

          /* closing maybe received, refuse to send data */
          if ((result == COM_SOCKETS_ERR_OK)
              && (socket_desc->closing == COM_SOCKETS_FALSE)
              && (socket_desc->state == COM_SOCKET_CONNECTED))
          {
            /* network maybe down, refuse to send data */
            if (com_ip_modem_is_network_up() == COM_SOCKETS_FALSE)
            {
              result = COM_SOCKETS_ERR_NONETWORK;
              PrintERR("sndto data NOK no network")
            }
            else
            {
              uint32_t length_to_send;
              uint32_t length_send;

              result = COM_SOCKETS_ERR_GENERAL;
              length_send = 0U;
              socket_desc->state = COM_SOCKET_SENDING;

              if (flags == COM_MSG_DONTWAIT)
              {
                length_to_send = COM_MIN((uint32_t)len, MODEM_MAX_TX_DATA_SIZE);
                if (osCDS_socket_sendto(socket_desc->id,
                                        buf, length_to_send,
                                        socket_addr.ip_type,
                                        socket_addr.ip_value,
                                        socket_addr.port)
                    == CELLULAR_OK)
                {
                  length_send = length_to_send;
                  result = (int32_t)length_send;
                  PrintINFO("sndto data DONTWAIT ok")
                }
                else
                {
                  PrintERR("sndto data DONTWAIT NOK at low level")
                }
                socket_desc->state = COM_SOCKET_CONNECTED;
              }
              else
              {
                is_network_up = com_ip_modem_is_network_up();
                /* Send all data of a big buffer - Whatever the size */
                while ((length_send != (uint32_t)len)
                       && (socket_desc->closing == COM_SOCKETS_FALSE)
                       && (is_network_up == COM_SOCKETS_TRUE)
                       && (socket_desc->state == COM_SOCKET_SENDING))
                {
                  length_to_send = COM_MIN((((uint32_t)len) - length_send),
                                           MODEM_MAX_TX_DATA_SIZE);
                  /* A tempo is already managed at low-level */
                  if (osCDS_socket_sendto(socket_desc->id,
                                          buf + length_send,
                                          length_to_send,
                                          socket_addr.ip_type,
                                          socket_addr.ip_value,
                                          socket_addr.port)
                      == CELLULAR_OK)
                  {
                    length_send += length_to_send;
                    PrintINFO("sndto data ok")
                    /* Update Network status */
                    is_network_up = com_ip_modem_is_network_up();
                  }
                  else
                  {
                    socket_desc->state = COM_SOCKET_CONNECTED;
                    PrintERR("sndto data NOK at low level")
                  }
                }
                socket_desc->state = COM_SOCKET_CONNECTED;
                result = (int32_t)length_send;
              }
            }
          }
          else
          {
            if (socket_desc->closing == COM_SOCKETS_TRUE)
            {
              PrintERR("sndto data NOK socket closing")
              result = COM_SOCKETS_ERR_CLOSING;
            }
            else
            {
              /* else result already updated com_ip_modem_connect_udp_service */
            }
          }

          /*com_sockets_statistic_update((result >= 0) ? \
                                       COM_SOCKET_STAT_SND_OK : COM_SOCKET_STAT_SND_NOK);*/
        }
        else
        {
          /* result = COM_SOCKETS_ERR_PARAMETER */
        }
      }
    }
  }

  if (result >= 0)
  {
    SOCKET_SET_ERROR(socket_desc, COM_SOCKETS_ERR_OK);
  }
  else
  {
    SOCKET_SET_ERROR(socket_desc, result);
  }

  return (result);
}

/**
  * @brief  Socket receive from data
  * @note   Receive data from a remote host
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  buf       - pointer to application data buffer to store the data to
  * @note   see below
  * @param  len       - size of application data buffer (in bytes)
  * @note   even if len > interface between COM and low level
  *         a maximum of the interface capacity can be received
  *         at each function call
  * @param  flags     - options
  * @note   - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         until data are available at low level
  *         - if flags = COM_MSG_WAIT, application accept to wait
  *         until data are available at low level with respect of potential
  *         timeout COM_SO_RCVTIMEO setting
  * @param  from      - remote IP address and port
  * @note   only an IPv4 address is supported
  *         if information is reported by the modem
  *         elsif value 0 is returned as remote IP address and port
  * @param  fromlen   - addr length
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recvfrom_ip_modem(int32_t sock,
                              com_char_t *buf, int32_t len,
                              int32_t flags,
                              com_sockaddr_t *from, int32_t *fromlen)
{
  int32_t result;
  int32_t len_rcv;
  osEvent event;
  socket_desc_t *socket_desc;
  socket_msg_t   msg;
  CS_IPaddrType_t ip_addr_type;
  CS_CHAR_t       ip_addr_value[40];
  uint16_t        ip_remote_port;

  result = COM_SOCKETS_ERR_PARAMETER;
  len_rcv = 0;
  socket_desc = com_ip_modem_find_socket(sock,
                                         COM_SOCKETS_FALSE);

  ip_remote_port = 0U;
  ip_addr_type = CS_IPAT_INVALID;
  (void)strcpy((CSIP_CHAR_t *)&ip_addr_value[0],
               (const CSIP_CHAR_t *)"0.0.0.0");

  if ((socket_desc != NULL)
      && (buf != NULL)
      && (len > 0))
  {
    if (socket_desc->type == (uint8_t)COM_SOCK_STREAM)
    {
      result = com_recv_ip_modem(sock, buf, len, flags);
      /* If data received set remote addr to the connected one */
      if ((result > 0)
          && (from != NULL)
          && (fromlen != NULL)
          && (*fromlen >= (int32_t)sizeof(com_sockaddr_in_t)))
      {
        com_ip_addr_t remote_addr;
        uint16_t remote_port;

        remote_addr.addr = socket_desc->remote_addr.addr;
        remote_port = socket_desc->remote_port;

        com_convert_ipaddr_port_to_sockaddr(&remote_addr,
                                            remote_port,
                                            (com_sockaddr_in_t *)from);

        *fromlen = (int32_t)sizeof(com_sockaddr_in_t);
      }
    }
    else /* socket_desc->type == (uint8_t)COM_SOCK_DGRAM */
    {
      if (UDP_SERVICE_SUPPORTED == 0U)
      {
        result = COM_SOCKETS_ERR_UNSUPPORTED;
      }
      else
      {
        /* If socket state == CREATED implicit bind and connect must be done */
        /* Without updating internal parameters
           => com_ip_modem_connect must not be called */
        result = com_ip_modem_connect_udp_service(socket_desc);

        /* closing maybe received, refuse to send data */
        if ((result == COM_SOCKETS_ERR_OK)
            && (socket_desc->state == COM_SOCKET_CONNECTED))
        {
          uint32_t length_to_read;
          length_to_read = COM_MIN((uint32_t)len, MODEM_MAX_RX_DATA_SIZE);
          socket_desc->state = COM_SOCKET_WAITING_FROM;

          do
          {
            event = osMessageGet(socket_desc->queue, 0U);
            if (event.status == osEventMessage)
            {
              PrintDBG("rcvfrom cleanup MSGqueue ")
            }
          } while (event.status == osEventMessage);

          if (flags == COM_MSG_DONTWAIT)
          {
            /* Application don't want to wait if there is no data available */
            len_rcv = osCDS_socket_receivefrom(socket_desc->id,
                                               buf, length_to_read,
                                               &ip_addr_type,
                                               &ip_addr_value[0],
                                               &ip_remote_port);
            result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
            socket_desc->state = COM_SOCKET_CONNECTED;
            PrintINFO("rcvfrom data DONTWAIT")
          }
          else
          {
            /* Maybe still some data available
               because application don't read all data with previous calls */
            PrintDBG("rcvfrom data waiting")
            len_rcv = osCDS_socket_receivefrom(socket_desc->id,
                                               buf, length_to_read,
                                               &ip_addr_type,
                                               &ip_addr_value[0],
                                               &ip_remote_port);
            PrintDBG("rcvfrom data waiting exit")

            if (len_rcv == 0)
            {
              /* Waiting for Distant response or Closure Socket or Timeout */
              PrintDBG("rcvfrom data waiting on MSGqueue")
              event = osMessageGet(socket_desc->queue,
                                   socket_desc->rcv_timeout);
              PrintDBG("rcvfrom data exit from MSGqueue")
              if (event.status == osEventTimeout)
              {
                result = COM_SOCKETS_ERR_TIMEOUT;
                socket_desc->state = COM_SOCKET_CONNECTED;
                PrintINFO("rcvfrom data exit timeout")
              }
              else
              {
                msg = event.value.v;

                if (GET_SOCKET_MSG_TYPE(msg) == SOCKET_MSG)
                {
                  switch (GET_SOCKET_MSG_ID(msg))
                  {
                    case DATA_RCV :
                    {
                      len_rcv = osCDS_socket_receivefrom(socket_desc->id,
                                                         buf, (uint32_t)len,
                                                         &ip_addr_type,
                                                         &ip_addr_value[0],
                                                         &ip_remote_port);
                      result = (len_rcv < 0) ? \
                               COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
                      socket_desc->state = COM_SOCKET_CONNECTED;
                      if (len_rcv == 0)
                      {
                        PrintDBG("rcvfrom data exit with no data")
                      }
                      PrintINFO("rcvfrom data exit with data")
                      break;
                    }
                    case CLOSING_RCV :
                    {
                      result = COM_SOCKETS_ERR_CLOSING;
                      /* socket_desc->state = COM_SOCKET_CLOSING; */
                      socket_desc->state = COM_SOCKET_CONNECTED;
                      PrintINFO("rcvfrom data exit socket closing")
                      break;
                    }
                    default :
                    {
                      /* Impossible case */
                      result = COM_SOCKETS_ERR_GENERAL;
                      socket_desc->state = COM_SOCKET_CONNECTED;
                      PrintERR("rcvfrom data exit NOK impossible case")
                      break;
                    }
                  }
                }
                else
                {
                  /* Impossible case */
                  result = COM_SOCKETS_ERR_GENERAL;
                  socket_desc->state = COM_SOCKET_CONNECTED;
                  PrintERR("rcvfrom data msg NOK impossible case")
                }
              }
            }
            else
            {
              result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
              socket_desc->state = COM_SOCKET_CONNECTED;
              PrintINFO("rcvfrom data exit data available or err low level")
            }
          }

          do
          {
            event = osMessageGet(socket_desc->queue, 0U);
            if (event.status == osEventMessage)
            {
              PrintDBG("rcvfrom data exit cleanup MSGqueue")
            }
          } while (event.status == osEventMessage);
        }
        else
        {
          result = (socket_desc->state == COM_SOCKET_CLOSING) ? \
                   COM_SOCKETS_ERR_CLOSING : COM_SOCKETS_ERR_INPROGRESS;
        }

        /*com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? \
                                     COM_SOCKET_STAT_RCV_OK : COM_SOCKET_STAT_RCV_NOK);*/
      }
    }

    if ((len_rcv > 0)
        && (from != NULL)
        && (fromlen != NULL)
        && (*fromlen >= (int32_t)sizeof(com_sockaddr_in_t)))
    {
      if (COM_SOCKETS_TRUE
          == com_convert_IPString_to_sockaddr(ip_remote_port,
                                              (com_char_t *)(&ip_addr_value[0]),
                                              from))
      {
        *fromlen = (int32_t)sizeof(com_sockaddr_in_t);
      }
      else
      {
        *fromlen = 0;
      }
    }
  }

  SOCKET_SET_ERROR(socket_desc, result);
  return ((result == COM_SOCKETS_ERR_OK) ? len_rcv : result);
}


/*** Server functionalities - NOT yet supported *******************************/

/**
  * @brief  Socket listen
  * @note   Set socket in listening mode
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  backlog   - number of connection requests that can be queued
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_listen_ip_modem(int32_t sock,
                            int32_t backlog)
{
  UNUSED(sock);
  UNUSED(backlog);
  return COM_SOCKETS_ERR_UNSUPPORTED;
}

/**
  * @brief  Socket accept
  * @note   Accept a connect request for a listening socket
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  addr      - IP address and port number of the accepted connection
  * @param  len       - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_accept_ip_modem(int32_t sock,
                            com_sockaddr_t *addr, int32_t *addrlen)
{
  UNUSED(sock);
  UNUSED(addr);
  UNUSED(addrlen);
  return COM_SOCKETS_ERR_UNSUPPORTED;
}


/*** Other functionalities ****************************************************/

/**
  * @brief  Component initialization
  * @note   must be called only one time and
  *         before using any other functions of com_*
  * @param  None
  * @retval bool      - true/false init ok/nok
  */
com_bool_t com_init_ip_modem(void)
{
  com_bool_t result;

  result = COM_SOCKETS_FALSE;

#if (USE_COM_PING == 1)
  ping_socket_id = COM_SOCKET_INVALID_ID;
#endif /* USE_COM_PING == 1 */

  for (uint8_t i = 0U; i < COM_SOCKET_LOCAL_ID_NB; i++)
  {
    socket_local_id[i] = COM_SOCKETS_FALSE; /* set socket local id to unused */
  }
  osMutexDef(ComSocketsMutex);
  ComSocketsMutexHandle = osMutexCreate(osMutex(ComSocketsMutex));
  if (ComSocketsMutexHandle != NULL)
  {
    /* Create always the first element of the list */
    socket_desc_list = com_ip_modem_create_socket_desc();
    if (socket_desc_list != NULL)
    {
      result = COM_SOCKETS_TRUE;
    }

#if (USE_DATACACHE == 1)
    network_is_up = COM_SOCKETS_FALSE;
#endif /* USE_DATACACHE == 1 */
  }

  local_port = 0U;

  return result;
}


/**
  * @brief  Component start
  * @note   must be called only one time but
  *         after com_init and dc_start
  *         and before using any other functions of com_*
  * @param  None
  * @retval None
  */
void com_start_ip_modem(void)
{
  uint32_t random;
#if (USE_DATACACHE == 1)
  /* Datacache registration for netwok on/off status */
  (void)dc_com_register_gen_event_cb(&dc_com_db,
                                     com_socket_datacache_cb,
                                     (void *) NULL);
#endif /* USE_DATACACHE == 1 */

  /* Initialize local port to a random value */
  if (HAL_OK != HAL_RNG_GenerateRandomNumber(&hrng, &random))
  {
    random = (uint32_t)rand();
  }
  random = random & ~LOCAL_PORT_BEGIN;
  random = random + LOCAL_PORT_BEGIN;
  local_port = (uint16_t)(random);
}


/**
  * @brief  Get host IP from host name
  * @note   Retrieve host IP address from host name
  *         DNS resolver is a fix value in the module
  *         only a primary DNS is used see com_primary_dns_addr_str value
  * @param  name      - host name
  * @param  addr      - host IP corresponding to host name
  * @note   only IPv4 address is managed
  * @retval int32_t   - ok or error value
  */
int32_t com_gethostbyname_ip_modem(const com_char_t *name,
                                   com_sockaddr_t   *addr)
{
  int32_t result;
  CS_PDN_conf_id_t PDN_conf_id;
  CS_DnsReq_t  dns_req;
  CS_DnsResp_t dns_resp;

  PDN_conf_id = CS_PDN_CONFIG_DEFAULT;
  result = COM_SOCKETS_ERR_PARAMETER;

  if ((name != NULL)
      && (addr != NULL))
  {
    if (strlen((const CSIP_CHAR_t *)name) <= sizeof(dns_req.host_name))
    {
      (void)strcpy((CSIP_CHAR_t *)&dns_req.host_name[0],
                   (const CSIP_CHAR_t *)name);

      result = COM_SOCKETS_ERR_GENERAL;
      if (osCDS_dns_request(PDN_conf_id,
                            &dns_req,
                            &dns_resp)
          == CELLULAR_OK)
      {
        PrintINFO("DNS resolution OK - Remote: %s IP: %s", name, dns_resp.host_addr)
        if (com_convert_IPString_to_sockaddr(0U,
                                             (com_char_t *)&dns_resp.host_addr[0],
                                             addr)
            == COM_SOCKETS_TRUE)
        {
          PrintDBG("DNS conversion OK")
          result = COM_SOCKETS_ERR_OK;
        }
        else
        {
          PrintERR("DNS conversion NOK")
        }
      }
      else
      {
        PrintERR("DNS resolution NOK for %s", name)
      }
    }
  }

  return (result);
}

/**
  * @brief  Get peer name
  * @note   Retrieve IP address and port number
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  name      - IP address and port number of the peer
  * @param  namelen   - name length
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_getpeername_ip_modem(int32_t sock,
                                 com_sockaddr_t *name, int32_t *namelen)
{
  UNUSED(sock);
  UNUSED(name);
  UNUSED(namelen);
  return COM_SOCKETS_ERR_UNSUPPORTED;
}

/**
  * @brief  Get sock name
  * @note   Retrieve local IP address and port number
  *         NOT yet supported
  * @param  sock      - socket handle obtained with com_socket
  * @note   socket handle on which operation has to be done
  * @param  name      - IP address and port number
  * @param  namelen   - name length
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_getsockname_ip_modem(int32_t sock,
                                 com_sockaddr_t *name, int32_t *namelen)
{
  UNUSED(sock);
  UNUSED(name);
  UNUSED(namelen);
  return COM_SOCKETS_ERR_UNSUPPORTED;
}

#if (USE_COM_PING == 1)
/**
  * @brief  Ping handle creation
  * @note   Create a ping session
  * @param  None
  * @retval int32_t  - ping handle or error value
  */
int32_t com_ping_ip_modem(void)
{
  int32_t result;
  socket_desc_t *socket_desc;

  /* Need to create a new socket_desc ? */
  socket_desc = com_ip_modem_provide_socket_desc(COM_SOCKETS_TRUE);
  if (socket_desc == NULL)
  {
    result = COM_SOCKETS_ERR_NOMEMORY;
    PrintERR("create ping NOK no memory")
    /* Socket descriptor is not existing in COM
       and nothing to do at low level */
  }
  else
  {
    /* Socket state is set directly to CREATED
       because nothing else as to be done */
    result = COM_SOCKETS_ERR_OK;
    socket_desc->state = COM_SOCKET_CREATED;
  }

  return ((result == COM_SOCKETS_ERR_OK) ? socket_desc->id : result);
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
int32_t com_ping_process_ip_modem(int32_t ping,
                                  const com_sockaddr_t *addr, int32_t addrlen,
                                  uint8_t timeout, com_ping_rsp_t *rsp)
{
  int32_t  result;
  uint32_t timeout_ms;
  socket_desc_t *socket_desc;
  socket_addr_t  socket_addr;
  CS_Ping_params_t ping_params;
  CS_PDN_conf_id_t PDN_conf_id;
  osEvent event;
  socket_msg_t  msg;

  result = COM_SOCKETS_ERR_PARAMETER;

  socket_desc = com_ip_modem_find_socket(ping,
                                         COM_SOCKETS_TRUE);

  if (socket_desc != NULL)
  {
    /* No need each time to close the connection */
    if ((socket_desc->state == COM_SOCKET_CREATED)
        || (socket_desc->state == COM_SOCKET_CONNECTED))
    {
      if ((timeout != 0U)
          && (rsp != NULL)
          && (addr != NULL))
      {
        if (com_translate_ip_address(addr, addrlen,
                                     &socket_addr)
            == COM_SOCKETS_TRUE)
        {
          /* Check Network status */
          if (com_ip_modem_is_network_up() == COM_SOCKETS_TRUE)
          {
            PDN_conf_id = CS_PDN_CONFIG_DEFAULT;
            ping_params.timeout = timeout;
            ping_params.pingnum = 1U;
            (void)strcpy((CSIP_CHAR_t *)&ping_params.host_addr[0],
                         (const CSIP_CHAR_t *)&socket_addr.ip_value[0]);
            socket_desc->state = COM_SOCKET_WAITING_RSP;
            socket_desc->rsp   =  rsp;
            if (osCDS_ping(PDN_conf_id,
                           &ping_params,
                           com_ip_modem_ping_rsp_cb)
                == CELLULAR_OK)
            {
              ping_socket_id = socket_desc->id;
              timeout_ms = (uint32_t)timeout * 1000U;
              /* Waiting for Response or Timeout */
              event = osMessageGet(socket_desc->queue,
                                   timeout_ms);
              if (event.status == osEventTimeout)
              {
                result = COM_SOCKETS_ERR_TIMEOUT;
                socket_desc->state = COM_SOCKET_CONNECTED;
                PrintINFO("ping exit timeout")
              }
              else
              {
                msg = event.value.v;

                if (GET_SOCKET_MSG_TYPE(msg) == PING_MSG)
                {
                  switch (GET_SOCKET_MSG_ID(msg))
                  {
                    case DATA_RCV :
                    {
                      result = COM_SOCKETS_ERR_OK;
                      /* Format the data */
                      socket_desc->state = COM_SOCKET_CONNECTED;
                      break;
                    }
                    case CLOSING_RCV :
                    {
                      /* Impossible case */
                      result = COM_SOCKETS_ERR_GENERAL;
                      socket_desc->state = COM_SOCKET_CONNECTED;
                      PrintERR("rcv data exit NOK closing case")
                      break;
                    }
                    default :
                    {
                      /* Impossible case */
                      result = COM_SOCKETS_ERR_GENERAL;
                      socket_desc->state = COM_SOCKET_CONNECTED;
                      PrintERR("rcv data exit NOK impossible case")
                      break;
                    }
                  }
                }
                else
                {
                  /* Impossible case */
                  result = COM_SOCKETS_ERR_GENERAL;
                  socket_desc->state = COM_SOCKET_CONNECTED;
                  PrintERR("rcv socket msg NOK impossible case")
                }
              }
            }
            else
            {
              socket_desc->state = COM_SOCKET_CONNECTED;
              result = COM_SOCKETS_ERR_GENERAL;
              PrintERR("ping send NOK at low level")
            }
          }
          else
          {
            result = COM_SOCKETS_ERR_NONETWORK;
            PrintERR("ping send NOK no network")
          }
        }
      }
    }
    else
    {
      result = COM_SOCKETS_ERR_STATE;
      PrintERR("ping send NOK state invalid")
    }
  }

  return result;
}

/**
  * @brief  Ping close
  * @note   Close a ping session and release ping handle
  * @param  ping      - ping handle obtained with com_socket
  * @note   ping handle on which operation has to be done
  * @retval int32_t   - ok or error value
  */
int32_t com_closeping_ip_modem(int32_t ping)
{
  int32_t result;
  const socket_desc_t *socket_desc;

  result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc = com_ip_modem_find_socket(ping,
                                         COM_SOCKETS_TRUE);

  if (socket_desc != NULL)
  {
    /* If socket is currently under process refused to close it */
    if ((socket_desc->state == COM_SOCKET_SENDING)
        || (socket_desc->state == COM_SOCKET_WAITING_RSP))
    {
      PrintERR("close ping NOK err state")
      result = COM_SOCKETS_ERR_INPROGRESS;
    }
    else
    {
      com_ip_modem_delete_socket_desc(ping, COM_SOCKETS_TRUE);
      result = COM_SOCKETS_ERR_OK;
      PrintINFO("close ping ok")
    }
  }

  return result;
}

#endif /* USE_COM_PING == 1 */

#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
