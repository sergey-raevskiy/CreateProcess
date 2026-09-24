#pragma once

#include <Windows.h>

typedef struct envar_t envar_t;
struct envar_t
{
    envar_t *next;
    LPCWSTR var;
};

typedef struct
{
    envar_t *first;
} envblock_t;

void env_init(envblock_t *b);
void env_clear(envblock_t *b);

BOOL env_is_empty(const envblock_t *b);

void env_set(envblock_t *b, LPCWSTR var);
void env_import(envblock_t *b, LPCWSTR raw_env);
LPWCH env_export(const envblock_t *b);
void env_override(envblock_t *b, const envblock_t *o, BOOL unset);
