#include <iostream>
#include <benzene/benzene.hpp>


int main([[maybe_unused]] int argc, [[maybe_unused]] char const *argv[])
{
    benzene::instance engine{800, 600};

    engine.run([]{});
    return 0;
}
