#include "syntax-tree.hpp"
#include "debug.hpp"

std::ostream& operator<<(std::ostream& o, Variables& variables) {
    std::vector<Variables::VariableMap> scoped_variables = variables.scoped_variables;
    for (int i = 0; i < scoped_variables.size(); i++) {
        o << "Scope " << i << ":" << std::endl;
        for (std::pair<std::string, int> variable_map_entry : scoped_variables[i]) {
            o << variable_map_entry.first << " = " << variable_map_entry.second << std::endl;
        }
    }
    return o;
}


int Variables::get_variable_value(const std::string& variable_name) {
    int scope_limit = function_scope_indices.top();
    for (int i = scoped_variables.size() - 1; i >= scope_limit; i--) {
        VariableMap& variable_map = scoped_variables[i];
        if (variable_map.find(variable_name) != variable_map.end()) return variable_map[variable_name];
    }
    std::cerr << "ERROR: COULD NOT FIND VALUE FOR VARIABLE " << variable_name << std::endl;
    return -1;
}

void Variables::assign_variable_and_initialize_if_necessary(const std::string& variable_name, int value) {
    int scope_limit = function_scope_indices.top();
    for (int i = scoped_variables.size() - 1; i >= scope_limit; i--) {
        VariableMap& variable_map = scoped_variables[i];
        if (variable_map.find(variable_name) != variable_map.end()) {
            variable_map[variable_name] = value;
            return;
        }
    }

    scoped_variables.back()[variable_name] = value;
}

void Variables::enter_block_scope() {
    scoped_variables.push_back(VariableMap());
}

void Variables::exit_block_scope() {
    scoped_variables.pop_back();
}

void Variables::enter_function_scope() {
    scoped_variables.push_back(VariableMap());
    function_scope_indices.push(scoped_variables.size() - 1);
}

void Variables::exit_function_scope() {
    scoped_variables.pop_back();
    function_scope_indices.pop();
}

std::string get_node_type_string_from_enum(SyntaxTreeNodeType type) {
    switch (type) {
        case SyntaxTreeNodeType::STATEMENT_SEQUENCE:
            return "STATEMENT_SEQUENCE";
        case SyntaxTreeNodeType::OPERAND:
            return "OPERAND";
        case SyntaxTreeNodeType::RETURN:
            return "RETURN";
        case SyntaxTreeNodeType::ASSIGNMENT:
            return "ASSIGNMENT";
        case SyntaxTreeNodeType::BINARY_OPERATION:
            return "BINARY_OPERATION";
        case SyntaxTreeNodeType::IF_ELSE:
            return "IF_ELSE";
        case SyntaxTreeNodeType::FUNCTION_CALL:
            return "FUNCTION";
        case SyntaxTreeNodeType::PRINT:
            return "PRINT";
        case SyntaxTreeNodeType::EMPTY:
            return "EMPTY";
        case SyntaxTreeNodeType::WHILE:
            return "WHILE";
    }
}

std::ostream& operator<<(std::ostream& o, const SyntaxTreeNode* node) {
    std::string node_type_string = get_node_type_string_from_enum(node->node_type);
    o << node_type_string << " NODE";
    return o;
}


SyntaxTreeNode::EvaluationResult StatementSequenceNode::evaluate() {
    EvaluationResult result;
    for (SyntaxTreeNode* node : statements) {
        EvaluationResult node_result = node->evaluate();
        if (node_result.should_return) {
            result.return_value =  node_result.return_value;
            result.should_return = true;
            return result;
        }
    }
    return result;
}

SyntaxTreeNode::EvaluationResult OperandNode::evaluate() {
    EvaluationResult result;
    switch (operand_type) {
        case IDENTIFIER:
            result.expression_value = variables.get_variable_value(identifier_value);
            break;
        case LITERAL:
            result.expression_value = literal_value;
            break;
    }
    return result;
}

SyntaxTreeNode::EvaluationResult ReturnNode::evaluate() {
    EvaluationResult result;
    result.return_value = value->evaluate().expression_value;
    result.should_return = true;
    return result;
}

SyntaxTreeNode::EvaluationResult AssignmentNode::evaluate() {
    EvaluationResult result;
    EvaluationResult assignment_value_result = value->evaluate();
    int assignment_value = assignment_value_result.expression_value;
    if (value->node_type == SyntaxTreeNodeType::FUNCTION_CALL) assignment_value = assignment_value_result.return_value;
    variables.assign_variable_and_initialize_if_necessary(variable_name, assignment_value);
    result.expression_value = 1;
    return result;
}

SyntaxTreeNode::EvaluationResult BinaryOperationNode::evaluate() {
    EvaluationResult result;
    int left_value = left_operand->evaluate().expression_value;
    int right_value = right_operand->evaluate().expression_value;

    int expression_value = 0; 
    switch(operation) {
        case BinaryOperation::ADD:
            expression_value = left_value + right_value;
            break;
        case BinaryOperation::SUBTRACT:
            expression_value = left_value - right_value;
            break;
        case BinaryOperation::MULTIPLY:
            expression_value = left_value * right_value;
            break;
        case BinaryOperation::DIVIDE:
            expression_value = left_value / right_value;
            break;
        case BinaryOperation::MOD:
            expression_value = left_value % right_value;
            break;
        case BinaryOperation::LESS:
            expression_value = left_value < right_value;
            break;
        case BinaryOperation::LESS_EQUAL:
            expression_value = left_value <= right_value;
            break;
        case BinaryOperation::GREATER:
            expression_value = left_value > right_value;
            break;
        case BinaryOperation::GREATER_EQUAL:
            expression_value = left_value >= right_value;
            break;
        case BinaryOperation::EQUAL:
            expression_value = left_value == right_value;
            break;
        case BinaryOperation::NOT_EQUAL:
            expression_value = left_value != right_value;
            break;
        case BinaryOperation::AND:
            expression_value = (left_value != 0) && (right_value != 0);
            break;
        case BinaryOperation::OR:
            expression_value = (left_value != 0) || (right_value != 0);
            break;
    }

    result.expression_value = expression_value;
    return result;
}

SyntaxTreeNode::EvaluationResult IfElseNode::evaluate() {
    int condition_value = condition->evaluate().expression_value;
    if (condition_value) return if_block->evaluate();
    else return else_block->evaluate();
}

SyntaxTreeNode::EvaluationResult FunctionNode::evaluate() {
    std::map<std::string, int> argument_values; 
    for (std::pair<std::string, SyntaxTreeNode*> argument : arguments) {
        std::string& variable_name = argument.first;
        SyntaxTreeNode* node = argument.second; 

        int value = node->evaluate().expression_value;

        argument_values[variable_name] = value;
    }

    variables.enter_function_scope();
    for (std::pair<std::string, int> argument_value : argument_values) {
        variables.assign_variable_and_initialize_if_necessary(argument_value.first, argument_value.second);
    }

    EvaluationResult result = body->evaluate();
    result.should_return = false;

    variables.exit_function_scope();
    return result;
}

SyntaxTreeNode::EvaluationResult EmptyNode::evaluate() {
    EvaluationResult result;
    return result;
}

SyntaxTreeNode::EvaluationResult PrintNode::evaluate() {
    EvaluationResult value_result = value->evaluate();
    int to_print = 0;
    if (value->node_type == FUNCTION_CALL) to_print = value_result.return_value;
    else to_print = value_result.expression_value;
    std::cout << to_print << std::endl;
    return EvaluationResult();
}

SyntaxTreeNode::EvaluationResult WhileNode::evaluate() {
    EvaluationResult result;
    while (condition->evaluate().expression_value == 1) {
        variables.enter_block_scope();
        EvaluationResult current_iteration_result = body->evaluate();
        if (current_iteration_result.should_return) {
            result.should_return = true;
            result.return_value = current_iteration_result.return_value;
            break;
        }
        variables.exit_block_scope();
    }
    return result;
}