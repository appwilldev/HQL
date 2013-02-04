#include <iostream>
#include <fstream>
#include <vector>

using std::vector;
using std::ifstream;

#include "cmn_util.hpp"

#include "ast.hpp"
#include "cmd.hpp"
#include "parser.hpp"
#include "type_config.hpp"

#define YY_EXTRA_TYPE void*
typedef void* yyscan_t;

extern int yylex(yyscan_t);
extern int yylex_init(yyscan_t*);
extern int yylex_init_extra(YY_EXTRA_TYPE, yyscan_t*);
extern int yyparse(yyscan_t);
extern int yylex_destroy(yyscan_t);

int main(int argc, char* argv[]){
    ifstream tc;
    string json, tmp;
    tc.open("test/type.json", ifstream::in);
    while(!tc.eof()){
        tmp.clear();
        tc >> tmp;
        json += tmp;
    }
    tc.close();
    TypeConfig::setup_from_json(json);

    yyscan_t scanner;
    ShellState *shell = new ShellState();
    yylex_init_extra(shell, &scanner);
    yyparse(scanner);
    yylex_destroy(scanner);
    delete shell;
    TypeConfig::clear();
}
