#include <windows.h>
#include "misc/globals.h"

#include "scheduler/scheduler.h"
#include "renderer/renderer.h"

// credits
// unity resolver wrapper :  https://github.com/issuimo/UnityResolve.hpp/
// script execution method : https://github.com/ElCapor/polytoria-executor/

void init() {
    msgbox("hi sir");

    auto game_assembly = GetModuleHandle("GameAssembly.dll");
    unity::Init(game_assembly, unity::Mode::Il2Cpp);
    unity::ThreadAttach();

	std::thread([&]() {
		scheduler.initialize();
	}).detach();

	std::thread([&]() {
		renderer.render();
	}).detach();

	while (true) {}
}

LRESULT WINAPI DllMain(HMODULE hmod, ULONG reason, PVOID) {
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hmod);
		std::thread(init).detach();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}