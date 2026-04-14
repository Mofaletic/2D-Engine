#include "backend/core/Engine.h"
#include <exception>
#include <iostream>

int main() {
    try {
        Engine engine;
        if (!engine.init()) {
            std::cerr << "Engine initialization failed." << std::endl;
            return 1;
        }

        engine.run();
        engine.shutdown();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Unhandled std::exception: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unhandled unknown exception." << std::endl;
        return 1;
    }
}
