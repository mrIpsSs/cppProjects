#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<int> nums;

    std::string input;

    std::cout << "only numbers must be input" << std::endl;


    for (int i = 0; i < 10; i++)
    {
        bool pass = true;
        std::getline(std::cin, input);

        for (int i = 0; i < nums.size(); i++)
        {
            if (stoi(input) != nums.at(i))
            {
                pass = true;

            } else if (stoi(input) == nums.at(i))
            {
                pass = false;
                break;
            }
        }
        if (pass)
        {
            nums.push_back(stoi(input));

        } else if (!pass)
        {
            std::cout << "Double: " << input << " (this is for debugging)" << std::endl;
        }
    }
    for (int i = 0; i < nums.size(); i++)
    {
        std::cout << "number " << i << ": " << nums[i] << std::endl;
    }
}