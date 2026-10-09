#include <iostream>
#include <vector>
#include <sstream>

int main()
{
    std::string input;

    while (true)
    {
        std::cout << '\n' << ">> ";

        std::getline(std::cin, input);

        if (input == "exit")
        {
            break;
        }
            std::vector<char> chars;

            std::vector<char> letters;

            std::vector<std::string> words;

            int spaces = 0;

            for (int x = 0; x < input.size(); x++)
            {
                chars.push_back(input[x]);

                if ((input[x] < 91 && input[x] > 64) || 
                    (input[x] < 123 && input[x] > 96))
                {
                    letters.push_back(input[x]);

                } else if (input[x] == ' ')

                {
                    spaces++;
                }
            }
            std::stringstream ss(input);

            std::string word;

            while (std::getline(ss, word, ' '))
            {
                std::string nextword;

                for (char c : word)
                {
                    if ((c < 91 && c > 64) || 
                        (c < 123 && c > 96))
                    {
                        nextword += c;
                    }
                }
                words.push_back(nextword);
            }

            std::cout << chars.size() << " characters: ";

            for (int i = 0; i < chars.size(); i++)
            {
                std::cout << chars[i] << ' ';
            }

            std::cout << "\n";
            std::cout << letters.size() << " letters: ";

            for (int i = 0; i < letters.size(); i++)
            {
                std::cout << letters[i] << ' ';
            }

            std::cout << "\n";

            std::cout << spaces << " spaces" << std::endl;

            std::cout << words.size() << " words: ";

            for (int i = 0; i < words.size(); i++)
            {
                std::cout << words[i] << ' ';
            }
    }
    return 0;
}