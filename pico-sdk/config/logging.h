#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>

#define LOG_NONE  0
#define LOG_ERROR 1
#define LOG_WARN  2
#define LOG_INFO  3
#define LOG_DEBUG 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_DEBUG
#endif

#if LOG_LEVEL >= LOG_ERROR
#define LogError(message) do { printf("[ERROR] "); printf message; printf("\n"); } while(0);
#else
#define LogError(message)
#endif


#if LOG_LEVEL >= LOG_WARN
#define LogWarn(message) do { printf("[WARN] "); printf message; printf("\n"); } while(0);
#else
#define LogWarn(message)
#endif


#if LOG_LEVEL >= LOG_INFO
#define LogInfo(message) do { printf("[INFO] "); printf message; printf("\n"); } while(0);
#else
#define LogInfo(message)
#endif

#if LOG_LEVEL >= LOG_DEBUG
#define LogDebug(message) do { printf("[DEBUG] "); printf message; printf("\n"); } while(0);
#else
#define LogDebug(message)
#endif

#endif
