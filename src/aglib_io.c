#include "../include/aglib_io.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>

#define BUF_SIZE 512

// ——— Stdin Func —————————————————————————————————————————————————————————————————————————————————————

bool get_str(const char* prompt, char** str , size_t len , sAllocator* a) {
  if (!prompt || !str || len == 0 || !a) return false;
  
  char* buffer = (char*)ag_alloc(a, len, false);
  if (!buffer) return false;
  
  printf("%s", prompt);
  fflush(stdout);
  
  if (fgets(buffer, (int)len, stdin)) {
    size_t last = strlen(buffer) - 1;
    if (buffer[last] == '\n') {
      buffer[last] = '\0';
    }
    
    *str = buffer;
    return true;
  }
  
  return false;
}

bool get_char(const char* prompt, char*  val) {
  if (!prompt || !val) return false;
  
  printf("%s", prompt);
  fflush(stdout);
  
  int c = getchar();
  if (c == EOF) return false;
    
  *val = (char)c;
  
  int next;
  while ((next = getchar()) != '\n' && next != EOF);
  
  return true;
}

bool get_opt(const char* prompt, char*  val , size_t nOpt, const char opts[]) {
  if (!prompt || !val || !opts || nOpt == 0) return false;
  
  while (1) {
    printf("%s", prompt);
    fflush(stdout);
    
    int input = getchar();
    if (input == EOF) return false;
    
    int  next;
    bool extraInp = false;
    while ((next = getchar()) != '\n' && next != EOF) {
      if (next != ' ' && next != '\t') extraInp = true;
    }

    if (extraInp) {
      printf("Invalid input. Please enter a single character.\n");
      continue;
    }
    
    for (size_t i = 0; i < nOpt; ++i) {
      if ((char)input == opts[i]) {
        *val = (char)input;
        return true;
      }
    }
    
    printf("Invalid option. Please try again.\n");
  }
}

bool get_int(const char* prompt, int* val) {
  return get_int_range(prompt, val, INT_MIN, INT_MAX);
}

bool get_float(const char* prompt, float* val) {
  return get_float_range(prompt, val, -FLT_MAX, FLT_MAX);
}


bool get_int_range(const char* prompt, int* val , int min , int max) {
  if (!prompt || !val || max < min) return false;

  while (1) {
    printf("%s", prompt);
    fflush(stdout);
    
    char buf[BUF_SIZE];
    if (!(fgets(buf, sizeof(buf), stdin))) return false;
    buf[strcspn(buf, "\n")] = '\0';
    
    if (buf[0] == '\0') {
      printf("Input cannot be empty.\n");
      continue;
    }
    
    
    errno       = 0;
    char* end;
    long  value = strtol(buf, &end, 10);
    if (end == buf || *end != '\0' || errno == ERANGE || value < min || value > max) {
      printf("Please enter a number between %d and %d.\n", min, max);
      continue;
    }

    *val = (int)value;
    return true;
  }
}

bool get_float_range(const char* prompt, float* val , float min , float max) {
  if (!prompt || !val || max < min) return false;

  while (1) {
    printf("%s", prompt);
    fflush(stdout);
    
    char buf[BUF_SIZE];
    if (!(fgets(buf, sizeof(buf), stdin))) return false;
    buf[strcspn(buf, "\n")] = '\0';
    
    if (buf[0] == '\0') {
      printf("Input cannot be empty.\n");
      continue;
    }
    
    
    errno       = 0;
    char* end;
    float value = strtof(buf, &end);
    if (end == buf || *end != '\0' || errno == ERANGE || value < min || value > max || isnan(value)) {
      printf("Please enter a number between %.2f and %.2f.\n", min, max);
      continue;
    }

    *val = value;
    return true;
  }
}
