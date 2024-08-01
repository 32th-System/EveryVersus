#include <stdint.h>

typedef int GameInitFunc();

enum Game {
	TH06,
	TH_UNKNOWN = -1
};

struct THGameInfo {
	GameInitFunc* initFunc = nullptr;
	Game game = TH_UNKNOWN;
};

struct THGameDef {
	THGameInfo info;
	uint64_t hash;
};

THGameDef& identify_running_game();