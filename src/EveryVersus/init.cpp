#include <Windows.h>

#include "identify.h"
#include "pipe.h"

#include <format>

DWORD WINAPI EveryVersus_init() {
	MessageBoxW(nullptr, L"", L"", MB_OK);

	auto& def = identify_running_game();
	auto& gameInfo = def.info;
	if (gameInfo.game == TH_UNKNOWN || gameInfo.initFunc == nullptr) {
		auto str = std::format(L"Game not supported (hash = 0x{:x})", def.hash);
		MessageBoxW(NULL, str.c_str(), L"Error", MB_ICONERROR);
		return 0;
	}
	
	gameInfo.initFunc();

	return 1;
}

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH) {
		return EveryVersus_init();
	}
	return TRUE;
}