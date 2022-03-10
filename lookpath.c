#include <windows.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool has_ext(const char *fname)
{
    // We first find the last occurrence of a `.` in `fname` (if any)
    // and then check that no `:`, `/`, or `\\` appears after it
    const char *dot = strrchr(fname, '.');
    if (!dot) {
        return false;
    }
    return strpbrk(dot + 1, ":/\\") == NULL;
}

static bool is_exec(const char *fpath)
{
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExA(fpath, GetFileExInfoStandard, &info)) {
        DWORD is_dir = info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        return !is_dir;
    }
    return false;
}

int lookpath(const char *cmd, char cmd_path[MAX_PATH + 1])
{
    // Inspiration drawn from https://golang.org/src/os/exec/lp_windows.go
    // We use `_dupenv_s` and `strtok_s` as safe alternatives to `getenv` and `strtok`
    int   err = 0;
    char *path;
    err = _dupenv_s(&path, NULL, "PATH");
    if (err || path == NULL) {
        return -1;
    }

    if (has_ext(cmd)) {
        char *path_ctx = NULL;
        char *path_dir = strtok_s(path, ";", &path_ctx);
        err            = 1;
        while (path_dir != NULL) {
            sprintf(cmd_path, "%s\\%s", path_dir, cmd);
            if (is_exec(cmd_path)) {
                err = 0;
                break;
            }
            path_dir = strtok_s(NULL, ";", &path_ctx);
        }
        free(path);
        return err;
    }

    char *pathext;
    err = _dupenv_s(&pathext, NULL, "PATHEXT");
    if (err || pathext == NULL) {
        free(path);
        return -1;
    }
    size_t exts_len    = 1;
    char  *pathext_ctx = NULL;
    char  *ext         = strtok_s(pathext, ";", &pathext_ctx);
    while (ext != NULL) {
        ext = strtok_s(NULL, ";", &pathext_ctx);
        exts_len++;
    }

    char *path_ctx = NULL;
    char *path_dir = strtok_s(path, ";", &path_ctx);
    err            = 1;
    while (path_dir != NULL) {
        char *ext = pathext;
        for (size_t i = 0; i < exts_len; i++) {
            sprintf(cmd_path, "%s\\%s%s", path_dir, cmd, ext);
            if (is_exec(cmd_path)) {
                err = 0;
                goto exit;
            }
            ext += strlen(ext) + 1;
        }
        path_dir = strtok_s(NULL, ";", &path_ctx);
    }
exit:
    free(path);
    free(pathext);
    return err;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf_s(stderr, "Provide a command to search the path for: %s <cmd>\r\n", argv[0]);
        return EXIT_FAILURE;
    }
    char cmd_path[MAX_PATH + 1];
    int  err = lookpath(argv[1], cmd_path);
    if (!err) {
        printf("%s\r\n", cmd_path);
    }
    return err;
}
