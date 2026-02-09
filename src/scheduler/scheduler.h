#pragma once
#include <string>
#include <thread>

#include "misc/globals.h"

class scheduler_t {
private:
	std::queue<std::string> script_queue;
	void execute_script(const std::string& script);
	void execution_loop();
public:
	void queue_script(const std::string& script);
	void initialize();
};

inline auto scheduler = scheduler_t();