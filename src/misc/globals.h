#pragma once
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <vector>
#include <atomic>
#include <map>
#include <set>
#include <unordered_map>
#include <string>
#include <queue>

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#include "unity_resolve/wrapper.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"
#include "imgui/TextEditor/TextEditor.h"

namespace fs = std::filesystem;
#define sleep(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))
#define msgbox(text) MessageBoxA(0, text, "polysploit", 0)
using unity = UnityResolve;

namespace globals {

}