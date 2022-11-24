#include <libini/libini.h>
#include <iostream>

int main(void) {
  auto parser = libini::IniParser("example.ini"); // Initialize the parser object with a file name.
  auto result = parser.parse(); 		  // Parse the file (synchronously) and obtain a result.

  // Check if 'my_string' is a member of a section in the file.
  std::cout << "Has member 'my_string'? " << result.has_member("my_string") << '\n';

  if (result.has_member("my_string")) {
    auto my_string = result["my_string"]; // Get the container for the 'my_string' member.
    auto value = my_string.get_value<libini::IniString>(); // Get the value for 'my_string'.

    std::cout << "my_string = " << value << '\n';

    auto future = parser.parse_async(); // Parse the file asynchronously and get a future.
    auto async_result = future.get();   // Wait for the result

    auto my_string_async = async_result["my_string"];
    auto value_async = my_string_async.get_value<libini::IniString>();

    std::cout << "(async) my_string = " << value << '\n';
  }

  return 0;
}
