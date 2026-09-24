#include "env.h"

static size_t get_name_len(LPCWSTR var)
{
    LPCWSTR value = wcschr(var, L'=');

    if (value)
    {
        return value - var;
    }
    else
    {
        return wcslen(var);
    }
}

static int compare_name(LPCWSTR a,
                        size_t a_len,
                        LPCWSTR b,
                        size_t b_len)
{
    if (a_len == b_len)
    {
        return _wcsnicmp(a, b, a_len);
    }
    if (a_len > b_len)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

void env_init(envblock_t *b)
{
    b->first = NULL;
}

void env_clear(envblock_t *b)
{
    envar_t *v = b->first;

    while (v)
    {
        envar_t *next = v->next;

        free(v->var);
        free(v);
        v = next;
    }
}

BOOL env_is_empty(const envblock_t *b)
{
    return !b->first;
}

static void set_internal(envblock_t *b, LPCWSTR var, BOOL unset)
{
    envar_t *v = b->first;
    envar_t *left = NULL;
    envar_t *right = NULL;
    envar_t *mid = NULL;

    while (v)
    {
        int cmp = compare_name(var, get_name_len(var), v->var, get_name_len(v->var));

        if (cmp < 0)
        {
            left = v;
            right = v->next;
        }
        else if (cmp == 0)
        {
            mid = v;
            right = v->next;
            break;
        }
        else
        {
            break;
        }

        v = v->next;
    }

    if (mid)
    {
        free(mid->var);
        free(mid);
    }

    if (unset)
    {
        v = right;
    }
    else
    {
        v = malloc(sizeof(*v));

        if (v)
        {
            v->next = right;
            v->var = _wcsdup(var);
        }
    }

    if (left)
    {
        left->next = v;
    }
    else
    {
        b->first = v;
    }
}

void env_set(envblock_t *b, LPCWSTR var)
{
    set_internal(b, var, FALSE);
}

void env_import(envblock_t *b, LPCWSTR raw_env)
{
    LPCWSTR v = raw_env;

    while (TRUE)
    {
        size_t len = wcslen(v);

        if (len == 0)
        {
            break;
        }

        set_internal(b, v, FALSE);

        v += len + 1;
    }
}

void env_override(envblock_t *b, const envblock_t *o, BOOL unset)
{
    envar_t *v = o->first;

    while (v)
    {
        set_internal(b, v->var, unset);

        v = v->next;
    }
}

static size_t export_buf_len(const envblock_t *b)
{
    envar_t *v = b->first;
    size_t len = 0;

    while (v)
    {
        size_t vlen = wcslen(v->var);

        len += vlen + 1;

        v = v->next;
    }

    len += 1;

    return len;
}

static void export_buf(LPWCH dst, const envblock_t *b)
{
    envar_t *v = b->first;

    while (v)
    {
        size_t vlen = wcslen(v->var);

        memcpy(dst, v->var, vlen * sizeof(dst[0]));
        dst += vlen;
        *dst = L'\0';
        dst++;

        v = v->next;
    }

    *dst = L'\0';
}

LPWCH env_export(const envblock_t *b)
{
    LPWCH buf = malloc(export_buf_len(b) * sizeof(WCHAR));

    export_buf(buf, b);

    return buf;
}

