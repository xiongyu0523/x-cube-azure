/**
  ******************************************************************************
  * @file    test_macro.h
  * @author  MCD Application Team
  * @brief   test macros
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#ifndef TEST_MACRO_H
#define TEST_MACRO_H

#define TEST_SUCCESS 0
#define TEST_RUN(test) { \
    int rc = -1; \
    printf("\n" #test " starting \n\n"); \
    rc = (test)(); \
    if (rc != TEST_SUCCESS) \
    { \
      printf("\n" #test " failed with rc=%d\n\n", rc); \
    } else { \
      printf("\n" #test " successful\n\n"); \
    } \
  }

#define TEST_RUN_ARG(test,a) { \
    int rc = -1; \
    printf("\n" #test " starting \n\n"); \
    rc = (test)(a); \
    if (rc != TEST_SUCCESS) \
    { \
      printf("\n" #test " failed with rc=%d\n\n", rc); \
    } else { \
      printf("\n" #test " successful\n\n"); \
    } \
  }
  
#endif /* TEST_MACRO_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
