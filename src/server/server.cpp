#include "server.hpp"

#include <geom.hpp>

#include <drogon/drogon.h>


namespace server {


    struct Device
    {
        std::string name;
        std::string id;
        trantor::InetAddress ip;

        Device(std::string name, std::string id, const trantor::InetAddress& ip);

    };

    Device::Device(std::string name, std::string id, const trantor::InetAddress& ip):
            name(std::move(name)), id(std::move(id)), ip(ip)
    {}

    Json::Value to_json(const Device& device)
    {
        Json::Value json;
        json["ip"] = device.ip.toIp();
        json["id"] = device.id;
        json["name"] = device.name;
        json["default_feed"] = "/live";
        return json;
    }

    nonstd::optional<Device> device_from_json(Json::Value json, trantor::InetAddress ip)
    {

        Json::Value id = json["id"];
        Json::Value name = json["name"];
        Json::Value default_feed = json["default_feed"];

        if (!id.isString() || !name.isString()) {
            std::cout << "Device id or name is not a string" << std::endl;
            return nonstd::nullopt;
        } else {
            std::string s_id = id.asString();
            std::string s_name = name.asString();
            return Device(s_name, s_id, ip);
        }
    }

    Json::Value to_json(const geom::Line& l)
    {
        Json::Value json;
        json["x1"] = l.a.x;
        json["y1"] = l.a.y;
        json["x2"] = l.b.x;
        json["y2"] = l.b.y;
        return json;
    }

    nonstd::optional<geom::Line> line_from_json(const Json::Value& json)
    {
        Json::Value x1 = json["x1"];
        Json::Value y1 = json["y1"];
        Json::Value x2 = json["x2"];
        Json::Value y2 = json["y2"];

        if (x1.isDouble() && y1.isDouble() && x2.isDouble() && y2.isDouble())
        {
            geom::Line line(
                    geom::Point(x1.asDouble(), y1.asDouble()),
                    geom::Point(x2.asDouble(), y2.asDouble())
            );
            return line;
        } else {
            std::cout << "One of the points is not a float" << std::endl;
            return nonstd::nullopt;
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

        std::cout << "Server running at http://localhost:8080" << std::endl;
        app().run();
    }

    void init_master(const std::function<int()>& getCount, const std::function<void(int)>& addCount)
    {
        using namespace drogon;

        static std::vector<Device> devices;


        // TODO move these lambdas into separate files
        // I just threw them here because it was easy, but, its a bit clunky
        drogon::app().registerHandler("/count",
                [getCount](const drogon::HttpRequestPtr&,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    Json::Value json;
                    json["count"] = std::to_string(getCount());
                    auto resp=HttpResponse::newHttpJsonResponse(json);
                    callback(resp);
                },
                {Get}
        );

        drogon::app().registerHandler("/status_update",
                [addCount](const drogon::HttpRequestPtr& req,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    std::cout << "status_update" << std::endl;
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

                        Json::Value delta = json["delta"];
                        if (!delta.isInt())
                        {
                            auto resp=HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(CT_TEXT_PLAIN);
                            resp->setStatusCode(k400BadRequest);
                            resp->setBody("Must have an int in 'delta'");
                            callback(resp);
                        } else {
                            int change = delta.asInt();
                            addCount(change);
                            auto resp=HttpResponse::newHttpResponse();
                            resp->setStatusCode(k200OK);
                            callback(resp);
                        }
                    }
                },
                {Post}
        );

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
                            devices.push_back(*device);

                            auto resp=HttpResponse::newHttpResponse();
                            resp->setStatusCode(k200OK);
                            callback(resp);
                        }
                    }
                },
                {Post}
        );

        drogon::app().registerHandler("/devices",
                [](const drogon::HttpRequestPtr& req,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    Json::Value json = Json::arrayValue;

                    for (const Device& d : devices)
                        json.append(to_json(d));

                    // Assume I'm a client and add myself
                    // TODO don't hardcode myself in
                    json.append(to_json(Device("master", "master", req->localAddr())));

                    auto resp=HttpResponse::newHttpJsonResponse(json);
                    callback(resp);
                },
                {Get}
        );
    }

    void init_slave(const std::function<WorldConfig()>& getConfig, const std::function<void(WorldConfig)>& setConfig)
    {
        drogon::app().registerHandler("/get_config",
                [getConfig](const drogon::HttpRequestPtr&,
                        std::function<void (const drogon::HttpResponsePtr &)> &&callback, const std::string&) {

                    WorldConfig config = getConfig();
                    Json::Value json = to_json(config);

                    auto resp=drogon::HttpResponse::newHttpJsonResponse(json);
                    callback(resp);
                },
                {drogon::Get}
        );


        drogon::app().registerHandler("/set_config",
                [setConfig](const drogon::HttpRequestPtr& req,
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
                        nonstd::optional<WorldConfig> config = config_from_json(json);

                        if (config)
                        {
                            setConfig(*config);

                            auto resp=drogon::HttpResponse::newHttpResponse();
                            resp->setStatusCode(drogon::k200OK);

                            callback(resp);
                        } else {
                            auto resp=drogon::HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                            resp->setStatusCode(drogon::k400BadRequest);
                            resp->setBody("In/out lines not specified correctly");
                            callback(resp);
                        }
                    }
                },
                {drogon::Post}
        );
    }
}
