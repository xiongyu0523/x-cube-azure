/**
  ******************************************************************************
  * @file    stack_analysis.c
  * @author  MCD Application Team
  * @brief   This file implements Stack analysis debug facilities
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
#include "stack_analysis.h"
#include <stdio.h>
#include <string.h>
#include "cmsis_os_misrac2012.h"
#if (USE_CMD_CONSOLE == 1)
#include "cmd.h"
#endif /* (USE_CMD_CONSOLE == 1) */


/* Private defines -----------------------------------------------------------*/
#if (STACK_ANALYSIS_TRACE == 1)
#define USE_TRACE_STACK_ANALYSIS      (1)
#else
#define USE_TRACE_STACK_ANALYSIS      (0)
#endif /* STACK_ANALYSIS_TRACE */

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_STACK_ANALYSIS == 1)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PrintForce(format, args...) \
  TracePrintForce(DBG_CHAN_MAIN, DBL_LVL_P0, "" format "\n\r", ## args)
#define PrintINFO(format, args...) \
  TracePrint(DBG_CHAN_MAIN, DBL_LVL_P0, "SA:" format "\n\r", ## args)
#define PrintDBG(format, args...)  \
  TracePrint(DBG_CHAN_MAIN, DBL_LVL_P1, "SA:" format "\n\r", ## args)
#define PrintERR(format, args...)  \
  TracePrint(DBG_CHAN_MAIN, DBL_LVL_ERR, "SA ERROR:" format "\n\r", ## args)
#else
#define PrintForce(format, args...) printf("" format "\n\r", ## args);
#define PrintINFO(format, args...) printf("SA:" format "\n\r", ## args);
#define PrintDBG(format, args...)  do {} while(0);
#define PrintERR(format, args...)  printf("SA ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PrintForce(format, args...) do {} while(0);
#define PrintINFO(format, args...)  do {} while(0);
#define PrintDBG(format, args...)   do {} while(0);
#define PrintERR(format, args...)   do {} while(0);
#endif /* USE_TRACE_STACK_ANALYSIS */

#define STACK_ANALYSIS_MIN(a,b) (((a) < (b)) ? (a) : (b))

/* Private typedef -----------------------------------------------------------*/
typedef char SA_CHAR_t; /* used in stdio.h and string.h service call */

typedef struct
{
  void *TaskHandle;
  uint16_t stackSizeAtCreation;
  uint16_t stackSizeFree;
} TaskAnalysis_t;

/* Private define ------------------------------------------------------------*/
#define tskBLOCKED_CHAR   (SA_CHAR_t)('B')
#define tskREADY_CHAR     (SA_CHAR_t)('R')
#define tskDELETED_CHAR   (SA_CHAR_t)('D')
#define tskSUSPENDED_CHAR (SA_CHAR_t)('S')
#define tskUNKNOWN_CHAR   (SA_CHAR_t)(' ')

/* Private variables ---------------------------------------------------------*/
static TaskAnalysis_t TaskAnalysisList[STACK_ANALYSIS_TASK_MAX];
static uint8_t TaskAnalysisListNb;
static uint16_t GlobalstackSizeFree;

#if (USE_CMD_CONSOLE == 1)
static uint8_t *stackAnalysis_cmd_label = (uint8_t *)"stack";
#endif /* USE_CMD_CONSOLE == 1 */

/* Private function prototypes -----------------------------------------------*/
static bool getStackSizeByHandle(
  const TaskHandle_t *TaskHandle,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice);

static bool getStackSizeByName(
  const SA_CHAR_t *TaskName,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice);

static void setStackSize(uint8_t  indice,
                         uint16_t stackSizeFree);

static void formatTaskName(SA_CHAR_t *pcBuffer,
                           size_t size_max,
                           const SA_CHAR_t *pcTaskName);

#if (USE_CMD_CONSOLE == 1)
static cmd_status_t stackAnalysis_cmd(uint8_t *cmd_line_p);
#endif /* USE_CMD_CONSOLE == 1 */

/* Private functions ---------------------------------------------------------*/


#if (USE_CMD_CONSOLE == 1)
static cmd_status_t stackAnalysis_cmd(uint8_t *cmd_line_p)
{
  uint32_t argc;
  uint8_t  *argv_p[10];
  const uint8_t *cmd_p;

  PrintForce()

  cmd_p = (uint8_t *)strtok((SA_CHAR_t *)cmd_line_p, " \t");

  if (strncmp((const SA_CHAR_t *)cmd_p,
              (const SA_CHAR_t *)stackAnalysis_cmd_label,
              strlen((const SA_CHAR_t *)cmd_p))
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
      PrintForce("%s argument missing", stackAnalysis_cmd_label)
    }
    /*  1st parameter analysis */
    else if (strncmp((SA_CHAR_t *)argv_p[0],
                     "help",
                     strlen((const SA_CHAR_t *)argv_p[0])) == 0)
    {
      PrintForce("***** %s help *****", (SA_CHAR_t *)stackAnalysis_cmd_label)
      PrintForce("%s help", stackAnalysis_cmd_label)
      PrintForce("%s state", stackAnalysis_cmd_label)
    }
    else if (strncmp((SA_CHAR_t *)argv_p[0],
                     "state",
                     strlen((const SA_CHAR_t *)argv_p[0]))
             == 0)
    {
      (void)stackAnalysis_trace(true);
    }
    else
    {
      PrintForce("%s  bad parameter %s>>>\n\r", cmd_p, argv_p[0])
    }
  }
  return CMD_OK;
}
#endif /* USE_CMD_CONSOLE == 1 */

static bool getStackSizeByHandle(
  const TaskHandle_t *TaskHandle,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice)
{
  uint8_t i;
  bool found;

  i = 0U;
  found = false;
  while ((found == false)
         && (i < TaskAnalysisListNb))
  {
    if (TaskHandle == TaskAnalysisList[i].TaskHandle)
    {
      found = true;
      *stackSizeAtCreation = TaskAnalysisList[i].stackSizeAtCreation;
      *stackSizeFree = TaskAnalysisList[i].stackSizeFree;
      *indice = i;
    }
    else
    {
      i++;
    }
  }

  return found;
}

static bool getStackSizeByName(
  const SA_CHAR_t *TaskName,
  uint16_t *stackSizeAtCreation,
  uint16_t *stackSizeFree,
  uint8_t *indice)
{
  uint8_t i;
  bool found;

  i = 0U;
  found = false;

  while ((found == false)
         && (i < TaskAnalysisListNb))
  {
    if (strcmp(TaskName,
               (const SA_CHAR_t *)TaskAnalysisList[i].TaskHandle)
        == 0)
    {
      found = true;
      *stackSizeAtCreation = TaskAnalysisList[i].stackSizeAtCreation;
      *stackSizeFree = TaskAnalysisList[i].stackSizeFree;
      *indice = i;
    }
    else
    {
      i++;
    }
  }

  return found;
}

static void setStackSize(uint8_t  indice,
                         uint16_t stackSizeFree)
{
  if (indice < TaskAnalysisListNb)
  {
    TaskAnalysisList[indice].stackSizeFree = stackSizeFree;
  }
}

static void formatTaskName(SA_CHAR_t *pcBuffer,
                           size_t size_max,
                           const SA_CHAR_t *pcTaskName)
{
  size_t i;

  /* Start by copying the entire string. */
  (void)memcpy(pcBuffer,
               pcTaskName,
               STACK_ANALYSIS_MIN((size_max - 1U),
                                  strlen(pcTaskName)));

  /* Pad the end of the string with spaces to ensure columns line up
     when printed out. */
  for (i = (size_t)(STACK_ANALYSIS_MIN((size_max - 1U),
                                       strlen(pcTaskName)));
       i < (size_max - 1U);
       i++)
  {
    pcBuffer[i] = (SA_CHAR_t)(' ');
  }
  /* Terminate. */
  pcBuffer[i] = (SA_CHAR_t)(0x00);
}



/* Public functions ----------------------------------------------------------*/
bool stackAnalysis_addStackSizeByHandle(
  void *TaskHandle,
  uint16_t stackSizeAtCreation)
{
  bool result;

  if (TaskHandle == NULL)
  {
    PrintINFO("Error: NULL TaskHandle passed")
    result = false;
  }
  else
  {
    if ((int32_t)TaskAnalysisListNb < STACK_ANALYSIS_TASK_MAX)
    {
      TaskAnalysisList[TaskAnalysisListNb].TaskHandle = TaskHandle;
      TaskAnalysisList[TaskAnalysisListNb].stackSizeAtCreation = stackSizeAtCreation;
      /* Initialize at the maximum possible (even if it is already less) */
      TaskAnalysisList[TaskAnalysisListNb].stackSizeFree = stackSizeAtCreation;
      TaskAnalysisListNb++;
      result = true;
    }
    else
    {
      PrintINFO("Error: Too many tasks added %d",
                STACK_ANALYSIS_TASK_MAX)
      result = false;
    }
  }

  return result;
}

bool stackAnalysis_addStackSizeByName(
  uint8_t *TaskName,
  uint16_t stackSizeAtCreation)
{
  bool result;

  if (TaskName == NULL)
  {
    PrintINFO("Error: NULL TaskName passed")
    result = false;
  }
  else
  {
    if ((int32_t)TaskAnalysisListNb < STACK_ANALYSIS_TASK_MAX)
    {
      TaskAnalysisList[TaskAnalysisListNb].TaskHandle = TaskName;
      TaskAnalysisList[TaskAnalysisListNb].stackSizeAtCreation = stackSizeAtCreation;
      /* Initialize at the maximum possible (even if it is already less) */
      TaskAnalysisList[TaskAnalysisListNb].stackSizeFree = stackSizeAtCreation;
      TaskAnalysisListNb++;
      result = true;
    }
    else
    {
      PrintINFO("Error: Too many tasks added %d",
                STACK_ANALYSIS_TASK_MAX)
      result = false;
    }
  }

  return result;
}

bool stackAnalysis_trace(bool force)
{
  bool result;
  bool found;
  bool printed;
  uint8_t indice;
  uint16_t usStackSizeAtCreation;
  uint16_t usStackSizeFree;
  uint32_t i;
  uint32_t j;
  uint32_t ulTotalRunTime;
  uint32_t ulStatsAsPercentage;
  SA_CHAR_t xTaskName[STACK_ANALYSIS_TASK_NAME_MAX];
  SA_CHAR_t cTaskStatus;
  size_t freeHeap1;
  size_t freeHeap2;
  TaskStatus_t *pxTaskStatusArray;
  UBaseType_t uxArraySize;

  result = false;
  printed = false;
  usStackSizeFree = 0U;
  cTaskStatus = tskUNKNOWN_CHAR;

  uxArraySize = uxTaskGetNumberOfTasks();

  /* Before allocation GetFreeHeapSize() */
  freeHeap1 = xPortGetFreeHeapSize();

  /* Allocate a TaskStatus_t structure for each task. */
  pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

  if (pxTaskStatusArray != NULL)
  {
    /*
     * It is recommended that production systems call uxTaskGetSystemState()
     * directly to get access to raw stats data, rather than indirectly
     * through a call to vTaskList().
    */
    /* Generate raw status information about each task. */
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray,
                                       uxArraySize,
                                       &ulTotalRunTime);

    /* For percentage calculations. */
    ulTotalRunTime /= 100UL;

    /* After allocation GetFreeHeapSize() */
    freeHeap2 = xPortGetFreeHeapSize();

    if ((force == true)
        || (freeHeap2 < (((configTOTAL_HEAP_SIZE) * (TRACE_IF_TOTAL_FREE_HEAP_AT_X_PERCENT)) / 100U))
        || ((TRACE_ON_CHANGE == 1)
            && (freeHeap2 < GlobalstackSizeFree)))
    {
      printed = true;
      PrintINFO("<< Status  Begin >>")
      PrintINFO("Free Heap : %d=>%d (%d) (%d SA usage)", GlobalstackSizeFree,
                freeHeap2,
                configTOTAL_HEAP_SIZE,
                freeHeap1 - freeHeap2)
      GlobalstackSizeFree = (uint16_t)freeHeap2;
    }
    if (force == true)
    {
      PrintINFO("Task List (%lu): ", uxArraySize)
    }

    /* For each populated position in the pxTaskStatusArray array,
       format the raw data as human readable ASCII data */
    for (i = 0U; i < uxArraySize; i++)
    {
      uint32_t taskNumber;
      found = false;
      usStackSizeAtCreation = 0U;
      j = 0U;
      cTaskStatus = tskUNKNOWN_CHAR;

      /* Ordering Task according to their xTaskNumber */
      while ((found == false)
             && (j < uxArraySize))
      {
        taskNumber = pxTaskStatusArray[j].xTaskNumber;
        if (taskNumber == (i + 1U))
        {
          found = true;
        }
        else
        {
          j++;
        }
      }

      if (j < uxArraySize)
      {
        switch (pxTaskStatusArray[j].eCurrentState)
        {
          case eReady:
          {
            cTaskStatus = tskREADY_CHAR;
            break;
          }

          case eBlocked:
          {
            cTaskStatus = tskBLOCKED_CHAR;
            break;
          }

          case eSuspended:
          {
            cTaskStatus = tskSUSPENDED_CHAR;
            break;
          }

          case eDeleted:
          {
            cTaskStatus = tskDELETED_CHAR;
            break;
          }

          default:
          {
            /* Should not get here, but it is included
               to prevent static checking errors. */
            break;
          }
        }

        /* Avoid divide by zero errors. */
        if (ulTotalRunTime > 0U)
        {
          /* What percentage of the total run time has the task used?
             This will always be rounded down to the nearest integer.
             ulTotalRunTimeDiv100 has already been divided by 100. */
          ulStatsAsPercentage = pxTaskStatusArray[j].ulRunTimeCounter / ulTotalRunTime;
        }
        else
        {
          ulStatsAsPercentage = 0U;
        }

        formatTaskName(xTaskName,
                       (uint32_t)STACK_ANALYSIS_TASK_NAME_MAX,
                       (const SA_CHAR_t *)pxTaskStatusArray[j].pcTaskName);
        found = getStackSizeByHandle(pxTaskStatusArray[j].xHandle,
                                     &usStackSizeAtCreation,
                                     &usStackSizeFree,
                                     &indice);
        if (found == false)
        {
          found = getStackSizeByName((const SA_CHAR_t *)pxTaskStatusArray[j].pcTaskName,
                                     &usStackSizeAtCreation,
                                     &usStackSizeFree,
                                     &indice);
        }

        if ((force == true)
            || (found == false)
            || (pxTaskStatusArray[j].usStackHighWaterMark
                < (usStackSizeAtCreation * (uint16_t)TRACE_IF_THREAD_FREE_STACK_AT_X_PERCENT / (uint16_t)100))
            || ((TRACE_ON_CHANGE == 1)
                && (pxTaskStatusArray[j].usStackHighWaterMark < usStackSizeFree)))
        {
          if (printed == false)
          {
            PrintINFO("<< Status Begin >>")
          }

          if (ulStatsAsPercentage > 0UL)
          {
            printed = true;

            PrintINFO("%s n:%2lu FreeHeap:%4u=>%4u/%4u Prio:%lu State:%c Time:%lu  %lu%% ",
                      xTaskName,
                      pxTaskStatusArray[j].xTaskNumber,
                      usStackSizeFree,
                      pxTaskStatusArray[j].usStackHighWaterMark,
                      usStackSizeAtCreation,
                      pxTaskStatusArray[j].uxCurrentPriority,
                      cTaskStatus,
                      pxTaskStatusArray[j].ulRunTimeCounter,
                      ulStatsAsPercentage)

            if (found == true)
            {
              setStackSize(indice,
                           pxTaskStatusArray[j].usStackHighWaterMark);
            }
          }
          else
          {
            /* If the percentage is zero here then the task has
               consumed less than 1% of the total run time. */
#if ( configGENERATE_RUN_TIME_STATS == 1 )
            printed = true;
            PrintINFO("%s n:%2u FreeHeap:%4u=>%4u/%4u Prio:%u State:%c Time:<1%%",
                      xTaskName,
                      pxTaskStatusArray[j].xTaskNumber,
                      usStackSizeFree,
                      pxTaskStatusArray[j].usStackHighWaterMark,
                      usStackSizeAtCreation,
                      pxTaskStatusArray[j].uxCurrentPriority,
                      cTaskStatus)
            if (found == true)
            {
              setStackSize(indice,
                           pxTaskStatusArray[j].usStackHighWaterMark);
            }
#else
            printed = true;
            PrintINFO("%s n:%2lu FreeHeap:%4u=>%4u/%4u Prio:%lu State:%c ",
                      xTaskName,
                      pxTaskStatusArray[j].xTaskNumber,
                      usStackSizeFree,
                      pxTaskStatusArray[j].usStackHighWaterMark,
                      usStackSizeAtCreation,
                      pxTaskStatusArray[j].uxCurrentPriority,
                      cTaskStatus)
            if (found == true)
            {
              setStackSize(indice,
                           pxTaskStatusArray[j].usStackHighWaterMark);
            }
#endif /* configGENERATE_RUN_TIME_STATS == 1 */
          }
        }
      }
    }

    if (printed == true)
    {
      PrintINFO("<< Status End >>")
    }
    /* The array is no longer needed, free the memory it consumes. */
    vPortFree(pxTaskStatusArray);

    result = true;
  }
  else
  {
    PrintERR("NOT enough memory for study - task nb: %lu", uxArraySize)
  }

#if (USE_TRACE_STACK_ANALYSIS == 0) || (SW_DEBUG_VERSION == 0U)
  /* To avoid warning if trace not activated (variables are set but not used) */
  if ((cTaskStatus == tskUNKNOWN_CHAR)
      && (freeHeap1 == 0U))
  {
    /* Nothing to do */
  }
#endif /* (USE_TRACE_STACK_ANALYSIS == 0) || (SW_DEBUG_VERSION == 0U) */

  return (result);
}


void stackAnalysis_init(void)
{
  TaskAnalysisListNb = 0U;
  /* Initialize at the maximum possible value */
  GlobalstackSizeFree = (uint16_t)(configTOTAL_HEAP_SIZE);
}

void stackAnalysis_start(void)
{
#if (USE_CMD_CONSOLE == 1)
  CMD_Declare(stackAnalysis_cmd_label, stackAnalysis_cmd, (uint8_t *)"stack analysis");
#endif /* USE_CMD_CONSOLE == 1 */
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
