#include <iostream>
#include <string>
#include <vector>
#include <sstream>

typedef struct {
    std::string name;
    int health;
    int age;
} person;

int main()
{
    std::vector<person> people;
    std::string input;

    std::cout << "create name + health + age/read + pos/exit" << std::endl;

    while (input != "exit")
    {
        std::cout << ">> ";

        std::getline(std::cin, input);

        std::stringstream ss(input);

        std::string command;

        ss >> command;

        if (command == "create")
        {
            std::string tempname;

            int temphealth;
            int tempage;

            ss >> tempname >> temphealth >> tempage;

            if ((temphealth > 0 && temphealth < 101) &&
                (tempage > 0 && tempage < 101))

                {
                    person Newperson;

                    Newperson.name = tempname;
                    Newperson.health = temphealth;
                    Newperson.age = tempage;

                    people.push_back(Newperson);

                } else 
                {
                    std::cout << "Invalid parameters given." << std::endl;
                }

        } else if (command == "read")
        {
            int pos;

            ss >> pos;

            if (pos < 0 || pos >= (int)people.size())
            {
                std::cout << "Position out of range." << std::endl;

            } else
            {
                std::cout << "Name: " << people[pos - 1].name << "\n"
                          << "Health: " << people[pos - 1].health << "\n"
                          << "Age: " << people[pos - 1].age << std::endl;
            }
        }
    }

    std::cout << "Exiting..";
    return 0;
}