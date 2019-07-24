#include "server.hpp"

#include "../utils.hpp"

#include <drogon/drogon.h>
#include <zconf.h>


namespace server {

    Feed::Feed(std::string name, std::string location):
            name(std::move(name)), location(std::move(location))
    {}

    Json::Value to_json(const Feed& feed)
    {
        Json::Value json;
        json["name"] = feed.name;
        json["location"] = feed.location;
        return json;
    }

    nonstd::optional<Feed> feed_from_json(const Json::Value& data)
    {
        Json::Value name = data["name"];
        Json::Value location = data["location"];

        if (name.isString() && location.isString()) {
            return Feed(name.asString(), location.asString());
        } else {
            std::cout << "Feed name or location is not a string" << std::endl;
            return nonstd::nullopt;
        }
    }

    struct Device
    {
        std::string name;
        std::string id;
        trantor::InetAddress ip;
        Feed default_feed;

        Device(std::string name, std::string id, const trantor::InetAddress& ip, Feed default_feed);

    };

    Device::Device(std::string name, std::string id, const trantor::InetAddress& ip, Feed default_feed):
            name(std::move(name)), id(std::move(id)), ip(ip), default_feed(std::move(default_feed))
    {}

    Json::Value to_json(const Device& device)
    {
        Json::Value json;
        json["ip"] = device.ip.toIp();
        json["id"] = device.id;
        json["name"] = device.name;
        json["default_feed"] = to_json(device.default_feed);
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
            nonstd::optional<Feed> def_feed = feed_from_json(default_feed);

            if (def_feed)
                return Device(s_name, s_id, ip, *def_feed);
            else
                return nonstd::nullopt;
        }
    }

    Json::Value to_json(const utils::Line& l)
    {
        Json::Value json;
        json["x1"] = l.a.x;
        json["y1"] = l.a.y;
        json["x2"] = l.b.x;
        json["y2"] = l.b.y;
        return json;
    }

    nonstd::optional<utils::Line> line_from_json(const Json::Value& json)
    {
        Json::Value x1 = json["x1"];
        Json::Value y1 = json["y1"];
        Json::Value x2 = json["x2"];
        Json::Value y2 = json["y2"];

        if (x1.isDouble() && y1.isDouble() && x2.isDouble() && y2.isDouble())
        {
            utils::Line line(
                    utils::Point(x1.asDouble(), y1.asDouble()),
                    utils::Point(x2.asDouble(), y2.asDouble())
            );
            return line;
        } else {
            std::cout << "One of the points is not a float" << std::endl;
            return nonstd::nullopt;
        }
    }

    OpenCVConfig::OpenCVConfig(const utils::Line& crossing):
            crossing(crossing)
    {}

    Config::Config(const OpenCVConfig& cvConfig, std::vector<Feed> feeds):
            cvConfig(cvConfig), feeds(std::move(feeds))
    {}

    /**
     * Gets the location of the file that is being executed
     * @return the executable files path
     */
    std::string getExecutablePath()
    {
        char szTmp[32];
        char pBuf[256];
        size_t len = sizeof(pBuf);
        sprintf(szTmp, "/proc/%d/exe", getpid());
        int bytes = std::min(static_cast<unsigned long>(readlink(szTmp, pBuf, len)), len - 1);
        if(bytes >= 0)
            pBuf[bytes] = '\0';
        return std::string(pBuf, bytes);
    }

    /**
     * Gets the directory that the executable is in
     * @return the executables directory
     */
    std::string getExecutableDir()
    {
        std::string path = getExecutablePath();
        return path.substr(0, path.find_last_of('/'));
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

        app().setLogPath("./");
        app().setLogLevel(trantor::Logger::WARN);
        app().addListener("0.0.0.0", 8080);
        app().setThreadNum(2);
        app().setDocumentRoot(getExecutableDir());

        std::cout << "Server running at http://localhost:8080" << std::endl;
        app().run();
    }

    void init_master()
    {
        using namespace drogon;

        // TODO store count events in database
        static int count = 0;
        static std::vector<Device> devices;

        drogon::app().registerHandler("/count",
                [](const drogon::HttpRequestPtr&,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    Json::Value json;
                    json["count"] = std::to_string(count);
                    auto resp=HttpResponse::newHttpJsonResponse(json);
                    callback(resp);
                },
                {Get}
        );

        drogon::app().registerHandler("/status_update",
                [](const drogon::HttpRequestPtr& req,
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
                            count += change;

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
                [](const drogon::HttpRequestPtr&,
                        std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                    Json::Value json;

                    for (const Device& d : devices)
                        json.append(to_json(d));

                    auto resp=HttpResponse::newHttpJsonResponse(json);
                    callback(resp);
                },
                {Get}
        );
    }

    void init_slave(const std::function<Config()>& getConfig, const std::function<void(OpenCVConfig)>& setConfig)
    {
        drogon::app().registerHandler("/get_config",
                [getConfig](const drogon::HttpRequestPtr&,
                        std::function<void (const drogon::HttpResponsePtr &)> &&callback, const std::string&) {
                    Json::Value json;

                    Config config = getConfig();

                    Json::Value feeds;
                    for (const Feed& feed : config.feeds)
                        feeds.append(to_json(feed));
                    json["feeds"] = feeds;

                    json["crossing"] = to_json(config.cvConfig.crossing);

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
                        nonstd::optional<utils::Line> crossing = line_from_json(json["crossing"]);

                        if (crossing)
                        {
                            OpenCVConfig config(*crossing);
                            setConfig(config);

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