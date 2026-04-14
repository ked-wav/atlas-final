#include "core/Application.h"

#include <string>

int main(int argc, char** argv) {
    atlas::core::Application app;

    if (argc > 1 && std::string(argv[1]) == "--smoke-test") {
        return app.runSmokeTest();
    }

    return app.run();
}
