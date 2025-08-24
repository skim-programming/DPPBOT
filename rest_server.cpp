#define NOMINMAX
#include <winsock2.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include "globals.h"

using json = nlohmann::json; // from <nlohmann/json.hpp>
namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;         // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

void rest_server(unsigned short port) {
	try {
		net::io_context ioc;
		tcp::acceptor acceptor{ ioc, {tcp::v4(), port} };
        for (;;) {
            tcp::socket socket{ ioc };
            acceptor.accept(socket);

            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket, buffer, req);

            http::response<http::string_body> res{
                http::status::ok, req.version() };
            res.set(http::field::server, "Beast");
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());

            // Whitelist
            if (req.method() == http::verb::patch && req.target() == "/api/whitelist") {
                auto data = json::parse(req.body());
                json resp;
                resp["status"] = "success";
                resp["message"] = "";
                bool changed = false;

                std::set<std::string> add = data.value("add", std::set<std::string>{});
                std::set<std::string> del = data.value("delete", std::set<std::string>{});

                if (!add.empty()) {
                    for (const auto& w : add) {
                        manager.add_to_whitelist(w);
                    }
                    changed = true;
                    resp["message"] += "Words added to whitelist. ";
                    resp["added_words"] = json::array();
                    for (const auto& w : add) {
                        resp["added_words"].push_back(w);
                    }
                }

                if (!del.empty()) {
                    for (const auto& w : del) {
                        manager.remove_whitelist(w);
                    }
                    changed = true;
                    resp["message"] += "Words removed from whitelist.";
                    resp["removed_words"] = json::array();
                    for (const auto& w : del) {
                        resp["removed_words"].push_back(w);
                    }
                }

                if (changed) {
                    updFilterGlobals();
                    res.body() = resp.dump();
                } else {
                    res.result(http::status::bad_request);
                    res.body() = R"({"error": "Empty sets for add and delete - nothing to do"})";
                }
            }

            else if (req.method() == http::verb::get && req.target() == "/api/whitelist") {
                std::lock_guard<std::mutex> lock(manager.config_mutex);
                json j(manager.config.whitelist);

				res.body() = j.dump();
            }
            // Blacklist
            else if (req.method() == http::verb::post && req.target() == "/api/blacklist") {
                
            }
            else {
                res.result(http::status::not_found);
                res.body() = R"({"error": "Not found"})";
            }
            res.prepare_payload();
            http::write(socket, res);
        }
    }
    catch (std::exception& e) {
        std::cerr << "REST server error: " << e.what() << std::endl;
    }
}