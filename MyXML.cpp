#include <iostream>
#include <string>
#include <tinyxml2.h>
#include <sstream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

typedef struct
{
    std::string name;
    std::string PID;
    std::string EID;
    std::string timestamp;
    std::string level;
    std::string msg;
} NewEvent;

std::string input;

bool save = false;

std::vector<NewEvent> Events;

bool ParseOneXML(std::string filename, bool save)
{
    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cout << "failed to parse xml file: "
                  << doc.ErrorStr() << "\n";
        return false;
    }


    tinyxml2::XMLElement* root = doc.RootElement();

    for (tinyxml2::XMLElement* event = root->FirstChildElement("Event");
        event != nullptr;
        event = event->NextSiblingElement("Event"))
    {
        tinyxml2::XMLElement* system = event->FirstChildElement();
        tinyxml2::XMLElement* name = system->FirstChildElement("Provider");
        tinyxml2::XMLElement* PID = system->FirstChildElement("Execution");
        tinyxml2::XMLElement* EventID = system->FirstChildElement("EventID");
        tinyxml2::XMLElement* Timestamp = system->FirstChildElement("TimeCreated");

        const char* name_text = name->Attribute("Name");
        const char* PID_text = PID->Attribute("ProcessID");
        const char* EventID_text = EventID->GetText();
        const char* Timestamp_text = Timestamp->Attribute("SystemTime");

        tinyxml2::XMLElement* RenderingInfo = event->FirstChildElement("RenderingInfo");
        tinyxml2::XMLElement* Level = RenderingInfo->FirstChildElement("Level");
        tinyxml2::XMLElement* Message = RenderingInfo->FirstChildElement("Message");

        const char* Level_text = Level->GetText();
        const char* Message_text = Message->GetText();

        std::cout << name_text 
        << " | " << "PID " << PID_text 
        << " | " << "EID " << EventID_text 
        << " | " << Level_text 
        << " | " << Timestamp_text
        << " | " << Message_text << "\n";

        if (save)
        {
            NewEvent event;

            event.name = name_text;
            event.EID = EventID_text;
            event.level = Level_text;
            event.msg = Message_text;
            event.PID = PID_text;
            event.timestamp = Timestamp_text;

            Events.push_back(event);
        }
    }
    return true;
}

bool ParseAllXML(bool save)
{
    fs::path dir = fs::current_path();

    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (entry.path().extension() == ".xml")
        {
            std::string filename = entry.path().filename().string();

            bool success = ParseOneXML(filename, save);
        }
    }
    return true;
}

int main()
{
    while (input != "exit")
    {
        std::cout << ">> ";

        std::getline(std::cin, input);
        std::stringstream ss(input);

        std::string command;

        ss >> command;

        if (command == "parse" || command == "Parse")
        {
            std::string filename;

            ss >> filename;

            if (filename.size() > 3)
            {
                bool success = ParseOneXML(filename, save);
                std::cout << "Parsed file: " << success << "\n";
            }
        }
        else if (command == "save" || command == "Save")
        {
            std::string status;

            ss >> status;

            if (status == "true" || status == "True")
            {
                save = true;
                std::cout << "Event saving set to: " << save << "\n";
            }
            else if (status == "false" || status == "False")
            {
                save = false;
                std::cout << "Event saving set to: " << save << "\n";
            }
            else
            {
                std::cout << "Wrong parameter input, Event saving is currently set to: " << save << "\n";
            }
        }
        else if (command == "parseall")
        {
            bool success = ParseAllXML(save);
        }
        else if (command == "help")
        {
            std::cout << "Command: Parse, Args: File name. Parses a single XML file, returns boolean value of success." << "\n"
                      << "Command: Exit. Exits the program." << "\n"
                      << "Command: Help. Help menu pop-up." << "\n"
                      << "Command: Save, Args: True/False. Whether events get stored or not (only during code run)." << "\n"
                      << "In order to parse an .xml file it must be in the same directory of the executable file." << "\n";
        }
        else if (command == "allevent")
        {
            if (Events.size() > 0)
            {
                for (int c = 0; c < Events.size(); c++)
                {
                    std::cout << Events[c].name 
                    << " | " << "PID " << Events[c].PID 
                    << " | " << "EID " << Events[c].EID
                    << " | " << Events[c].level
                    << " | " << Events[c].timestamp
                    << " | " << Events[c].msg << "\n";
                }
            }
        }
    }
    if (input == "exit")
    {
        system("pause");
    }
}