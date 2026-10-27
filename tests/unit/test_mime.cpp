#include <doctest/doctest.h>
#include "assets/mime.hpp"

using namespace auraui::assets;

TEST_CASE("Mime - Common Web Types") {
    CHECK(Mime::from_extension("html") == "text/html; charset=utf-8");
    CHECK(Mime::from_extension(".html") == "text/html; charset=utf-8");
    CHECK(Mime::from_extension("css") == "text/css; charset=utf-8");
    CHECK(Mime::from_extension("js") == "application/javascript; charset=utf-8");
    CHECK(Mime::from_extension("mjs") == "application/javascript; charset=utf-8");
    CHECK(Mime::from_extension("json") == "application/json; charset=utf-8");
    CHECK(Mime::from_extension("wasm") == "application/wasm");
}

TEST_CASE("Mime - Images and Media") {
    CHECK(Mime::from_extension("png") == "image/png");
    CHECK(Mime::from_extension("jpg") == "image/jpeg");
    CHECK(Mime::from_extension("jpeg") == "image/jpeg");
    CHECK(Mime::from_extension("svg") == "image/svg+xml");
    CHECK(Mime::from_extension("ico") == "image/x-icon");
    CHECK(Mime::from_extension("webp") == "image/webp");
    CHECK(Mime::from_extension("woff2") == "font/woff2");
}

TEST_CASE("Mime - Path Extraction and Case Insensitivity") {
    CHECK(Mime::from_path("/assets/app.min.js") == "application/javascript; charset=utf-8");
    CHECK(Mime::from_path("C:\\Users\\app\\index.HTML") == "text/html; charset=utf-8");
    CHECK(Mime::from_path("/version.1.0/logo.PNG") == "image/png");
    CHECK(Mime::from_path("unknown_file") == "application/octet-stream");
    CHECK(Mime::from_path("/path.with.dots/binary") == "application/octet-stream");
}
