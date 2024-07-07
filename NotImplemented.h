#include <stdexcept>
#include <string_view>

class NotImplemented : public std::logic_error
{
public:
    NotImplemented() : std::logic_error("Function not implemented") {}
};

