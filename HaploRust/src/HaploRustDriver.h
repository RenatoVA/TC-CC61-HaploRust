#include "antlr4-runtime.h"
#include "HaploRustBaseVisitor.h"
#include "HaploRustLexer.h"
#include "HaploRustParser.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>

using namespace antlr4;
using namespace llvm;

using namespace std;

class HaploRustDriver : public HaploRustVisitor
{
private:
    LLVMContext context;
    unique_ptr<Module> module;
    unique_ptr<IRBuilder<>> builder;

public:
    HaploRustDriver()
    {
    }

    std::any visitProgram(HaploRustParser::ProgramContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitStatement(HaploRustParser::StatementContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitVariableDecl(HaploRustParser::VariableDeclContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitTupleType(HaploRustParser::TupleTypeContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitFunctionDecl(HaploRustParser::FunctionDeclContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitReturnStmt(HaploRustParser::ReturnStmtContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitParameters(HaploRustParser::ParametersContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitParameter(HaploRustParser::ParameterContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitPrintStmt(HaploRustParser::PrintStmtContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitForLoop(HaploRustParser::ForLoopContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitWhileLoop(HaploRustParser::WhileLoopContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitMatchStmt(HaploRustParser::MatchStmtContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitMatchArm(HaploRustParser::MatchArmContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitAssignment(HaploRustParser::AssignmentContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitExprStmt(HaploRustParser::ExprStmtContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitIfStmt(HaploRustParser::IfStmtContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitTupleAccess(HaploRustParser::TupleAccessContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitMulDiv(HaploRustParser::MulDivContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitAddSub(HaploRustParser::AddSubContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitParens(HaploRustParser::ParensContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitString(HaploRustParser::StringContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitTernaryOp(HaploRustParser::TernaryOpContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitNot(HaploRustParser::NotContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitIdentifier(HaploRustParser::IdentifierContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitNumber(HaploRustParser::NumberContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitComparison(HaploRustParser::ComparisonContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitNegate(HaploRustParser::NegateContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitBoolean(HaploRustParser::BooleanContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitCallFunction(HaploRustParser::CallFunctionContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitTuple(HaploRustParser::TupleContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitFunctionCall(HaploRustParser::FunctionCallContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitArguments(HaploRustParser::ArgumentsContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitCondition(HaploRustParser::ConditionContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitComparisonOp(HaploRustParser::ComparisonOpContext *ctx) override
    {
        return visitChildren(ctx);
    }

    std::any visitType(HaploRustParser::TypeContext *ctx) override
    {
        return visitChildren(ctx);
    }
};