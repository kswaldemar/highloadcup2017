#include <cstdio>

#include <crow.h>

inline void setup_routing(crow::SimpleApp &app) {
    CROW_ROUTE(app, "/")([]{
        return crow::response(404);
    });
}

int main() {
    crow::SimpleApp app;
    setup_routing(app);
    app.port(80).multithreaded().run();
}