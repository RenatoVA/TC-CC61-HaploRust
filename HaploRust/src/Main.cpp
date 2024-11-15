#include <cstdlib>
#include <iostream>

#include "HaploRustLexer.h"
#include "HaploRustParser.h"
#include "HaploRustDriver.h"

using namespace antlr4;
using namespace std;

int main(int argc, const char *argv[]) {

  ifstream ifile;

  if (argc > 1) {
    ifile.open(argv[1]);
    if (!ifile.is_open()) {
      exit(-1);
    }
  }

  istream &stream = argc > 1 ? ifile : cin;
  
  ANTLRInputStream input(stream);
  HaploRustLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  HaploRustParser parser(&tokens);

  tree::ParseTree *tree = parser.program();
  HaploRustDriver *driver = new HaploRustDriver();
  driver->visit(tree);

  return 0;
}