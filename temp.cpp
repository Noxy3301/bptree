#include <iostream>
#include <string>
int main()
{
    for (int idx = 0; idx < 100000; ++idx)
    {
        std::cout << "\r" << "Loop: #" << idx << std::string(20, ' ');
    }
    std::cout << std::endl;
    std::cin.get();
    return 0;
}