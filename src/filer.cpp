// SMC32 File-handling routines Copyright Mark B Davies 1998

#include "filer.h"
#include <algorithm>
#include <stdlib.h>
#include <string.h>

void File::changeextension(const char* ext)
{
    char* p;

    p = strrchr(name, '.');
    if (p == nullptr)
    {
        p = name + strlen(name);
        *p = '.';
    }
    strcpy(p + 1, ext);
}

// Changes first two characters of extension, leaves number
void File::changeexttype(const char* ext)
{
    char* p;

    p = strrchr(name, '.');
    if (p == nullptr)
    {
        p = name + strlen(name);
        *p = '.';
        strcpy(p + 1, ext);
    }
    else
    {
        p[1] = ext[0];
        p[2] = ext[1];
    }
}

// Checks first two characters only
int File::sameexttype(const char* ext)
{
    auto p = strrchr(name, '.');
    if (p == nullptr)
        return ext == nullptr || ext[0] == '\0';
    return strncmpi(p + 1, ext, 2) == 0;
}

int File::incextension(const char* text)
{
    close();
    changeextension(text);
    auto ext = getextension();
    if (strlen(ext) < 3)
    {
        printf("ERROR: extension was not 3 or more characters (%s)\n", ext);
        return FALSE;
    }
    do
    {
        ext[2]++;
        if (ext[2] > '9')
        {
            printf("ERROR: you have too many output files (%s)\n", name);
            return (FALSE);
        }
    } while (exists());
    return (TRUE);
}

int File::resetpos()
{
    if (!open())
    {
        printf("\nERROR: failed to open file %s\n", name);
        return (FALSE);
    }
    if (fseek(handle, mark, SEEK_SET) != 0)
    {
        printf("\nERROR: seek failed on file %s\n", name);
        return (FALSE);
    }
    return (TRUE);
}

// Ensure we have no garbage past our file position
// Re-write the file in `w` mode and close.
bool File::finalise()
{
    auto pos = ftell(handle);
    if (fseek(handle, 0, SEEK_END) != 0)
    {
        close();
        return false;
    }

    auto end = ftell(handle);
    if (end <= pos)
    {
        close();
        return true;
    }

    // Write null terminator to mark end of file
    char nullt = 0;
    if (fseek(handle, pos, SEEK_SET) != 0)
    {
        close();
        return false;
    }
    fwrite(&nullt, sizeof(nullt), 1, handle);
    fclose(handle);

    // Re-open file in binary mode
    handle = fopen(name, "rb");
    if (handle == nullptr)
        return false;

    // Re-read in entire file into new buffer
    if (fseek(handle, 0, SEEK_END) != 0)
    {
        close();
        return false;
    }
    auto fsize = ftell(handle);
    if (fseek(handle, 0, SEEK_SET) != 0)
    {
        close();
        return false;
    }

    auto buffer = (char*)malloc(fsize + 1);
    auto r = fread(buffer, 1, fsize, handle);
    buffer[r] = '\0';
    auto realSize = strlen(buffer);

    // Write all bytes in buffer until null terminator is reached
    auto result = false;
    handle = freopen(name, "wb", handle);
    if (handle != nullptr && fwrite(buffer, 1, realSize, handle) == realSize)
        result = true;
    fclose(handle);
    free(buffer);
    handle = nullptr;
    return result;
}

// Prints error message and returns FALSE if write failed
int LineFile::writeline(const char* line)
{
    if (!open())
    {
        printf("\nERROR: failed to open file %s\n", name);
        return (FALSE);
    }
    if (fprintf(handle, "%s\n", line) <= 0)
    {
        printf("\nERROR: write failed on file %s\n", name);
        return (FALSE);
    }
    return (TRUE);
}

// Prints error message and returns FALSE if write failed
int LineFile::multiwrite(const char* buf)
{
    if (!open())
    {
        printf("\nERROR: failed to open file %s\n", name);
        return (FALSE);
    }
    if (fputs(buf, handle) < 0)
    {
        printf("\nERROR: write failed on file %s\n", name);
        return (FALSE);
    }
    return (TRUE);
}

// Returns nullptr and closes the file if nothing read
int LineFile::readline()
{
    char* buf = buffer;
    int c;
    int nothingread = TRUE;

    if (!open())
        return (FALSE);
    c = fgetc(handle);
    while (TRUE)
    {
        while (c != EOF && (c == 10 || c == 13)) // Skip blank lines
            c = fgetc(handle);
        if (c == '/') // Comment line
        {
            while (c != EOF && c != 10 && c != 13)
                c = fgetc(handle);
        }
        else
            break;
    }
    while (c != EOF && c != 10 && c != 13)
    {
        *buf++ = c;
        nothingread = FALSE;
        c = fgetc(handle);
    }
    *buf = 0;
    if (c == EOF && nothingread)
        return (FALSE);
    return (TRUE);
}

void get_directory(char* dst, size_t dstLen, const char* filename)
{
    if (dstLen != 0)
    {
        auto slash = strrchr(filename, '/');
#ifdef _WIN32
        auto l = strrchr(filename, '\\');
        if (l != nullptr && l > slash)
        {
            slash = l;
        }
#endif
        if (slash != nullptr)
        {
            auto len = std::min<size_t>(dstLen - 1, slash - filename);
            memcpy(dst, filename, len);
            dst[len] = '\0';
        }
        else
        {
            dst[0] = '\0';
        }
    }
}

char* concat_path(char* dst, size_t dstLen, const char* p)
{
    auto len = strnlen(dst, dstLen);
    if (len > 0)
    {
#ifdef _WIN32
        if (dst[len - 1] != '\\' || dst[len - 1] != '/')
        {
            strncat(dst, "\\", dstLen - strlen(dst) - 1);
        }
#else
        if (dst[len - 1] != '/')
        {
            strncat(dst, "/", dstLen - strlen(dst) - 1);
        }
#endif
    }
    strncat(dst, p, dstLen - strlen(dst) - 1);
    return dst;
}
