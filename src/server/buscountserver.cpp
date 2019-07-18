#include <utility>

#include <json/json.h>
#include <drogon/drogon.h>
#include <zconf.h>

#include "../optional.hpp"

struct Feed
{
    std::string name;
    std::string location;

    Feed(std::string name, std::string location):
            name(std::move(name)), location(std::move(location))
    {}

    Json::Value to_json() const
    {
        Json::Value json;
        json["name"] = name;
        json["location"] = location;
        return json;
    }

    static nonstd::optional<Feed> from_json(const Json::Value& data)
    {
        Json::Value name = data["name"];
        Json::Value location = data["location"];

        if (name.isString() && location.isString()) {
            std::cout << "Feed name or location is not a string" << std::endl;
            return Feed(name.asString(), location.asString());
        } else {
            return nonstd::nullopt;
        }
    }
};

struct Device
{
    std::string name;
    std::string id;
    trantor::InetAddress ip;
    Feed default_feed;

    Device(std::string name, std::string id, const trantor::InetAddress& ip, Feed default_feed):
        name(std::move(name)), id(std::move(id)), ip(ip), default_feed(std::move(default_feed))
    {}

    Json::Value to_json() const
    {
        Json::Value json;
        json["ip"] = ip.toIp();
        json["id"] = id;
        json["name"] = name;
        json["default_feed"] = default_feed.to_json();
        return json;
    }

    static nonstd::optional<Device> from_json(Json::Value json, trantor::InetAddress ip)
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
            nonstd::optional<Feed> def_feed = Feed::from_json(default_feed);

            if (def_feed)
                return Device(s_name, s_id, ip, *def_feed);
            else
                return nonstd::nullopt;
        }
    }
};

struct Point
{
    float x, y;

    Point(float x, float y): x(x), y(y)
    {}
};

struct Line
{
    Point a, b;

    Line(const Point& a, const Point& b):
        a(a), b(b)
    {}

    Json::Value to_json() const
    {
        Json::Value json;
        json["x1"] = a.x;
        json["y1"] = a.y;
        json["x2"] = b.x;
        json["y2"] = b.y;
        return json;
    }

    static nonstd::optional<Line> from_json(const Json::Value& json)
    {
        Json::Value x1 = json["x1"];
        Json::Value y1 = json["y1"];
        Json::Value x2 = json["x2"];
        Json::Value y2 = json["y2"];

        if (x1.isDouble() && y1.isDouble() && x2.isDouble() && y2.isDouble())
        {
            Line line(
                    Point(x1.asDouble(), y1.asDouble()),
                    Point(x2.asDouble(), y2.asDouble())
            );
            return line;
        } else {
            std::cout << "One of the points is not a float" << std::endl;
            return nonstd::nullopt;
        }
    }
};

struct Config
{
    Line in, out;
    std::vector<Feed> feeds;

    Config(const Line& in, const Line& out, std::vector<Feed> feeds):
        in(in), out(out), feeds(std::move(feeds))
    {}
};

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

int main()
{

    // TODO Server assumes we are both master and slave. Need to seperate these two out.

    using namespace drogon;

    // master state
    // TODO store count events in database
    int count = 0;
    std::vector<Device> devices;

    // TODO link slave state to server state
    // slave state
    std::vector<Feed> feeds;
    feeds.emplace_back("live", "/live");
    feeds.emplace_back("test", "/test");
    feeds.emplace_back("dirty", "/dirty");
    Config config(
            Line(Point(0.4, 0), Point(0.4, 1)),
            Line(Point(0.6, 0), Point(0.6, 1)),
            feeds
    );

    app().setLogPath("./");
    app().setLogLevel(trantor::Logger::WARN);
    app().addListener("0.0.0.0", 8080);
    app().setThreadNum(2);
    app().setDocumentRoot(getExecutableDir());

    drogon::app().registerHandler("/sanity",
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

    drogon::app().registerHandler("/count",
            [&count](const drogon::HttpRequestPtr&,
                    std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                Json::Value json;
                json["count"] = std::to_string(count);
                auto resp=HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            },
            {Get}
    );

    drogon::app().registerHandler("/status_update",
            [&count](const drogon::HttpRequestPtr& req,
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
            [&devices](const drogon::HttpRequestPtr& req,
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
                    nonstd::optional<Device> device = Device::from_json(*data, ip);
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
            [&devices](const drogon::HttpRequestPtr&,
                    std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                Json::Value json;

                for (const Device& d : devices)
                    json.append(d.to_json());

                auto resp=HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            },
            {Get}
    );

    drogon::app().registerHandler("/get_config",
            [&config](const drogon::HttpRequestPtr&,
                    std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                Json::Value json;

                Json::Value feeds;
                for (const Feed& feed : config.feeds)
                    feeds.append(feed.to_json());
                json["feeds"] = feeds;

                json["line_in"] = config.in.to_json();
                json["line_out"] = config.out.to_json();

                auto resp=HttpResponse::newHttpJsonResponse(json);
                callback(resp);
            },
            {Get}
    );


    drogon::app().registerHandler("/set_config",
            [&config](const drogon::HttpRequestPtr& req,
                    std::function<void (const HttpResponsePtr &)> &&callback, const std::string&) {
                std::shared_ptr<Json::Value> json_ptr = req->jsonObject();
                if (!json_ptr)
                {
                    auto resp=HttpResponse::newHttpResponse();
                    resp->setContentTypeCode(CT_TEXT_PLAIN);
                    resp->setStatusCode(k400BadRequest);
                    resp->setBody("Invalid JSON");

                    callback(resp);
                } else {
                    const Json::Value& json = *json_ptr;
                    nonstd::optional<Line> in = Line::from_json(json["line_in"]);
                    nonstd::optional<Line> out = Line::from_json(json["line_out"]);

                    if (in && out)
                    {
                        config.in = *in;
                        config.out = *out;

                        auto resp=HttpResponse::newHttpResponse();
                        resp->setStatusCode(k200OK);
                        callback(resp);
                    } else {
                        auto resp=HttpResponse::newHttpResponse();
                        resp->setContentTypeCode(CT_TEXT_PLAIN);
                        resp->setStatusCode(k400BadRequest);
                        resp->setBody("In/out lines not specified correctly");
                        callback(resp);
                    }
                }
            },
            {Post}
    );


    std::cout << "App running at localhost:8080" << std::endl;
    app().run();

}