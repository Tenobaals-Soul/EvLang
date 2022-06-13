#include<parser.h>
#include<stdlib.h>

struct parse_args {
    bool has_errors;
    StackedData* context;
    TokenList tokens;
};

typedef struct state_data {
    unsigned int index;
    unsigned int flags;
    Expression* exp;
} state_data;

typedef struct state {
    struct state (*func)(state_data*);
} state;

#define END_FINE (1 << 2)
#define NO_ADVANCE (1 << 2)
#define END_ERROR (1 << 3)

#define transition1(new_state_f) { data->flags = 0; data->on_eof = NULL; return (state) { new_state_f }; }
#define transition2(new_state_f, status) { data->flags = status; data->on_eof = NULL; return (state) { new_state_f }; }
#define transition3(new_state_f, status, on_eof) { data->flags = status; data->on_eof = on_eof; return (state) { new_state_f }; }
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define transition(...) GET_MACRO(__VA_ARGS__, transition3, transition2, transition1)(__VA_ARGS__)

static inline bool parse_with_context(StringDict* dict_to_parse, struct parse_args* args);

void free_expression(Expression* exp) {
    switch (exp->expression_type) {
    default:
        break;
    }
    free(exp);
}

state parse_exp_start(state_data* data) {
    Token* token = get_token(data);
    switch (token->type) {
    case FIXED_VALUE_TOKEN:
        //transition();
    default:
        throw_raw_expected(token, "expected \"{\"");
        //transition(NULL, END_ERROR);
    }
}

Expression* parse_expression(StackedData* method, struct parse_args* args) {
    state next_state = { parse_exp_start };
    bool error = false;
    state_data data = {
        .index = 0,
        .flags = 0,
        .exp = NULL
    };
    while (next_state.func) {
        if (data.index >= method->method.text_start) {
            if (data.flags & END_FINE) {
                return data.exp;
            }
            else {
                throw_unexpected_end(&args->tokens.tokens[data.index - 1]);
                error = true;
                break;
            }
        }
        next_state = next_state.func(&data);
        if (data.flags & END_ERROR) {
            error = true;
        }
        data.index++;
    }
    if (error) {
        data.index--;
        free_expression(data.exp);
        return NULL;
    }
    data.index--;
    return data.exp;
}

void parse_method(StackedData* method, struct parse_args* args) {

}

void parse_dict_item(void* env, const char* key, void* val) {
    struct parse_args* args = env;
    StackedData* st_data = val;
    switch (st_data->type) {
    case ENTRY_CLASS:
        struct parse_args new_args = {
            st_data,
            false
        };
        parse_with_context(st_data->class.class_content, &new_args);
        args->has_errors |= new_args.has_errors;
        break;
    case ENTRY_ERROR:
        break;
    case ENTRY_METHOD:
        parse_method(st_data, args);
        break;
    case ENTRY_METHOD_TABLE:
        for (unsigned int i = 0; i < st_data->method_table.len; i++) {
            parse_method(st_data->method_table.methods[i], args);
        }
        break;
    case ENTRY_VARIABLE:
        if (st_data->var.text_start && st_data->var.text_end) {
            st_data->var.exec_text = parse_expression(st_data, args);
        }
        else {
            st_data->var.exec_text = NULL;
        }
        break;
    }
}

static inline bool parse_with_context(StringDict* dict_to_parse, struct parse_args* args) {
    string_dict_complex_foreach(dict_to_parse, parse_dict_item, args);
}

bool parse(StringDict* dict_to_parse, TokenList tokenlist) {
    struct parse_args args = {
        false,
        NULL,
        tokenlist
    };
    parse_with_context(dict_to_parse, &args);
    return args.has_errors;
}