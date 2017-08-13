#include <cstdio>

#include <crow.h>

inline void setup_routing(crow::SimpleApp &app) {
    CROW_ROUTE(app, "/")([]{
        return "Hello, world!";
    });
}


int main() {
    crow::SimpleApp app;
    setup_routing(app);
    app.port(8080).run();
}
