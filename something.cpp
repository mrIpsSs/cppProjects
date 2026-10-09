#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

int main()
{
    std::string input;

    std::vector<std::string> lines;
    std::vector<std::string> words;

    std::vector<char> characters;
    std::vector<char> letters;
    std::vector<char> numbers;
    std::vector<int> words_index;

    for (int i = 0; i < 4; i++)
    {
        std::getline(std::cin, input);
        lines.push_back(input);
    }

    for (int c = 0; c < lines.size(); c++)
    {
        std::string templine = lines[c];

        for (char c : templine)
        {
            characters.push_back(c);

            if ((c <  91 && c > 64) || (c < 123 && c > 96))
            {
                letters.push_back(c);

            } else if (c > 47 && c < 58)
            {
                numbers.push_back(c);
            }
        }

        std::stringstream ss(lines[c]);
        std::string word;

        while (std::getline(ss, word, ' '))
        {
            words.push_back(word);
        }
        
        for (int x = 0; x < words.size(); x++)
        {
            std::string tempword = words[x];

            words_index.push_back(1);

            for (int i = 0; i < words.size(); i++)
            {
                if (tempword == words[i])
                {
                    words_index[x]++;
                }
            }
        }
    }

    auto iteration = std::max_element(words_index.begin(), words_index.end());

    std::string mostrepeatedword;

    bool commonword = false;

    if (*iteration > 1)
    {
        auto mostrepeatedword = std::find(words_index.begin(), words_index.end(), *iteration);
        commonword = true;
    }

    std::cout << "Lines: " << lines.size() << "\n"
            << "Characters: " << characters.size() << "\n"
            << "Words: " << words.size() << "\n"
            << "Letters: " << letters.size() << "\n"
            << "Numbers: " << numbers.size() << std::endl;

    if (commonword)
    {
        std::cout << "Common word: " << (std::string)mostrepeatedword << " said " << words_index.at(*iteration) << " times" << std::endl;

    } else if (!commonword)
    {
        std::cout << "No common word was used" << std::endl;
    }

    return 0;
}