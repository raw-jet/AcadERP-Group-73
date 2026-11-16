// group-73
#include "grp_73_CommandParser.h"
#include "grp_73_ErpException.h"
#include "grp_73_Registrar.h"

#include <cstddef>
#include <iostream>
#include <string>

int main() {
    try {
        acaderp::Registrar registrar;
        const acaderp::CommandParser parser;
        std::string line;
        std::size_t line_number = 0U;
        while (std::getline(std::cin, line)) {
            ++line_number;
            try {
                const auto command = parser.parse(line);
                if (!command.has_value()) {
                    continue;
                }
                const auto result = registrar.execute(command.value());
                if (result.has_value()) {
                    std::cout << result.value() << '\n';
                }
            } catch (const acaderp::ErpException& error) {
                std::cout << "ERROR " << line_number << ' ' << error.code() << '\n';
                std::cerr << error.what() << '\n';
            }
        }
        return 0;
    } catch (...) {
        std::cerr << "FATAL\n";
        return 2;
    }
}
