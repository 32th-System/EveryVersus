#include <Windows.h>

struct MappedFile {
    HANDLE fileMap = nullptr;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    size_t fileSize = 0;
    void* fileMapView = nullptr;

    MappedFile(const wchar_t* fn, size_t max_size = -1)
    {
        hFile = CreateFileW(fn, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            return;
        }
        fileSize = GetFileSize(hFile, nullptr);
        if (fileSize > max_size)
            return;
        fileMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, fileSize, nullptr);
        if (fileMap == nullptr) {
            CloseHandle(hFile);
            return;
        }
        fileMapView = MapViewOfFile(fileMap, FILE_MAP_READ, 0, 0, fileSize);
        if (!fileMapView) {
            CloseHandle(hFile);
            CloseHandle(fileMap);
            return;
        }
    }
    ~MappedFile()
    {
        if (fileMapView)
            UnmapViewOfFile(fileMapView);
        if (fileMap)
            CloseHandle(fileMap);
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
    }
};