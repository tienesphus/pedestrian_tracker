#include "server.hpp"
#include "json_convert.hpp"

#include <geom.hpp>

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>

namespace server {

    struct Device
    {
        std::string name;
        trantor::InetAddress ip;

        Device(std::string name, const trantor::InetAddress& ip);

    };

    Device::Device(std::string name, const trantor::InetAddress& ip):
            name(std::move(name)), ip(ip)
    {}

    Json::Value to_json(const Device& device)
    {
        Json::Value json;
        json["ip"] = device.ip.toIp();
        json["name"] = device.name;
        return json;
    }

    nonstd::optional<Device> device_from_json(Json::Value json, trantor::InetAddress ip) {

        Json::Value name = json["name"];

        if (!name.isString()) {
            std::cout << "Device name is not a string" << std::endl;
            return nonstd::nullopt;
        } else {
            std::string s_name = name.asString();
            return Device(s_name, ip);
        }
    }

    void start()
    {
        using namespace drogon;

        // Simple page to check if something is working
        app().registerHandler("/sanity",
                [](const drogon::HttpRequestPtr&,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k200OK);
                    resp->setContentTypeCode(CT_TEXT_PLAIN);
                    resp->setBody("All good");
                    callback(resp);
                },
                {Post, Get}
        );

        app().registerHandler("/kill",
                [](const drogon::HttpRequestPtr&,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k200OK);
                    resp->setContentTypeCode(CT_TEXT_PLAIN);
                    resp->setBody("System dying");
                    callback(resp);

                    app().quit();
                },
                {Post, Get}
        );

        app().setLogPath("./");
        app().setLogLevel(trantor::Logger::WARN);
        app().addListener("0.0.0.0", 8080);
        app().setThreadNum(2);
        app().setDocumentRoot(std::string(SOURCE_DIR) + "/src/server/");

        spdlog::info("Server running at http://localhost:8080");
        app().run();
    }

    void init_master(DataFetch&)
    {
        using namespace drogon;

        static std::vector<Device> devices;
        static std::mutex device_lock;


        // TODO move these lambdas into separate files
        // I just threw them here because it was easy, but, its a bit clunky

        drogon::app().registerHandler("/register",
                [](const drogon::HttpRequestPtr& req,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    std::shared_ptr<Json::Value> data = req->jsonObject();
                    if (!data) {
                        auto resp=HttpResponse::newHttpResponse();
                        resp->setContentTypeCode(CT_TEXT_PLAIN);
                        resp->setStatusCode(k400BadRequest);
                        resp->setBody("Invalid json");
                        callback(resp);
                    } else {
                        trantor::InetAddress ip = req->getPeerAddr();
                        nonstd::optional<Device> device = device_from_json(*data, ip);
                        if (!device) {
                            auto resp=HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(CT_TEXT_PLAIN);
                            resp->setStatusCode(k400BadRequest);
                            resp->setBody("Device not in correct format");
                            callback(resp);
                        } else {
                            {
                                std::lock_guard<std::mutex> lock(device_lock);
                                devices.erase(std::remove_if(devices.begin(), devices.end(),
                                        [&device](const Device& it) -> bool {
                                            return it.ip.toIp() == (*device).ip.toIp();
                                        }), devices.end());
                                devices.push_back(*device);
                            }

                            auto resp=HttpResponse::newHttpResponse();
                            resp->setStatusCode(k200OK);
                            callback(resp);
                        }
                    }
                },
                {Post}
        );

        drogon::app().registerHandler("/devices",
                [](const drogon::HttpRequestPtr&,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    Json::Value json = Json::arrayValue;

                    {
                        // TODO delete devices that don't regularly send a heartbeat
                        std::lock_guard<std::mutex> lock(device_lock);
                        for (const Device &d : devices)
                            json.append(to_json(d));
                    }

                    auto resp=HttpResponse::newHttpJsonResponse(json);
                    callback(resp);
                },
                {Get}
        );
    }

    void init_slave(DataFetch& data)
    {

        using namespace drogon;

        drogon::app().registerHandler("/get_config",
                [&data](const drogon::HttpRequestPtr&,
                        std::function<void (const drogon::HttpResponsePtr &)> &&callback, const std::string&) {

                    Json::Value json;
                    geom::Line line = data.get_config().crossing;
                    json["crossing"] = line_to_json(line);
                    json["busid"] = data.get_busid();
                    json["devicename"] = data.get_name();
                    json["cloudurl"] = data.get_remote_url();

                    auto resp=drogon::HttpResponse::newHttpJsonResponse(json);
                    callback(resp);
                },
                {drogon::Get}
        );


        drogon::app().registerHandler("/set_config",
                [&data](const drogon::HttpRequestPtr& req,
                        std::function<void (const drogon::HttpResponsePtr &)> &&callback, const std::string&) {
                    std::shared_ptr<Json::Value> json_ptr = req->jsonObject();
                    if (!json_ptr)
                    {
                        auto resp=drogon::HttpResponse::newHttpResponse();
                        resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                        resp->setStatusCode(drogon::k400BadRequest);
                        resp->setBody("Invalid JSON");

                        callback(resp);
                    } else {
                        const Json::Value& json = *json_ptr;

                        Json::Value crossingJson = json["crossing"];
                        if (!crossingJson.empty()) {
                            nonstd::optional<geom::Line> crossing = line_from_json(crossingJson);
                            if (crossing) {
                                WorldConfig config(*crossing, std::vector<geom::Line>());
                                data.update_config(config);
                            } else {
                                auto resp = drogon::HttpResponse::newHttpResponse();
                                resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                                resp->setStatusCode(drogon::k400BadRequest);
                                resp->setBody("In/out lines not specified correctly");
                                callback(resp);
                                return;
                            }
                        }

                        // Attempt to read the name
                        Json::Value name = json["devicename"];
                        if (!name.empty()) {
                            if (name.isString()) {
                                const std::string& name_str = name.asString();
                                if (!name_str.empty())
                                    data.set_name(name_str);
                            } else {
                                auto resp = drogon::HttpResponse::newHttpResponse();
                                resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                                resp->setStatusCode(drogon::k400BadRequest);
                                resp->setBody("Name not set as a string");
                                callback(resp);
                                return;
                            }
                        }

                        // Attempt to read the busid
                        Json::Value busid = json["busid"];
                        if (!busid.empty()) {
                            if (busid.isString()) {
                                const std::string& id_str = busid.asString();
                                if (!id_str.empty())
                                    data.set_busid(id_str);
                            } else {
                                auto resp = drogon::HttpResponse::newHttpResponse();
                                resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                                resp->setStatusCode(drogon::k400BadRequest);
                                resp->setBody("Busid is not a string");
                                callback(resp);
                                return;
                            }
                        }

                        // Attempt to read the remote url
                        Json::Value url = json["cloudurl"];
                        if (!url.empty()) {
                            if (url.isString()) {
                                const std::string& url_str = url.asString();
                                if (!url_str.empty())
                                    data.set_remote_url(url_str);
                            } else {
                                auto resp = drogon::HttpResponse::newHttpResponse();
                                resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                                resp->setStatusCode(drogon::k400BadRequest);
                                resp->setBody("url is not a string");
                                callback(resp);
                                return;
                            }
                        }

                        // All avaliable fields were okay
                        auto resp = drogon::HttpResponse::newHttpResponse();
                        resp->setStatusCode(drogon::k200OK);

                        callback(resp);
                    }
                },
                {drogon::Post}
        );

        drogon::app().registerHandler("/count",
                [&data](const drogon::HttpRequestPtr&,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    Json::Value json;
                    json["count"] = std::to_string(data.count());
                    auto resp=HttpResponse::newHttpJsonResponse(json);
                    callback(resp);
                },
                {drogon::Get}
        );

        drogon::app().registerHandler("/status_update",
                [&data](const drogon::HttpRequestPtr& req,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    spdlog::debug("status_update");
                    std::shared_ptr<Json::Value> json_ptr = req->jsonObject();
                    if (!json_ptr)
                    {
                        auto resp=HttpResponse::newHttpResponse();
                        resp->setContentTypeCode(CT_TEXT_PLAIN);
                        resp->setStatusCode(k400BadRequest);
                        resp->setBody("Invalid json");
                        callback(resp);
                    } else {

                        const Json::Value json = *json_ptr;

                        Json::Value count = json["count"];
                        if (!count.isInt())
                        {
                            auto resp=HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(CT_TEXT_PLAIN);
                            resp->setStatusCode(k400BadRequest);
                            resp->setBody("Must have an int in 'count'");
                            callback(resp);
                        } else {
                            int countValue = count.asInt();
                            data.set_count(countValue);
                            auto resp=HttpResponse::newHttpResponse();
                            resp->setStatusCode(k200OK);
                            callback(resp);
                        }
                    }
                },
                {Post}
        );

        drogon::app().registerHandler("/live.png", [](const drogon::HttpRequestPtr&,
                        std::function<void (const drogon::HttpResponsePtr &)> &&callback, const std::string&)
                {
                    auto resp=drogon::HttpResponse::newFileResponse(SOURCE_DIR "/ram_disk/live.png");
                    callback(resp);
                }
        );

        drogon::app().registerHandler("/dirty.png", [](const drogon::HttpRequestPtr&,
                        std::function<void (const drogon::HttpResponsePtr &)> &&callback, const std::string&)
                {
                    auto resp=drogon::HttpResponse::newFileResponse(SOURCE_DIR "/ram_disk/dirty.png");
                    callback(resp);
                }
        );
    }
}
