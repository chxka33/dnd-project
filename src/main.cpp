#include "core/Application.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        dnd::core::Application app;
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
