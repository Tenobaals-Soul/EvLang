#include<parser.h>
#include<stdlib.h>

struct parse_args {
    StackedData* context;
    bool has_errors;
};

static inline bool parse_with_context(StringDict* dict_to_parse, struct parse_args* args);

Expression* parse_expression(StackedData* method, struct parse_args* args) {

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
        if (st_data->text) {
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

bool parse(StringDict* dict_to_parse) {
    struct parse_args args = {
        NULL,
        false
    };
    parse_with_context(dict_to_parse, &args);
    return args.has_errors;
}