#include "scheduler.h"

void scheduler_t::execute_script(const std::string& script) {
    using type = UnityResolve::UnityType;
    using component = UnityResolve::UnityType::Component;
    using field = UnityResolve::Field;
    using method = UnityResolve::Method;
    using str = UnityResolve::UnityType::String;

    auto assembly = unity::Get("Assembly-CSharp.dll");
    if (!assembly) return;

    auto script_service = assembly->Get("ScriptService");
    auto game = assembly->Get("Game");
    auto script_instance = assembly->Get("ScriptInstance");
    auto base_script = assembly->Get("BaseScript");

    if (!script_service || !game || !script_instance || !base_script) return;

    auto script_service_inst_field = script_service->Get<field>("<Instance>k__BackingField");
    auto game_inst_field = game->Get<field>("singleton");

    if (!script_service_inst_field || !game_inst_field) return;

    component* game_inst = nullptr;
    game_inst_field->GetStaticValue<component*>(&game_inst);

    component* script_service_inst = nullptr;
    script_service_inst_field->GetStaticValue<component*>(&script_service_inst);

    if (!game_inst || !script_service_inst) return;

    auto game_obj = game_inst->GetGameObject();
    if (!game_obj) return;

    auto script_inst = game_obj->AddComponent<component*>(script_instance);
    if (!script_inst) return;
    
    auto script_obj = str::New(script);
    if (!script_obj) return;
    
    auto set_field_recursive = [&](const std::string& name, auto value) -> bool {
        if (script_instance->Get<field>(name)) {
            script_instance->SetValue(script_inst, name, value);
            return true;
        }

        if (base_script && base_script->Get<field>(name)) {
            base_script->SetValue(script_inst, name, value);
            return true;
        }

        return false;
    };

    if (!set_field_recursive("source", script_obj) || !set_field_recursive("running", false)) return;

    auto run_script_method = script_service->Get<method>("RunScript");
    if (!run_script_method) return;
    
    auto run_script = run_script_method->Cast<void, component*, component*>();
    if (!run_script) return;
    
    run_script(script_service_inst, script_inst);
}

void scheduler_t::queue_script(const std::string& script) {
    if (script.empty()) return;
    script_queue.push(script);
}

void scheduler_t::execution_loop() {
    while (true) {
        if (script_queue.empty()) {
            sleep(50);
            continue;
        };

        static std::string script;
        script.clear();

        script = script_queue.front();
        script_queue.pop();
        execute_script(script);

        sleep(50);
    }
}

void scheduler_t::initialize() {
    std::thread(&scheduler_t::execution_loop, this).detach();
    queue_script("print('polysploit initialized ^-^')");
    queue_script("print('\"In the end, We are all detected.\" - Sun Tzu, The Art Of War.')");
}