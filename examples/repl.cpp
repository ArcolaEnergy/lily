#include <lily.hpp>
#include <lilyConstruct.hpp>
#include <lilyParse.hpp>
#include <lilyDefaultEnvironment.hpp>

static bool useAbort= true;
// ^ add function to environment to change this setting?

auto environment= lilyDefaultEnvironment();

int main (int argc, const char** argv) {
	std::string line;
	while (true) {
		std::cout << "> " << std::flush;
		std::getline(std::cin, line);
		if (std::cin.eof())
			break;
		LilyObjectPtr expr= lilyParse(line, true); 
		LETU_AS(err, LilyParseError, expr);
		if (err) {
			std::cout << "PARSE_ERR: " << err->what() << std::endl;
		} else {
			if (useAbort) {
				LilyObjectPtr res=
					eval(expr, environment);
				WRITELN(res);
			} else {
				try {
					LilyObjectPtr res=
						eval(expr, environment);
					WRITELN(res);
				} catch (std::logic_error& e) {
					std::cout << "ERR: " <<
						e.what() << std::endl;
				}
			}
		}
	}
	if (line.length()) {
		std::cout << "PRE_PARSE_ERR: unfinished line" << std::endl;
		return 1;
	} else {
		return 0;
	}
}
