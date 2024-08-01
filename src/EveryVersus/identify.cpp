#include "commons.h"
#include "util.h"
#include <wininternal.h>

#include "identify.h"
#include <xxHash/xxhash.h>

extern int TH06Init();

THGameDef defs[] = {
	{},
	{
		.info = {
			.initFunc = TH06Init,
			.game = TH06,
		},
		.hash = 0x7619c9eb0ce9decc,
	},
		{
		.info = {
			.initFunc = TH06Init,
			.game = TH06,
		},
		.hash = 0x71e9c272ff0b1068,
	}
};

THGameDef& identify_running_game() {
	MappedFile f(CurrentPeb()->ProcessParameters->ImagePathName.Buffer);
	
	if (!f.fileMapView) {
		return defs[0];
	}

	auto hash = XXH3_64bits(f.fileMapView, f.fileSize);

	for (uint32_t i = 1; i < elementsof(defs); i++) {
		if (defs[i].hash == hash) {
			return defs[i];
		}
	}

	defs[0].hash = hash;
	return defs[0];
}