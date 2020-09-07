#include <stdexcept>
#include <string>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/simple-watch.h>


#include "keylightpp.hh"

using namespace std::string_literals;


namespace keylightpp {


  namespace {

    struct cb_data_type {
      device_list_type& res;
      AvahiClient* client;
      AvahiSimplePoll* simple_poll;
    };


    void client_callback(AvahiClient*, AvahiClientState state, void* userdata)
    {
      cb_data_type* data = (cb_data_type*) userdata;

      if (state == AVAHI_CLIENT_FAILURE) {
        avahi_simple_poll_quit(data->simple_poll);
      }
    }


    void resolve_callback(AvahiServiceResolver *r, AvahiIfIndex, AvahiProtocol protocol, AvahiResolverEvent event,
                          const char* name, const char* type, const char *domain, const char* host_name,
                          const AvahiAddress* address, uint16_t port, AvahiStringList *txt, AvahiLookupResultFlags flags,
                          void* userdata)
    {
      cb_data_type* data = (cb_data_type*) userdata;

      switch (event) {
      case AVAHI_RESOLVER_FOUND:
        if (std::find_if(std::begin(data->res), std::end(data->res), [host_name,port](const device_type& o){ return o.port == port && o.host_name == host_name; }) == std::end(data->res))
          data->res.emplace_back(name, host_name, port);
        break;

      default:
        break;
      }

      avahi_service_resolver_free(r);
    }


    void browse_callback(AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event,
                         const char *name, const char *type, const char *domain, AvahiLookupResultFlags, void* userdata)
    {
      cb_data_type* data = (cb_data_type*) userdata;

      switch (event) {
        case AVAHI_BROWSER_FAILURE:
          throw std::runtime_error(std::string("mDNS browsing error: ") + avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));

        case AVAHI_BROWSER_NEW:
          if (avahi_service_resolver_new(data->client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, AvahiLookupFlags(0), resolve_callback, userdata) == 0)
            throw std::runtime_error(std::string("mDNS resolve error") + avahi_strerror(avahi_client_errno(data->client)));
          break;

        case AVAHI_BROWSER_CACHE_EXHAUSTED:
          break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
          // std::cout << "done\n";
          avahi_simple_poll_quit(data->simple_poll);
          break;

        default:
          break;
      }
    }

  } // anonymous namespace


  device_type::device_type(const char* name_, const char* host_name_, in_port_t port_)
  : name(name_), host_name(host_name_), port(port_), base_uri("http://"s + host_name + ":" + std::to_string(port)), client(base_uri)
  {
    client.request(web::http::methods::GET, "/elgato/accessory-info")
    .then([this](web::http::http_response response) {
      if (response.status_code() == web::http::status_codes::OK) {
        auto res = response.extract_json().get();
        if (res["serialNumber"].is_string()) {
          serial = res["serialNumber"].as_string();
        }
      }
    })
    .wait();

    client.request(web::http::methods::GET, "/elgato/lights")
    .then([this](web::http::http_response response) {
      if (response.status_code() != web::http::status_codes::OK)
        throw std::runtime_error("cannot contact light "s + name + " (" + std::to_string(response.status_code()) + ")");

      auto res = response.extract_json().get();
      is_on = res["lights"][0]["on"].as_integer() != 0;
      cur_brightness = res["lights"][0]["brightness"].as_integer();
      cur_temperature = res["lights"][0]["temperature"].as_integer();
    })
    .wait();
  }


  void device_type::info()
  {
    std::cout << name << " is " << (is_on ? "on" : "off") << std::endl;
    std::cout << "brightness = " << cur_brightness << std::endl; 
    std::cout << "temperature = " << cur_temperature << std::endl; 
  }


  namespace {

    const char on_data[] = R"json({"numberOfLights":1,"lights":[{"on":1}]})json";
    const char off_data[] = R"json({"numberOfLights":1,"lights":[{"on":0}]})json";

  } // anonymous namespace


  bool device_type::power(bool on)
  {
    client.request(web::http::methods::PUT, "/elgato/lights", on ? on_data : off_data)
    .then([this](web::http::http_response response) {
      if (response.status_code() == web::http::status_codes::OK) {
        auto res = response.extract_json().get();
        is_on = res["lights"][0]["on"].as_integer() != 0;
      }
    })
    .wait();

    return is_on;
  }


  unsigned device_type::brightness(unsigned v)
  {
    auto req = std::string(R"json({"numberOfLights":1,"lights":[{"brightness":)json") + std::to_string(std::clamp(v, min_brightness, max_brightness)) + "}]}";

    client.request(web::http::methods::PUT, "/elgato/lights", req)
    .then([this](web::http::http_response response) {
      if (response.status_code() == web::http::status_codes::OK) {
        auto res = response.extract_json().get();
        cur_brightness = res["lights"][0]["brightness"].as_integer();
      }
    })
    .wait();

    return cur_brightness;
  }


  unsigned device_type::brightness_inc(unsigned d)
  {
    unsigned v;
    if (__builtin_add_overflow(cur_brightness, d, &v))
      v = max_brightness;
    return brightness(v);
  }


  unsigned device_type::brightness_dec(unsigned d)
  {
    unsigned v;
    if (__builtin_sub_overflow(cur_brightness, d, &v))
      v = min_brightness;
    return brightness(v);
  }


  unsigned device_type::color_from_dev(unsigned v) const { auto prev = std::fegetround(); std::fesetround(FE_TONEAREST); auto res = std::lrint(999000.0 / v); fesetround(prev); return res; }
  unsigned device_type::color_to_dev(unsigned v) const { auto prev = std::fegetround(); std::fesetround(FE_TONEAREST); auto res = std::lrint(999000.0 / v); fesetround(prev); return res; }


  unsigned device_type::color(unsigned v)
  {
    auto v_clamped = std::clamp(v, min_color, max_color);
    auto v_dev = color_to_dev(v_clamped);
    auto req = std::string(R"json({"numberOfLights":1,"lights":[{"temperature":)json") + std::to_string(v_dev) + "}]}";

    client.request(web::http::methods::PUT, "/elgato/lights", req)
    .then([this](web::http::http_response response) {
      if (response.status_code() == web::http::status_codes::OK) {
        auto res = response.extract_json().get();
        cur_temperature = res["lights"][0]["temperature"].as_integer();
      }
    })
    .wait();

    return color_from_dev(cur_temperature);
  }


  unsigned device_type::color_inc(unsigned d)
  {
    unsigned v;
    if (__builtin_add_overflow(color_from_dev(cur_temperature), d, &v))
      v = max_color;
    return color(v);
  }


  unsigned device_type::color_dec(unsigned d)
  {
    unsigned v;
    if (__builtin_sub_overflow(color_from_dev(cur_temperature), d, &v))
      v = min_color;
    return color(v);
  }


  device_list_type discover()
  {
    auto simple_poll = avahi_simple_poll_new();
    if (simple_poll == nullptr)
      throw std::runtime_error("cannot discover");

    device_list_type res;
    cb_data_type data { res, nullptr, simple_poll };

    int error;
    data.client = avahi_client_new(avahi_simple_poll_get(simple_poll), AvahiClientFlags(0), client_callback, &data, &error);
    if (data.client == nullptr)
      throw std::runtime_error(std::string("cannot discover: ") + avahi_strerror(error));

    auto browse = avahi_service_browser_new(data.client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_elg._tcp", nullptr, AvahiLookupFlags(0), browse_callback, &data);
    if (browse == nullptr)
      throw std::runtime_error(std::string("cannot discover: ") + avahi_strerror(avahi_client_errno(data.client)));

    avahi_simple_poll_loop(simple_poll);

    avahi_service_browser_free(browse);

    avahi_client_free(data.client);

    avahi_simple_poll_free(simple_poll);

    return res;
  }

} // namespace keylightpp
