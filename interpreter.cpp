#include "interpreter.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "debug.hpp"

std::ostream& operator<<(std::ostream& o, Interpreter::Line& line) {
    for (Interpreter::Token token : line) {
        o << token << " ";
    }
    o << std::endl;
    return o;
}

void Interpreter::preprocess_input_string(std::string& input_string) {
    std::string temp = input_string;
    input_string = "";
    for (char c : temp) {
        if (c == '{' || c == '}') {
            input_string.push_back('\n');
            input_string.push_back(c);
            input_string.push_back('\n');
        } else input_string.push_back(c);
    }

    temp = input_string;
    input_string = "";
    for (char c : temp) {
        if (c == '(' || c == ')' || c == ',') {
            input_string.push_back(' ');
            input_string.push_back(c);
            input_string.push_back(' ');
        } else input_string.push_back(c);
    }

    temp = input_string;
    input_string = "";
    bool seen_char = false;
    for (int i = 0; i < temp.size(); i++) {
        char c = temp[i];
        if (c == '\n') seen_char = false;
        else if (c == ' ' || c == '\t') {
            if (!seen_char) continue;
        }
        else seen_char = true;

        input_string.push_back(c);
    }

    temp = input_string;
    input_string = "";
    for (int i = 0; i < temp.size(); i++) {
        if (
            temp[i] == '\n' && 
            (i == temp.size() - 1 || (i > 0 && temp[i - 1] == '\n'))
        ) continue;
        else input_string.push_back(temp[i]);
    }
}

void Interpreter::read_input_file_and_parse_into_tokens() {

    std::ifstream input_file_stream(input_file_path);
    std::stringstream input_string_stream;
    input_string_stream << input_file_stream.rdbuf();
    std::string input_string = input_string_stream.str();
    preprocess_input_string(input_string);
    input_string_stream = std::stringstream(input_string);

    std::string line_string;
    Line current_line;
    while (getline(input_string_stream, line_string)) {
        current_line.clear();
        std::stringstream line_stream(line_string);
        Token token;
        while (line_stream >> token) {
            current_line.push_back(token);
        }
        lines.push_back(current_line);
    }

    total_lines = lines.size();

}

bool Interpreter::token_is_function_name(Token& token) {
    return function_map.find(token) != function_map.end();
}

bool Interpreter::line_is_lone_function_call(Line& line) {
    Token first_token = line[0];
    return token_is_function_name(first_token);
}

SyntaxTreeNode* Interpreter::parse_lone_function_call_node(int& start_line) {
    SyntaxTreeNode* function_call_node = parse_function_call_node(start_line);
    start_line++;
    return function_call_node;
}

Interpreter::StatementNodeType Interpreter::get_next_statement_node_type(int& start_line) {
    Line& line = lines[start_line];
    int num_tokens = line.size();
    if (num_tokens > 1 && line[1] == "=") return StatementNodeType::ASSIGNMENT;
    else if (line[0] == "if") return StatementNodeType::IF_ELSE;
    else if (line[0] == "return") return StatementNodeType::RETURN;
    else if (line[0] == "print") return StatementNodeType::PRINT;
    else if (line[0] == "function") return StatementNodeType::FUNCTION_DEFINITION;
    else if (line[0] == "while") return StatementNodeType::WHILE;
    else if (line_is_lone_function_call(line)) return StatementNodeType::LONE_FUNCTION_CALL;
    else {
        std::cerr << "Error: unidentified unit node type" << std::endl;
        return ASSIGNMENT;
    }
}

BinaryOperation Interpreter::binary_operation_token_to_enum(const Token& token) {
    if (token == "+") return BinaryOperation::ADD;
    else if (token == "-") return BinaryOperation::SUBTRACT;
    else if (token == "*") return BinaryOperation::MULTIPLY;
    else if (token == "/") return BinaryOperation::DIVIDE;
    else if (token == "%") return BinaryOperation::MOD;
    else if (token == "<") return BinaryOperation::LESS;
    else if (token == "<=") return BinaryOperation::LESS_EQUAL;
    else if (token == ">") return BinaryOperation::GREATER;
    else if (token == ">=") return BinaryOperation::GREATER_EQUAL;
    else if (token == "==") return BinaryOperation::EQUAL;
    else if (token == "!=") return BinaryOperation::NOT_EQUAL;
    else if (token == "&&") return BinaryOperation::AND;
    else if (token == "||") return BinaryOperation::OR;

    std::cerr << "Error: unidentified operation token" << std::endl; 
    return BinaryOperation::ADD;
}

bool Interpreter::token_is_variable_name(const Token& token) {
    char first_letter = token[0];
    if ('0' <= first_letter && first_letter <= '9') return false;
    else return true;
}

int Interpreter::get_literal_value_from_token(const Token& token) {
    return std::stoi(token);
}

SyntaxTreeNode* Interpreter::parse_operand_token(const Token& token) {
    OperandType operand_type = token_is_variable_name(token) ? IDENTIFIER : LITERAL;
    SyntaxTreeNode* operand_node = nullptr;
    if (operand_type == LITERAL) operand_node = new OperandNode(operand_type, get_literal_value_from_token(token), variables);
    else operand_node = new OperandNode(operand_type, token, variables);
    return operand_node;
}

SyntaxTreeNode* Interpreter::parse_binary_operation_node(const Token& left, const Token& op, const Token& right) {
    SyntaxTreeNode* left_operand = parse_operand_token(left);
    SyntaxTreeNode* right_operand = parse_operand_token(right);
    return new BinaryOperationNode(binary_operation_token_to_enum(op), left_operand, right_operand, variables);
}

Interpreter::AssignmentValueType Interpreter::get_assignment_value_type(Line& line, int start_index, int end_index) {
    int length = end_index - start_index + 1;
    if (length == 1) return AssignmentValueType::OPERAND;
    Token possible_function_name = line[start_index];
    if (function_map.find(possible_function_name) != function_map.end()) return AssignmentValueType::FUNCTION_CALL;
    else return AssignmentValueType::BINARY_OPERATION;
}

SyntaxTreeNode* Interpreter::parse_assignment_value_node(int start_line, int start_index, int end_index) {
    Line& line = lines[start_line];
    AssignmentValueType assignment_value_type = get_assignment_value_type(line, start_index, end_index);
    switch(assignment_value_type) {
        case AssignmentValueType::OPERAND: 
            return parse_operand_token(line[start_index]);
        case AssignmentValueType::BINARY_OPERATION: 
            return parse_binary_operation_node(line[start_index], line[start_index + 1], line[start_index + 2]);
        case AssignmentValueType::FUNCTION_CALL: 
            return parse_function_call_node(start_line);
    }
}

Interpreter::FunctionSignatureDetails Interpreter::get_function_signature_details(Line& line, bool is_definition) {
    int function_name_index = 0;
    if (is_definition) function_name_index = 1;
    else {
        while (!token_is_function_name(line[function_name_index])) function_name_index++;
    }
    Token function_name = line[function_name_index];

    int first_input_index = function_name_index + 2;
    std::vector<Token> input_tokens;
    for (int i = first_input_index; line[i] != ")"; i++) {
        if (line[i] != ",") {
            Token input = line[i];
            input_tokens.push_back(input);
        }
    }

    return FunctionSignatureDetails {
        .name = function_name,
        .inputs = input_tokens
    };
}

SyntaxTreeNode* Interpreter::parse_function_call_node(int& start_line) {
    Line& line = lines[start_line];

    FunctionSignatureDetails function_signature_details = get_function_signature_details(line, false);
    Token function_name = function_signature_details.name;
    std::vector<Token> argument_tokens = function_signature_details.inputs;

    std::vector<SyntaxTreeNode*> argument_nodes;
    for (Token token : argument_tokens) {
        SyntaxTreeNode* node = parse_operand_token(token);
        argument_nodes.push_back(node);
    }

    FunctionData function_data = function_map[function_name];
    SyntaxTreeNode* function_body = function_data.body;
    std::vector<Token> parameters = function_data.parameters; 

    std::map<std::string, SyntaxTreeNode*> argument_map;
    for (int i = 0; i < parameters.size(); i++) {
        argument_map[parameters[i]] = argument_nodes[i];
    }

    return new FunctionNode(function_body, argument_map, variables);
}

SyntaxTreeNode* Interpreter::parse_assignment_node(int& start_line) {
    Line& line = lines[start_line];
    Token variable_name = line[0];

    SyntaxTreeNode* assignment_value_node = parse_assignment_value_node(start_line, 2, line.size() - 1);

    start_line++;

    return new AssignmentNode(variable_name, assignment_value_node, variables);
}

int Interpreter::get_closing_brace_line(int opening_brace_line) {
    int num_open_braces = 1;
    int i = opening_brace_line + 1;
    while (i < total_lines) {
        if (lines[i][0] == "{") num_open_braces++;
        else if (lines[i][0] == "}") num_open_braces--;
        if (num_open_braces == 0) return i;
        i++;
    }
    std::cerr << "Error: No closing brace found";
    return -1;
}

SyntaxTreeNode* Interpreter::parse_braces_block(int& start_line) {
    int end_line = get_closing_brace_line(start_line);
    start_line++;
    end_line--;
    SyntaxTreeNode* node = parse_block(start_line, end_line);
    start_line++;
    return node;
}

SyntaxTreeNode* Interpreter::parse_if_else_node(int& start_line) {
    Line& line = lines[start_line];

    SyntaxTreeNode* binary_operation_node = parse_binary_operation_node(line[2], line[3], line[4]);
    start_line++;
    SyntaxTreeNode* if_block_node = parse_braces_block(start_line);
    SyntaxTreeNode* else_block_node = nullptr;
    if (start_line < total_lines && lines[start_line][0] == "else") {
        start_line++;
        else_block_node = parse_braces_block(start_line);
    } else else_block_node = new EmptyNode(variables);

    return new IfElseNode(binary_operation_node, if_block_node, else_block_node, variables);
}

int Interpreter::get_closing_parenthesis_index(Line& line) {
    for (int i = 0; i < line.size(); i++) {
        if (line[i] == ")") return i;
    }
    std::cerr << "Error: did not find closing parenthesis when expected to" << std::endl;
    return -1;
}

SyntaxTreeNode* Interpreter::parse_print_node(int& start_line) {
    Line& line = lines[start_line];
    int closing_parenthesis_index = get_closing_parenthesis_index(line);
    SyntaxTreeNode* print_value_node = parse_assignment_value_node(start_line, 2, closing_parenthesis_index - 1);
    SyntaxTreeNode* node = new PrintNode(print_value_node, variables);
    start_line++;
    return node;
}

SyntaxTreeNode* Interpreter::parse_function_definition(int& start_line) {
    Line& line = lines[start_line];

    FunctionSignatureDetails function_signature_details = get_function_signature_details(line, true);
    Token function_name = function_signature_details.name;
    std::vector<Token> parameters = function_signature_details.inputs;

    start_line++;

    SyntaxTreeNode* function_body_node = parse_braces_block(start_line);

    function_map[function_name] = FunctionData {
        .body = function_body_node,
        .parameters = parameters
    };

    SyntaxTreeNode* empty_node = new EmptyNode(variables);

    return empty_node;
}

SyntaxTreeNode* Interpreter::parse_return_node(int& start_line) {
    Line& line = lines[start_line];
    SyntaxTreeNode* value_node = parse_assignment_value_node(start_line, 1, line.size() - 1);
    start_line++;
    return new ReturnNode(value_node, variables);
}

SyntaxTreeNode* Interpreter::parse_while_node(int& start_line) {
    Line& line = lines[start_line];
    SyntaxTreeNode* condition_node = parse_binary_operation_node(line[2], line[3], line[4]);
    start_line++;
    SyntaxTreeNode* body_node = parse_braces_block(start_line);
    return new WhileNode(variables, condition_node, body_node);
}

SyntaxTreeNode* Interpreter::parse_single_statement_node(int& start_line) {
    Interpreter::StatementNodeType unit_node_type = get_next_statement_node_type(start_line);
    SyntaxTreeNode* node = nullptr;
    switch (unit_node_type) {
        case StatementNodeType::ASSIGNMENT:
            return parse_assignment_node(start_line);
        case StatementNodeType::IF_ELSE:
            return parse_if_else_node(start_line);
        case StatementNodeType::PRINT:
            return parse_print_node(start_line);
        case StatementNodeType::FUNCTION_DEFINITION:
            return parse_function_definition(start_line);
        case StatementNodeType::LONE_FUNCTION_CALL:
            return parse_lone_function_call_node(start_line);
        case StatementNodeType::RETURN:
            return parse_return_node(start_line);
        case StatementNodeType::WHILE:
            return parse_while_node(start_line);

    }
    return nullptr;
}


SyntaxTreeNode* Interpreter::parse_block(int& start_line, int& end_line) {
    std::vector<SyntaxTreeNode*> nodes;
    while (start_line <= end_line) {
        SyntaxTreeNode* node = parse_single_statement_node(start_line);
        nodes.push_back(node);
    }
    if (nodes.size() == 1) return nodes[0];
    else return new StatementSequenceNode(nodes, variables);
}

void Interpreter::run() {
    read_input_file_and_parse_into_tokens();
    int start = 0;
    int end = total_lines - 1;
    SyntaxTreeNode* node = parse_block(start, end);
    node->evaluate();
}






