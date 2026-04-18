#ifndef exercism_par_letter_freq
#define exercism_par_letter_freq

#include <cstddef>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace parallel_letter_frequency::exercism {
 [[nodiscard]] std::unordered_map<char, std::size_t> 
 frequency(std::vector<std::string_view> const& texts);
  
}
#endif
