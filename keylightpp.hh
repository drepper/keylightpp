#ifndef _KEYLIGHTPP_HH
#define _KEYLIGHTPP_HH 1

#include <cfenv>
#include <cmath>
#include <list>

#include <cpprest/http_client.h>

namespace keylightpp {

  struct device_type {
    std::string name;
    std::string host_name;
    in_port_t port;
    std::string serial;
    std::string base_uri;
    web::http::client::http_client client;

    device_type(const char* name, const char* host_name, in_port_t port);

    void info();

    void on() { power(true); }
    void off() { power(false); }
    bool toggle() { return power(! is_on); }


    static constexpr unsigned min_brightness = 0u;
    static constexpr unsigned max_brightness = 100u;

    unsigned brightness() const { return cur_brightness; }
    unsigned brightness(unsigned v);
    unsigned brightness_inc(unsigned d);
    unsigned brightness_dec(unsigned d);


    static constexpr unsigned min_color = 2900u;
    static constexpr unsigned max_color = 7000u;

    unsigned color() const { return color_to_dev(cur_temperature); }
    unsigned color(unsigned v);
    unsigned color_inc(unsigned d);
    unsigned color_dec(unsigned d);

  private:
    bool power(bool);

    unsigned color_from_dev(unsigned v) const;
    unsigned color_to_dev(unsigned v) const;


    bool is_on = false;
    unsigned cur_brightness = 0;
    unsigned cur_temperature = 0.0;
  };


  using device_list_type = std::list<device_type>;

  device_list_type discover();

} // namespace keylightpp

#endif // keylightpp.hh
