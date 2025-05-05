//
// Created by Alexander on 12.12.2020.
// Modified by Nikolay on 05.05.2025.
//

#include <queue>
#include <sstream>
#include <iostream>
#include <fstream>
#include "grammar.h"


typedef std::pair<int, int> substr_marker;
typedef std::pair<int, substr_marker> var_tree_ref;

struct parse_tree_node {
    std::set<var_tree_ref> contexts;
    std::set<std::vector<var_tree_ref>> productions;

    parse_tree_node() = default;
};


class derivation_table {
private:
    std::map<substr_marker, std::map<int, parse_tree_node>> substr_derivations;
    int _size, _start;
    alphabet _alphabet;

    std::string generate_def(const var_tree_ref &ref, int index_helper, const std::string &label = "");

public:
    // Инициализируем пустую таблицу по строке
    explicit derivation_table(const std::vector<int> &string);

    // Размер кроны
    int size() const;

    // Задаём стартовый символ
    void set_start(int start);

    // Задаём алфавит
    void set_alphabet(const alphabet &labels);

    // Возвращаем ссылку на список возможных выводов для маркера подстроки
    std::map<int, parse_tree_node> &operator[](substr_marker marker);

    // Выводим дерево в формате DOT в произвольный поток. Используем алфавит, заданный через метод выше
    friend std::ostream &operator<<(std::ostream &out, derivation_table &table);
};


derivation_table::derivation_table(const std::vector<int> &string) {
    this->_size = string.size();
    this->_start = 0;
    for (int i = 0; i < string.size(); ++i)
        for (int j = i + 2; j <= string.size(); ++j)
            this->substr_derivations[{i, j}] = {};
    for (int i = 0; i < string.size(); ++i)
        this->substr_derivations[{i, i + 1}][string[i]] = {};
}

int derivation_table::size() const {
    return _size;
}

void derivation_table::set_start(int start) {
    _start = start;
}

void derivation_table::set_alphabet(const alphabet &labels) {
    this->_alphabet = labels;
}

std::map<int, parse_tree_node> &derivation_table::operator[](substr_marker marker) {
    return this->substr_derivations[marker];
}

std::string generate_name(const var_tree_ref &ref, int index_helper) {
    std::ostringstream oss;
    oss << "vertex_" << ref.first << "_" << ref.second.first << "_" << ref.second.second << "_" << index_helper;
    return oss.str();
}

std::ostream &operator<<(std::ostream &out, derivation_table &table) {
    out << "digraph ParseTree {\n";
    std::queue<var_tree_ref> roots;
    roots.push({table._start, {0, table.size()}});
    int root_counter = 0;
    std::set<var_tree_ref> defined_roots;
    while (not roots.empty()) {
        std::queue<var_tree_ref> queue;
        queue.push(roots.front());
        out << "\t" << table.generate_def(queue.front(), root_counter);
        std::set<var_tree_ref> defined;
        while (not queue.empty()) {
            auto ref = queue.front();
            // Contexts
            for (const auto &context : table[ref.second][ref.first].contexts) {
                if (not defined_roots.count(context)) {
                    out << "\t\n\t" << table.generate_def(context, root_counter + (int) roots.size());
                    roots.push(context);
                    defined_roots.insert(context);
                }
                out << "\t" << generate_name(ref, root_counter) << " -> "
                    << generate_name(context, root_counter + (int) roots.size() - 1) << " [style=\"dotted\"];\n";
            }

            // Productions
            int prod_counter = 0;
            for (const auto &production : table[ref.second][ref.first].productions) {
                if (production.empty())
                    continue;
                if (not defined.count(production[0])) {
                    if (production.size() == 1) {
                        out << "\t\n\t" << table.generate_def(production[0], root_counter);
                        queue.push(production[0]);
                        defined.insert(production[0]);
                    } else {
                        out << "\t\n\tsubgraph cluster_" << generate_name(ref, root_counter) << "_derivation_"
                            << prod_counter << " {\n" << "\t\tcolor=blue style=dashed\n";
                        for (auto iter = production.rbegin(); iter != production.rend(); iter++) {
                            out << "\t\t" << table.generate_def(*iter, root_counter);
                            queue.push(*iter);
                            defined.insert(*iter);
                        }
                        prod_counter++;
                        out << "\t}\n\n";
                    }
                }
                for (const auto &node : production)
                    out << "\t" << generate_name(ref, root_counter) << " -> " << generate_name(node, root_counter)
                        << ";\n";
            }

            // Terminal
            if (ref.second.second - ref.second.first == 1 and table[ref.second][ref.first].productions.empty())
                out << "\t" << generate_name(ref, root_counter) << " -> " << "string:" << ref.second.first << ";\n";
            queue.pop();
        }
        roots.pop();
        root_counter++;
    }

    // Acquire terminal nodes
    std::vector<var_tree_ref> terminal;
    for (int i = 0; i < table.size(); ++i)
        for (const auto &pair : table[{i, i + 1}])
            if (pair.second.productions.empty())
                terminal.push_back({pair.first, {i, i + 1}});

    out << "\t\n\tstring [shape=\"record\", label=\"|";
    for (const var_tree_ref &term : terminal)
        out << " <" << term.second.first << "> " << table._alphabet[term.first] << " |";
    out << "\"];\n}" << std::endl;
    return out;
}

std::string derivation_table::generate_def(const var_tree_ref &ref, int index_helper, const std::string &label) {
    std::ostringstream oss;
    oss << generate_name(ref, index_helper) << " [label=\""
        << (label.empty() ? _alphabet[ref.first] : label) << "\"];\n";
    return oss.str();
}

bool algorithm_pass(derivation_table &table, const std::vector<rule> &rules) {
    bool needs_repetition = false;
    for (int len = 1; len <= table.size(); ++len) {
        for (int i = 0; i + len - 1 < table.size(); ++i) {
            for (const auto &rule : rules) {
                int productions = 0;
                bool rule_applies = true;
                parse_tree_node node;
                for (const auto &context : rule.contexts) {
                    if (context.second == NONE) {
                        productions++;
                        auto production = context.first;
                        if (production.size() == 1) {
                            if (table[std::make_pair(i, i + len)].count(production[0])) {
                                std::vector<var_tree_ref> new_pr = {var_tree_ref(production[0], {i, i + len})};
                                node.productions.insert(new_pr);
                            } else
                                rule_applies = false; // rule does not match
                        } else if (production.size() == 2) {
                            bool flag = false;
                            for (int s_len = 1; s_len < len; ++s_len) {
                                if (table[std::make_pair(i, i + s_len)].count(production[0]) and
                                    table[std::make_pair(i + s_len, i + len)].count(production[1])) {
                                    std::vector<var_tree_ref> new_pr = {{production[0], {i,         i + s_len}},
                                                                        {production[1], {i + s_len, i + len}}};
                                    if (not flag)
                                        node.productions.insert(new_pr);
                                    flag = true;
                                }
                            }
                            rule_applies &= flag;
                        } else rule_applies = false; // Not binary normal form
                    } else {
                        substr_marker context_marker;
                        switch (context.second) {
                            case LEFT:
                                context_marker = {0, i};
                                break;
                            case RIGHT:
                                context_marker = {i + len, table.size()};
                                break;
                            case LEFT_EXT:
                                context_marker = {0, i + len};
                                break;
                            case RIGHT_EXT:
                                context_marker = {i, table.size()};
                                break;
                            default:
                                break; // This case label is in fact useless
                        }
                        if (context.first.size() == 1 and table[context_marker].count(context.first[0])) {
                            node.contexts.insert({context.first[0], context_marker});
                        } else rule_applies = false;
                    }
                }
                if (rule_applies and productions > 0) {
                    if (not table[std::make_pair(i, i + len)].count(rule.origin)) {
                        table[std::make_pair(i, i + len)][rule.origin] = node;
                        if (i == 0 or i + len == table.size())
                            needs_repetition = true;
                    }
                }
            }
        }
    }
    return needs_repetition;
}

derivation_table build_derivation(const grammar &_grammar, const std::vector<int> &string) {
    derivation_table table(string);
    algorithm_pass(table, _grammar.no_context_rules());
    auto all_grammar_rules = _grammar.all_rules();
    while (algorithm_pass(table, all_grammar_rules)) {}
    return table;
}

#define ERROR "\e[31;1mCRITICAL ERROR:\u001b[37;0m "
#define WARNING "\e[33;1mWARNING:\u001b[37;0m "
#define INPUT "\e[36;1m> \u001b[37;0m"

template<typename T>
void warn(T warning_message) {
    std::cerr << WARNING << warning_message << std::endl;
}

template<typename T>
void die(T error_message) {
    std::cerr << ERROR << error_message << "\nThe program will be terminated immediately" << std::endl;
    std::exit(-1);
}

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

// Helper functions
void die(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
    exit(EXIT_FAILURE);
}

void warn(const std::string& message) {
    std::cerr << "Warning: " << message << std::endl;
}

void displayHelp(const std::string& programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Supported options:\n"
              << "  --help                Display this help message\n"
              << "  --version             Display parser version\n"
              << "  --grammar,-g <file>   Pass grammar file\n"
              << "  --input,-i <text>     Pass text to parse\n"
              << "  --input-file,-f <file> Pass text file to parse (overrides --input)\n"
              << "  --output,-o <file>    Place the output into <file>\n";
}

void displayVersion() {
    std::cout << NAME << " v" << VERSION << "\n"
              << "Copyright (C) 2020-2025 Yxbcvn410, nefanov\n";
}

struct Arguments {
    bool help = false;
    bool version = false;
    std::string grammar_file;
    std::string input_text;
    std::string input_file;
    std::string output_file;
};

Arguments parseArguments(int argc, char** argv) {
    Arguments args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            args.help = true;
        } else if (arg == "--version") {
            args.version = true;
        } else if (arg == "--grammar" || arg == "-g") {
            if (i + 1 >= argc) die("Missing argument for grammar file");
            args.grammar_file = argv[++i];
        } else if (arg == "--input" || arg == "-i") {
            if (i + 1 >= argc) die("Missing argument for input text");
            args.input_text = argv[++i];
        } else if (arg == "--input-file" || arg == "-f") {
            if (i + 1 >= argc) die("Missing argument for input file");
            args.input_file = argv[++i];
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 >= argc) die("Missing argument for output file");
            args.output_file = argv[++i];
        } else if (arg[0] == '-') {
            die("Unknown option: " + arg);
        } else {
            // Positional argument (treated as input text)
            if (args.input_text.empty()) {
                args.input_text = arg;
            } else {
                warn("Ignoring extra positional argument: " + arg);
            }
        }
    }
    
    return args;
}

int main(int argc, char **argv) {
    Arguments args = parseArguments(argc, argv);

    if (args.help) {
        displayHelp(argv[0]);
        return 0;
    }
    
    if (args.version) {
        displayVersion();
        return 0;
    }

    // Warnings
    if (args.input_text.empty() && args.input_file.empty()) {
        warn("text file not specified, string will be acquired via stdin");
    }
    if (args.output_file.empty()) {
        warn("output file not specified, tree will be placed to stdout");
    }

    // Init grammar
    grammar _grammar;
    if (!args.grammar_file.empty()) {
        try {
            auto grammar_in = std::ifstream(args.grammar_file, std::ios::in);
            if (grammar_in.bad() || grammar_in.fail())
                die("grammar file missing or corrupt");
            grammar_in >> _grammar;
        } catch (std::exception &e) { die(e.what()); }
    } else {
        die("grammar file not specified");
    }
    
    if (!_grammar.is_binary_normal_form()) {
        die("grammar form is not binary normal");
    }

    // Init text
    std::string text;
    if (!args.input_file.empty()) {
        auto text_in = std::ifstream(args.input_file, std::ios::in);
        if (text_in.bad() || text_in.fail())
            die("input text file missing or corrupt");
        text_in >> text;
    } else if (!args.input_text.empty()) {
        text = args.input_text;
    } else {
        std::cerr << INPUT;
        std::cin >> text;
    }

    // Build derivation
    std::vector<int> string;
    try {
        string = _grammar.convert_text(text);
    } catch (std::exception &e) {
        die("failed to resolve some symbols of string to parse");
    }
    
    derivation_table table = build_derivation(_grammar, string);
    table.set_start(_grammar.get_start_symbol());
    table.set_alphabet(_grammar.get_alphabet());
    
    if (!table[{0, table.size()}].count(_grammar.get_start_symbol())) {
        die("provided string cannot be derived with given grammar");
    }

    // Write tree
    if (!args.output_file.empty()) {
        std::ofstream stream(args.output_file, std::ios::out);
        stream << table;
    } else {
        std::cout << std::endl << table << std::endl;
    }
    
    return 0;
}
