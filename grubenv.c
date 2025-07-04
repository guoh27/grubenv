/*
 * mini_editenv_nocrc.c – minimalist GRUB env-block editor (CRC-less)
 * ---------------------------------------------------------------
 * Treats grubenv as a flat list of "KEY=VAL\0" strings; ignores CRC.
 *
 * Commands:
 *   create        – new blank block (or clear existing)
 *   list          – print all variables
 *   get VAR       – print the value of a single variable
 *   set VAR=VAL   – add/update variable
 *   unset VAR     – delete variable
 *   clear         – wipe all variables
 *
 * Options:
 *   -s SIZE       – set block size (default 1024 bytes)
 *   filename "-"  – read from stdin / write to stdout
 *
 * Build:
 *   gcc -std=c11 -O2 -Wall -Wextra -o mini_editenv mini_editenv_nocrc.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define DEFAULT_BLKSZ 1024U
static uint32_t blk_size = DEFAULT_BLKSZ;

/*──────────────────── I/O helpers ────────────────────*/
static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static void read_all(int fd, uint8_t *buf, size_t n)
{
    size_t off = 0;
    ssize_t r;
    while (off < n && (r = read(fd, buf + off, n - off)) > 0) {
        off += (size_t)r;
    }
    if (r < 0) {
        die("read");
    }
    if (off < n) {
        memset(buf + off, 0, n - off);
    }
}

static void write_all(int fd, const uint8_t *buf, size_t n)
{
    size_t off = 0;
    ssize_t w;

    while (off < n && (w = write(fd, buf + off, n - off)) > 0) {
        off += (size_t)w;
    }
    if (w <= 0) {
        die("write");
    }
}

/*─────────────────── env load/save ───────────────────*/
static void load_env(const char *path, uint8_t *env)
{
    if (strcmp(path, "-") == 0) {
        read_all(STDIN_FILENO, env, blk_size);
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) {
            memset(env, 0, blk_size);
            return;
        }
        die("open");
    }

    read_all(fd, env, blk_size);
    close(fd);
}

static void save_env(const char *path, uint8_t *env)
{
    if (strcmp(path, "-") == 0) {
        write_all(STDOUT_FILENO, env, blk_size);
        return;
    }

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        die("open");
    }

    write_all(fd, env, blk_size);
    close(fd);
}

/*────────────────── string utilities ──────────────────*/
static size_t data_end(const uint8_t *env)
{
    size_t off = 0;
    while (off < blk_size && env[off]) {
        off += strlen((const char *)(env + off)) + 1;
    }
    return off;
}

static uint8_t *find_kv(uint8_t *env, const char *key)
{
    size_t klen = strlen(key);
    size_t off = 0;

    while (off < blk_size && env[off]) {
        char *pair = (char *)(env + off);
        char *eq   = strchr(pair, '=');

        if (eq && (size_t)(eq - pair) == klen &&
            strncmp(pair, key, klen) == 0) {
            return (uint8_t *)pair;
        }

        off += strlen(pair) + 1;
    }
    return NULL;
}

static bool room_left(const uint8_t *env, size_t need)
{
    return data_end(env) + need < blk_size;
}

/*────────────── command handlers ──────────────────*/
static void cmd_list(uint8_t *env)
{
    size_t off = 0;
    while (off < blk_size && env[off]) {
        printf("%s\n", (char *)(env + off));
        off += strlen((char *)(env + off)) + 1;
    }
}

static void cmd_get(uint8_t *env, const char *key)
{
    uint8_t *kv = find_kv(env, key);
    if (kv) {
        char *val = strchr((char *)kv, '=');
        if (val) {
            printf("%s\n", val + 1);
        }
    }
}

static void cmd_set(uint8_t *env, const char *arg)
{
    char *eq = strchr((char *)arg, '=');
    if (!eq || eq == arg) {
        fprintf(stderr, "set: VAR=value required\n");
        exit(EXIT_FAILURE);
    }

    // Remove existing entry
    uint8_t *old = find_kv(env, arg);
    if (old) {
        size_t old_len = strlen((char *)old) + 1;
        memmove(old, old + old_len,
                blk_size - (old - env) - old_len);
        memset(env + blk_size - old_len, 0, old_len);
    }

    size_t len = strlen(arg) + 1;
    if (!room_left(env, len)) {
        fprintf(stderr, "env block full\n");
        exit(EXIT_FAILURE);
    }

    size_t off = data_end(env);
    memcpy(env + off, arg, len);
}

static void cmd_unset(uint8_t *env, const char *key)
{
    uint8_t *old = find_kv(env, key);
    if (!old) return;

    size_t len = strlen((char *)old) + 1;
    memmove(old, old + len,
            blk_size - (old - env) - len);
    memset(env + blk_size - len, 0, len);
}

static void cmd_clear(uint8_t *env)
{
    memset(env, 0, blk_size);
}

/*────────────────────── CLI ──────────────────────*/
static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [-s size] <envfile|-> <create|list|get|set|unset|clear> [ARGS]\n",
            prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
            case 's':
                blk_size = (uint32_t)strtoul(optarg, NULL, 0);
                if (blk_size < 8) die("size");
                break;
            default:
                usage(argv[0]);
        }
    }

    if (argc - optind < 2) {
        usage(argv[0]);
    }

    const char *file = argv[optind++];
    const char *cmd  = argv[optind++];

    uint8_t *env = malloc(blk_size);
    if (!env) die("malloc");

    bool need_save = false;
    if (strcmp(cmd, "create") == 0) {
        cmd_clear(env);
        const char header[] = "# GRUB Environment Block\n";
        strncpy((char *)env, header, sizeof(header));
        need_save = true;
    } else {
        load_env(file, env);
        if (strcmp(cmd, "list") == 0) {
            cmd_list(env);
        } else if (strcmp(cmd, "get") == 0) {
            if (argc - optind != 1) usage(argv[0]);
            cmd_get(env, argv[optind]);
        } else if (strcmp(cmd, "set") == 0) {
            if (argc - optind != 1) usage(argv[0]);
            cmd_set(env, argv[optind]);
            need_save = true;
        } else if (strcmp(cmd, "unset") == 0) {
            if (argc - optind != 1) usage(argv[0]);
            cmd_unset(env, argv[optind]);
            need_save = true;
        } else if (strcmp(cmd, "clear") == 0) {
            cmd_clear(env);
            need_save = true;
        } else {
            usage(argv[0]);
        }
    }

    if (need_save) {
        save_env(file, env);
    }
    free(env);
    return 0;
}
