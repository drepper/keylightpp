#include <cstdlib>
#include <stdexcept>
#include <string>

#include "keylightpp.hh"

using namespace std::string_literals;


int main(int argc, char* argv[])
{
  auto devs = keylightpp::discover();

  if (argc == 1)
    return 1;

  int nextarg = 1;
  auto selected = argc > 2 ? argv[nextarg++] : nullptr;
  bool any = false;

  while (1) {
    char* cmd = argv[nextarg++];

    for (auto& d : devs)
      if (selected == nullptr || d.name == selected) {
        any = true;

        if ("info"s == cmd)
          d.info();
        else if ("on"s == cmd)
          d.on();
        else if ("off"s == cmd)
          d.off();
        else if ("toggle"s == cmd)
          d.toggle();
        else if ("brightness"s == cmd && nextarg < argc)
          d.brightness(strtoul(argv[nextarg], nullptr, 0));
        else if ("brightness+"s == cmd && nextarg < argc)
          d.brightness_inc(strtoul(argv[nextarg], nullptr, 0));
        else if ("brightness-"s == cmd && nextarg < argc)
          d.brightness_dec(strtoul(argv[nextarg], nullptr, 0));
        else if ("color"s == cmd && nextarg < argc)
          d.color(strtoul(argv[nextarg], nullptr, 0));
        else if ("color+"s == cmd && nextarg < argc)
          d.color_inc(strtoul(argv[nextarg], nullptr, 0));
        else if ("color-"s == cmd && nextarg < argc)
          d.color_dec(strtoul(argv[nextarg], nullptr, 0));
        else
          throw std::invalid_argument("invalid command "s + cmd);
      }

    if (any || selected == nullptr)
      break;

    nextarg = 1;
    selected = nullptr;
  }
}
