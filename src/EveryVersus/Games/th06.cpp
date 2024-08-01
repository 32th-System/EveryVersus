#define NOMINMAX
#include <Windows.h>

#include "th_common.h"
#include "../breakpoint.h"
#include "../pipe.h"

#include <algorithm>
#include <format>
#include <string>
#include <vector>

#pragma pack(push, 1)

struct Float3 {
    float x;
    float y;
    float z;

    bool operator==(Float3& rhs) {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
};

struct Timer {
    int32_t previous; // 0x0
    float subframe; // 0x4
    int32_t current; // 0x8
};

struct AnmVM {
    char dummy[0x110];
};

struct Laser {
    uint8_t dummy[0x270];
};

// size: 0x560
struct BulletTemplateData {
    AnmVM vm; // 0x0
    AnmVM spawn_effect_short_vm; // 0x110
    AnmVM spawn_effect_medium_vm; // 0x220
    AnmVM spawn_effect_long_vm; // 0x330
    AnmVM despawn_effect_vm; // 0x440
    Float3 hitbox_size; // 0x550
    uint8_t bullet_width_pixels; // 0x55C
    uint8_t bullet_height_pixels; // 0x55D
    
    // Not a vanilla game thing, but I have decided that it is
    uint16_t type;  // 0x55E
    // 0x560

    bool operator==(BulletTemplateData& rhs) {
        return (
            hitbox_size == rhs.hitbox_size &&
            bullet_width_pixels == rhs.bullet_width_pixels &&
            bullet_height_pixels == rhs.bullet_height_pixels
        );
    }
};

struct BulletData {
    union {
        struct {
            Float3 pos;  // 0x560
            Float3 vel;  // 0x56C
            Float3 acc_vec;  // 0x578
            float spd;  // 0x584
            float acc;  // 0x588
            float ex_ang_spd;  // 0x58C
            float ang;  // 0x590
            float ang_vel;  // 0x594
            float ex_ang_off;  // 0x598
            Timer timer;  // 0x59C
            int32_t acc_dur;  // 0x5A8
            int32_t ex_ang_interval;  // 0x5AC
            int32_t ex_ang_cnt;  // 0x5B0
            int32_t ex_ang_max_cnt;  // 0x5B4
            union {
                uint16_t flags;  // 0x5B8
                struct {
                    uint16_t __unknown_flag_A : 1; // 1
                    uint16_t has_spawn_effect_short : 1; // 2
                    uint16_t has_spawn_effect_medium : 1; // 3
                    uint16_t has_spawn_effect_long : 1; // 4
                    uint16_t __unknown_flag_B : 1; // 5
                    uint16_t __unknown_flag_C : 1; // 6
                    uint16_t __unknown_flag_D : 1; // 7
                };
            };
        };
        uint8_t raw_data[0x5A];
    };
};

struct Bullet {
    BulletTemplateData tempData;  // 0x0
    // Not a struct in the original game, but I want to memcpy to it so I'm making it a struct anyways
    BulletData data; // 0x560
    uint16_t color;  // 0x5BA
    unknown_fields(0x2);  // 0x5BC
    uint16_t state;  // 0x5BE
    uint16_t __field_5c0;  // 0x5C0
    uint8_t __field_5c2;  // 0x5C2
    uint8_t __field_5c3;  // 0x5C3
};  // 0x5C4

struct Shooter
{
    uint16_t sprite;
    uint16_t color;
    Float3 position;
    float angle1;
    float angle2;
    float speed1;
    float speed2;
    float exFloats[4];
    int32_t exInts[4];
    int32_t unk_40;
    uint16_t count1;
    uint16_t count2;
    uint16_t aimMode;
    uint32_t flags;
    uint32_t sfx;
};

struct BulletManager {
    BulletTemplateData bullet_templates[16]; // 0x0
    Bullet bullets[640]; // 0x5600
    Laser lasers[64]; // 0xEC000
    int32_t next_bullet_index; // 0xF5C00
    int32_t bullet_count; // 0xF5C04
    Timer time; // 0xF5C08
    char* etama_anm_filename; // 0xF5C14
    // 0xF5C18
};

// TH06 does NOT store the bullet type ID in the Bullet class directly. ShootSingleBullet will simply copy a BulletTemplateData struct into the Bullet struct directly
// The BulletTemplateData struct usually does NOT contain the bullet type ID in it, but I decided that it will do that now
__forceinline static uint32_t _getBulletType(Bullet* bullet) {
#if 0
    return bullet->tempData.type;
#else
    BulletTemplateData* tempData = reinterpret_cast<BulletTemplateData*>(0x5a5ff8);

    for(size_t i = 0; i < 16; i++) {
        if (bullet->tempData == tempData[i]) {
            return i;
        }
    }
    
    constexpr const wchar_t* const bummer =
        L"Idk how to tell ya this, but I couldn't figure out what the type of this bullet is\n\n"
        L"Now, you might be wondering how this is possible, and I'm willing to tell you\n"
        L"but I kinda don't wanna put all that into one giant message box. So instead just contact me I guess.\n\n"
        L"Oh, and case you're wondering, the bullet is at address 0x{:x}"
    ;

    std::wstring msg = std::format(bummer, (uintptr_t)bullet);
    MessageBoxW(NULL, msg.c_str(), L"Well, this is kinda awkward", MB_ICONERROR | MB_OK);
    ExitProcess(-1);
#endif
}

struct BulletSend {
    uint8_t type;
    uint8_t color;
    BulletData data;

    inline BulletSend() {};
    inline BulletSend(Bullet* bullet) {
        memcpy(&this->data, bullet->data.raw_data, sizeof(this->data));
        this->color = bullet->color;
        this->type = _getBulletType(bullet);
    }
};

struct BulletsChunk {
    BulletSend data[22];
};

static_assert(sizeof(BulletsChunk) <= 2048);

#pragma pack(pop)
ValidateFieldOffset(0x0, BulletManager, bullet_templates);
ValidateFieldOffset(0x5600, BulletManager, bullets);
ValidateFieldOffset(0xEC000, BulletManager, lasers);
ValidateFieldOffset(0xF5C00, BulletManager, next_bullet_index);

#define ShootSingleBullet(_this, bulletProps, bulletIdx1, bulletIdx2, angle) asm_call<0x4135b0, Thiscall>(_this, bulletProps, bulletIdx1, bulletIdx2, angle);

std::vector<BulletSend> data_buf;
HANDLE hInputPipe;
HANDLE hOutputPipe;
CRITICAL_SECTION bulletLock = {};

static bool TH06BulletCancelHook(x86Reg* regs, void* param) {
    data_buf.emplace_back(reinterpret_cast<Bullet*>(regs->edx));
    return true;
}

bool pipeIsConnected = false;
DWORD WINAPI PipeOutWaitConnectThread(LPVOID lpParam) {
    ConnectNamedPipe(hOutputPipe, nullptr);
    pipeIsConnected = true;
    return 0;
}

static bool TH06BulletBulletTickEndHook(x86Reg* regs, void* param) {    
    if (data_buf.size()) {
        BulletsChunk ch;
        size_t i = 0;

        for (; i < std::min(22u, data_buf.size()); i++) {
            memcpy(&ch.data[i], &data_buf.back(), sizeof(BulletSend));
            data_buf.pop_back();
        }

        DWORD byteRet;
        if (!WriteFile(hOutputPipe, &ch, i * sizeof(BulletSend), &byteRet, nullptr) && pipeIsConnected) {
            pipeIsConnected = false;
            CreateThread(nullptr, 0, PipeOutWaitConnectThread, nullptr, 0, nullptr);
        }
    }

    return true;
}

static bool TH06BulletTypeHook(x86Reg* regs, void* param) {
    // at 414133 

    if(regs->eax)
        return true;

    Bullet* bullet = *reinterpret_cast<Bullet**>(regs->ebp - 0xC);
    Shooter* shooter = *reinterpret_cast<Shooter**>(regs->ebp + 0x8);

    bullet->tempData.type = shooter->sprite;

    return true;

}

static bool TH06AcquireBulletLock(x86Reg* regs, void* param) {
    EnterCriticalSection(&bulletLock);
    return true;
}

static bool TH06ReleaseBulletLock(x86Reg* regs, void* param) {
    LeaveCriticalSection(&bulletLock);
    return true;
}

static void TH06SpawnBullets(BulletSend* sent, size_t amount) {
    EnterCriticalSection(&bulletLock);

    BulletManager* bmgr = reinterpret_cast<BulletManager*>(0x5a5ff8);

    for (size_t i = 0; i < amount; i++) {
        Shooter sht = { .sprite = sent[i].type, .color = sent[i].color, .aimMode = 9 };
        ShootSingleBullet(bmgr, &sht, 0, 0, 0);

        int32_t cur_bullet_index = bmgr->next_bullet_index - 1;
        if (cur_bullet_index == -1) {
            cur_bullet_index = elementsof(bmgr->bullets) - 1;
        }
       
        memcpy(bmgr->bullets[cur_bullet_index].data.raw_data, sent->data.raw_data, sizeof(sent->data));
    }

    LeaveCriticalSection(&bulletLock);
}

Breakpoint TH06Breakpoints[] = {
    {
        .func = TH06BulletCancelHook,
        .param = nullptr,
        .addr = 0x415D8B,
        .cavesize = 6,
    },
    {
        .func = TH06BulletBulletTickEndHook,
        .param = nullptr,
        .addr = 0x4164DB,
        .cavesize = 5,
    },
    {
        .func = TH06BulletTypeHook,
        .param = nullptr,
        .addr = 0x414133,
        .cavesize = 5,
    },
    // Remember to practice thread safety
    {
        .func = TH06AcquireBulletLock,
        .param = nullptr,
        .addr = 0x4145C0,
        .cavesize = 6,
    },
    {
        .func = TH06ReleaseBulletLock,
        .param = nullptr,
        .addr = 0x414662,
        .cavesize = 6,
    },

};
DWORD WINAPI PipeMsgThread(LPVOID lpParam) {
    BulletsChunk ch;

    while(!(*reinterpret_cast<uint32_t*>(0x6C6BD8))) {
        DWORD byteRet;
        if (!ReadFile(hInputPipe, &ch, 2048, &byteRet, nullptr)) {
            ConnectNamedPipe(hInputPipe, nullptr);
            continue;
        }
        TH06SpawnBullets(ch.data, byteRet / sizeof(BulletSend));
    }

    return 0;
}

int TH06Init() {
#if 0
    LoadLibraryW(L"openinputlagpatch.dll");
#endif
#if 0
    HMODULE hVpatch = LoadLibraryW(L"vpatch_th06_unicode.dll");
    if (!hVpatch) {
        hVpatch = LoadLibraryW(L"vpatch_th06.dll");
    }
#endif

    InitializeCriticalSection(&bulletLock);

    hInputPipe = CreateNamedPipeW(
        L"\\\\.\\pipe\\EveryVersus_input",
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        2048, 2048,
        INFINITE,
        nullptr
    );

    hOutputPipe = CreateNamedPipeW(
        L"\\\\.\\pipe\\EveryVersus_output",
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        2048, 2048,
        INFINITE,
        nullptr
    );

    // ConnectNamedPipe(hOutputPipe, nullptr);

    CreateThread(nullptr, 0, PipeMsgThread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, PipeOutWaitConnectThread, nullptr, 0, nullptr);

    DWORD oldProt;
    VirtualProtect((LPVOID)0x401000, 0x69000, PAGE_EXECUTE_READWRITE, &oldProt);
    BreakpointsApply(TH06Breakpoints, elementsof(TH06Breakpoints));
    VirtualProtect((LPVOID)0x401000, 0x69000, PAGE_EXECUTE_READWRITE, &oldProt);
    
    return 0;
}