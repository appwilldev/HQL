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

extern int yylex();
extern int yyparse();
extern int yylex_destroy(void);

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
    yyparse();
    yylex_destroy();
    TypeConfig::clear();
}
