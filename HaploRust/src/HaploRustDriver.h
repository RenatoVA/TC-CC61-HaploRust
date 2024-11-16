#include "antlr4-runtime.h"
#include "HaploRustBaseVisitor.h"
#include "HaploRustLexer.h"
#include "HaploRustParser.h"

#include <cmath>
#include <map>
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
    struct SymbolInfo
    {
        llvm::Type *type; // Tipo de la variable (por ejemplo, int, float, etc.)
        std::string logicalType;
        llvm::Value *llvmValue; // Referencia a la posición en memoria (AllocaInst, etc.)
    };
    LLVMContext context;
    std::unique_ptr<Module> module;
    std::unique_ptr<IRBuilder<>> builder;
    std::unordered_map<std::string, SymbolInfo> symbolTable;
    FunctionCallee printfFunc;
    FunctionCallee expFunc;

public:
    HaploRustDriver()
    {
        module = std::make_unique<Module>("HaploRustModule", context);
        builder = std::make_unique<IRBuilder<>>(context);

        std::vector<Type *> printfArgs;
        printfArgs.push_back(PointerType::getUnqual(Type::getInt8Ty(context)));

        FunctionType *printfType = FunctionType::get(
            Type::getInt32Ty(context), printfArgs, true);
        printfFunc = module->getOrInsertFunction("printf", printfType);

        FunctionType *mathFuncType = FunctionType::get(
            Type::getDoubleTy(context), {Type::getDoubleTy(context)}, false);

        expFunc = module->getOrInsertFunction("exp", mathFuncType);
    }

    llvm::Type *getLLVMTypeFromLogicalType(const std::string &logicalType, llvm::LLVMContext &context)
    {
        if (logicalType == "int")
        {
            return llvm::Type::getInt32Ty(context);
        }
        else if (logicalType == "float")
        {
            return llvm::Type::getDoubleTy(context);
        }
        else if (logicalType == "string")
        {
            return llvm::PointerType::getUnqual(context); // Puntero a char para cadenas
        }
        else if (logicalType == "bool")
        {
            return llvm::Type::getInt1Ty(context);
        }
        else if (logicalType == "void")
        {
            return llvm::Type::getVoidTy(context); // Manejo del tipo void
        }
        else
        {
            std::cerr << "Error: Tipo no soportado '" << logicalType << "'\n";
            return nullptr;
        }
    }

    std::any visitProgram(HaploRustParser::ProgramContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitProgram\n";

    // Crear la función main
    FunctionType *mainType = FunctionType::get(Type::getInt32Ty(context), false);
    Function *mainFunc = Function::Create(mainType, Function::ExternalLinkage, "main", module.get());

    // Crear el bloque básico de entrada
    BasicBlock *entry = BasicBlock::Create(context, "entry", mainFunc);
    builder->SetInsertPoint(entry);

    // Visitar cada statement
    for (auto stmt : ctx->statement())
    {
        visit(stmt);

        // Verifica si el bloque actual ya tiene un terminador
        if (!builder->GetInsertBlock()->getTerminator())
        {
            llvm::errs() << "Debug: Agregando terminador por defecto al bloque básico\n";
            builder->CreateRet(ConstantInt::get(Type::getInt32Ty(context), 0));
        }
    }

    // Agregar terminador al final de main si no existe
    if (!builder->GetInsertBlock()->getTerminator())
    {
        llvm::errs() << "Debug: Agregando retorno final al main\n";
        builder->CreateRet(ConstantInt::get(Type::getInt32Ty(context), 0));
    }

    // Verificar función y módulo
    if (verifyFunction(*mainFunc, &errs()))
    {
        errs() << "Error: La función main contiene errores\n";
    }
    if (verifyModule(*module, &errs()))
    {
        errs() << "Error: El módulo contiene errores\n";
    }

    // Imprimir el módulo
    module->print(outs(), nullptr);
    return nullptr;
    }

    std::any visitStatement(HaploRustParser::StatementContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitStatement\n";
        if (ctx->functionDecl())
        {
            return visit(ctx->functionDecl());
        }
        else if (ctx->printStmt())
        {
            return visit(ctx->printStmt());
        }
        else if (ctx->forLoop())
        {
            return visit(ctx->forLoop());
        }
        else if (ctx->whileLoop())
        {
            return visit(ctx->whileLoop());
        }
        else if (ctx->variableDecl())
        {
            return visit(ctx->variableDecl());
        }
        else if (ctx->exprStmt())
        {
            return visit(ctx->exprStmt());
        }
        else if (ctx->ifStmt())
        {
            return visit(ctx->ifStmt());
        }

        llvm::errs() << "Error: Tipo de statement no reconocido\n";
        return nullptr;
    }

    std::any visitVariableDecl(HaploRustParser::VariableDeclContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitVariableDecl\n";
        std::string varName = ctx->IDENTIFIER()->getText();
        llvm::errs() << "Debug: Variable identificada: " << varName << "\n";
        std::string logicalType = ctx->type()->getText(); // Obtiene el tipo lógico directamente de la gramática
        llvm::errs() << "Debug: Tipo lógico: " << logicalType << "\n";
        Value *exprValue = std::any_cast<Value *>(visit(ctx->expr()));
        llvm::Type *llvmType = getLLVMTypeFromLogicalType(logicalType, context);
        if (!llvmType)
        {
            std::cerr << "Error: Tipo no soportado para la variable '" << varName << "'\n";
            return nullptr;
        }

        // Validar que el tipo del valor coincide con el tipo lógico
        if (logicalType == "int" && exprValue->getType()->isDoubleTy())
        {
            llvm::errs() << "Debug: Convertir double a int\n";
            exprValue = builder->CreateFPToSI(exprValue, llvm::Type::getInt32Ty(context), "double_to_int");
        }
        else if (logicalType == "float" && exprValue->getType()->isIntegerTy(32))
        {
            llvm::errs() << "Debug: Convertir int a float\n";
            exprValue = builder->CreateSIToFP(exprValue, llvm::Type::getDoubleTy(context), "int_to_double");
        }
        else if ((logicalType == "int" && !exprValue->getType()->isIntegerTy(32)) ||
                 (logicalType == "float" && !exprValue->getType()->isDoubleTy()))
        {
            std::cerr << "Error: Tipo incompatible para la variable '" << varName << "'\n";
            return nullptr;
        }

        AllocaInst *alloc = builder->CreateAlloca(llvmType, 0, varName.c_str());
        builder->CreateStore(exprValue, alloc);

        symbolTable[varName] = {llvmType, logicalType, alloc};
        return exprValue;
    }

    std::any visitFunctionDecl(HaploRustParser::FunctionDeclContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitFunctionDecl\n";

        std::string funcName = ctx->IDENTIFIER()->getText();
        std::string returnTypeStr = ctx->type()->getText();
        llvm::Type *returnType = getLLVMTypeFromLogicalType(returnTypeStr, context);

        if (!returnType)
        {
            llvm::errs() << "Error: Tipo de retorno no soportado para la función " << funcName << "\n";
            return nullptr;
        }

        // Crear el tipo de función
        std::vector<llvm::Type *> paramTypes;
        if (ctx->parameters())
        {
            for (auto &paramCtx : ctx->parameters()->parameter())
            {
                std::string paramTypeStr = paramCtx->type()->getText();
                llvm::Type *paramType = getLLVMTypeFromLogicalType(paramTypeStr, context);
                if (!paramType)
                {
                    llvm::errs() << "Error: Tipo de parámetro no soportado en la función " << funcName << "\n";
                    return nullptr;
                }
                paramTypes.push_back(paramType);
            }
        }

        llvm::FunctionType *funcType = llvm::FunctionType::get(returnType, paramTypes, false);
        llvm::Function *function = llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, funcName, module.get());

        // Crear el bloque de entrada
        llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(context, "entry", function);
        builder->SetInsertPoint(entryBlock);

        // Registrar parámetros en la tabla de símbolos
        auto paramIt = function->arg_begin();
        if (ctx->parameters())
        {
            for (auto &paramCtx : ctx->parameters()->parameter())
            {
                std::string paramName = paramCtx->IDENTIFIER()->getText();
                paramIt->setName(paramName);

                // Reservar espacio para el parámetro en la pila
                llvm::AllocaInst *alloc = builder->CreateAlloca(paramIt->getType(), nullptr, paramName.c_str());
                builder->CreateStore(&(*paramIt), alloc);
                llvm::errs() << "Debug: Registrando parámetro " << paramName
                             << " con tipo " << *(paramIt->getType()) << "\n";

                // Registrar en la tabla de símbolos
                symbolTable[paramName] = {paramIt->getType(), paramCtx->type()->getText(), alloc};
                paramIt++;
            }
        }

        llvm::errs() << "Debug: Parámetros registrados para la función " << funcName << "\n";

        // Visitar las instrucciones en el cuerpo de la función
        for (auto &stmtCtx : ctx->statement())
        {
            visit(stmtCtx);
        }

        // Agregar retorno implícito para funciones void
        if (returnType->isVoidTy())
        {
            builder->CreateRetVoid();
        }

        llvm::verifyFunction(*function);
        return nullptr;
    }

    std::any visitReturnStmt(HaploRustParser::ReturnStmtContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitReturnStmt\n";

        llvm::Value *returnValue = std::any_cast<llvm::Value *>(visit(ctx->expr()));
        if (!returnValue)
        {
            llvm::errs() << "Error: Valor de retorno inválido\n";
            return nullptr;
        }

        builder->CreateRet(returnValue);
        return nullptr;
    }

    std::any visitParameters(HaploRustParser::ParametersContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitParameters\n";
        return visitChildren(ctx);
    }

    std::any visitParameter(HaploRustParser::ParameterContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitParameter\n";
        return visitChildren(ctx);
    }

    std::any visitPrintStmt(HaploRustParser::PrintStmtContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitPrintStmt\n";
        // Evalúa la expresión
        Value *exprValue = std::any_cast<Value *>(visit(ctx->expr()));
        if (!exprValue)
        {
            auto token = ctx->getStart();
            std::cerr << "Error: exprValue es nullptr en visitPrintStmt en línea "
                      << token->getLine() << ", columna " << token->getCharPositionInLine() << "\n";
            return std::any();
        }

        // Determina el formato basado en el tipo lógico
        Value *formatStr = nullptr;

        // Verifica si la expresión es un identificador
        if (auto identifierCtx = dynamic_cast<HaploRustParser::IdentifierContext *>(ctx->expr()))
        {
            std::string varName = identifierCtx->IDENTIFIER()->getText();

            // Verifica que la variable esté en la tabla de símbolos
            if (symbolTable.find(varName) == symbolTable.end())
            {
                std::cerr << "Error: Variable '" << varName << "' no definida\n";
                return std::any();
            }

            // Usa el tipo lógico de la tabla de símbolos
            const std::string &logicalType = symbolTable[varName].logicalType;
            if (logicalType == "float")
            {
                formatStr = builder->CreateGlobalString("%lf\n", "fmt");
            }
            else if (logicalType == "int")
            {
                exprValue = builder->CreateSIToFP(exprValue, Type::getDoubleTy(context), "int_to_double");
                formatStr = builder->CreateGlobalString("%lf\n", "fmt");
            }
            else if (logicalType == "string")
            {
                formatStr = builder->CreateGlobalString("%s\n", "fmt");
            }
            else
            {
                std::cerr << "Error: Tipo no soportado para impresión\n";
                return std::any();
            }
        }
        else if (dynamic_cast<HaploRustParser::NumberContext *>(ctx->expr()))
        {
            // Maneja literales numéricos
            formatStr = builder->CreateGlobalString("%lf\n", "fmt");
        }
        else if (dynamic_cast<HaploRustParser::StringContext *>(ctx->expr()))
        {
            // Maneja literales de cadena
            formatStr = builder->CreateGlobalString("%s\n", "fmt");
        }
        else
        {
            std::cerr << "Error: Tipo no soportado para impresión\n";
            return std::any();
        }

        // Crea la llamada a printf
        std::vector<Value *> printfArgs = {formatStr, exprValue};
        builder->CreateCall(printfFunc, printfArgs, "printf_call");

        return std::any();
    }

    std::any visitForLoop(HaploRustParser::ForLoopContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitForLoop\n";
        return visitChildren(ctx);
    }

    std::any visitWhileLoop(HaploRustParser::WhileLoopContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitWhileLoop\n";
        return visitChildren(ctx);
    }

    std::any visitExprStmt(HaploRustParser::ExprStmtContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitExprStmt\n";
        return visitChildren(ctx);
    }

    std::any visitIfStmt(HaploRustParser::IfStmtContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitIfStmt\n";

        // Generar la condición
        Value *condValue = std::any_cast<Value *>(visit(ctx->condition()));
        if (!condValue)
        {
            llvm::errs() << "Error: Condición no válida en if\n";
            return nullptr;
        }

        // Crear los bloques básicos para "then", "else" y "merge"
        Function *currentFunction = builder->GetInsertBlock()->getParent();

        BasicBlock *thenBlock = BasicBlock::Create(context, "then", currentFunction);
        BasicBlock *elseBlock = nullptr;
        BasicBlock *mergeBlock = BasicBlock::Create(context, "merge", currentFunction);

        if (ctx->statement(1)) // Si existe un bloque "else"
        {
            elseBlock = BasicBlock::Create(context, "else", currentFunction);
        }

        // Instrucción de salto condicional
        if (elseBlock)
        {
            builder->CreateCondBr(condValue, thenBlock, elseBlock);
        }
        else
        {
            builder->CreateCondBr(condValue, thenBlock, mergeBlock);
        }

        // Emitir código para el bloque "then"
        builder->SetInsertPoint(thenBlock);
        visit(ctx->statement(0)); // Visitar las declaraciones del bloque "then"
        builder->CreateBr(mergeBlock);

        // Emitir código para el bloque "else" (si existe)
        if (elseBlock)
        {
            builder->SetInsertPoint(elseBlock);
            visit(ctx->statement(1)); // Visitar las declaraciones del bloque "else"
            builder->CreateBr(mergeBlock);
        }

        // Continuar en el bloque "merge"
        builder->SetInsertPoint(mergeBlock);

        return nullptr;
    }

    std::any visitMulDiv(HaploRustParser::MulDivContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitMulDiv\n";
        Value *left = std::any_cast<Value *>(visit(ctx->expr(0)));
        Value *right = std::any_cast<Value *>(visit(ctx->expr(1)));

        int opType = ctx->op->getType();

        if (opType == HaploRustParser::MUL)
        {
            return builder->CreateFMul(left, right, "multmp");
        }
        else
        {
            return builder->CreateFDiv(left, right, "divtmp");
        }
    }

    std::any visitAddSub(HaploRustParser::AddSubContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitAddSub\n";
        return visitChildren(ctx);
    }

    std::any visitParens(HaploRustParser::ParensContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitParens\n";
        return visit(ctx->expr());
    }

    std::any visitString(HaploRustParser::StringContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitString\n";
        // Obtén el texto del literal de cadena (sin las comillas)
        std::string stringValue = ctx->getText();
        if (stringValue.front() == '"' && stringValue.back() == '"')
        {
            stringValue = stringValue.substr(1, stringValue.size() - 2); // Elimina las comillas
        }

        llvm::errs() << "Debug: Literal de cadena procesado: " << stringValue << "\n";

        // Crea un GlobalStringPtr para el literal
        Value *stringPtr = builder->CreateGlobalString(stringValue, "string_literal");

        // Retorna el puntero a la cadena
        return stringPtr;
    }

    std::any visitIdentifier(HaploRustParser::IdentifierContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitIdentifier\n";
        std::string varName = ctx->IDENTIFIER()->getText();
        if (symbolTable.find(varName) == symbolTable.end())
        {
            // Si la variable no está definida, muestra un error
            auto token = ctx->getStart();
            errs() << "Error: Variable no definida: " << varName
                   << " en línea " << token->getLine()
                   << ", columna " << token->getCharPositionInLine() << "\n";
            return nullptr;
        }

        // Obtén la información de la tabla de símbolos
        auto &symbolInfo = symbolTable[varName];
        Type *varType = symbolInfo.type;
        const std::string &logicalType = symbolInfo.logicalType;
        llvm::errs() << "Debug: Variable '" << varName 
             << "' tiene logicalType: " << logicalType << "\n";

        // Carga el valor de la variable desde la memoria
        Value *value = builder->CreateLoad(varType, symbolInfo.llvmValue, varName.c_str());

        // Manejo adicional para booleanos y cadenas
        if (logicalType == "bool")
        {
            // Convertir a booleano si es necesario (i1 ya está representado como booleano en LLVM)
            llvm::errs() << "Debug: Variable es un booleano\n";
        }
        else if (logicalType == "string")
        {
            // Manejar cadenas (char pointers)
            llvm::errs() << "Debug: Variable es una cadena\n";
            // Retorna directamente el puntero a la cadena
            return value;
        }
        else if (logicalType == "int")
        {
            llvm::errs() << "Debug: Variable es un entero\n";
        }
        else if (logicalType == "float")
        {
            llvm::errs() << "Debug: Variable es un flotante\n";
        }
        else
        {
            llvm::errs() << "Error: Tipo no soportado para la variable '" << varName << "'\n";
            return nullptr;
        }

        return value;
    }

    std::any visitNumber(HaploRustParser::NumberContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitNumber\n";
        std::string numText = ctx->NUMBER()->getText();

        // Determina si el número es entero o flotante
        if (numText.find('.') != std::string::npos)
        {
            // Número con punto decimal => double
            auto numVal = std::stod(numText);
            llvm::Value *val = llvm::ConstantFP::get(context, llvm::APFloat(numVal));
            return std::any(val);
        }
        else
        {
            // Número entero => int
            auto numVal = std::stoi(numText);
            llvm::Value *val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), numVal);
            return std::any(val);
        }
    }

    std::any visitBoolean(HaploRustParser::BooleanContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitBoolean\n";
        return visitChildren(ctx);
    }

    std::any visitCallFunction(HaploRustParser::CallFunctionContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitCallFunction\n";

        // Obtener el contexto de `functionCall`
        auto funcCallCtx = ctx->functionCall();

        if (!funcCallCtx)
        {
            llvm::errs() << "Error: Contexto de functionCall no encontrado\n";
            return nullptr;
        }

        // Obtener el nombre de la función
        std::string funcName = funcCallCtx->IDENTIFIER()->getText();
        llvm::Function *function = module->getFunction(funcName);

        if (!function)
        {
            llvm::errs() << "Error: Función no definida: " << funcName << "\n";
            return nullptr;
        }

        // Procesar argumentos
        std::vector<llvm::Value *> args;
        if (funcCallCtx->arguments())
        {
            for (auto &argCtx : funcCallCtx->arguments()->expr())
            {
                llvm::Value *argValue = std::any_cast<llvm::Value *>(visit(argCtx));
                args.push_back(argValue);
            }
        }

        // Crear llamada a función
        return builder->CreateCall(function, args, "calltmp");
    }

    std::any visitFunctionCall(HaploRustParser::FunctionCallContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitFunctionCall\n";
        return visitChildren(ctx);
    }

    std::any visitArguments(HaploRustParser::ArgumentsContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitArguments\n";
        return visitChildren(ctx);
    }

    std::any visitCondition(HaploRustParser::ConditionContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitCondition\n";

        // Visitar las expresiones izquierda y derecha
        Value *lhs = std::any_cast<Value *>(visit(ctx->expr(0)));
        Value *rhs = std::any_cast<Value *>(visit(ctx->expr(1)));

        if (!lhs || !rhs)
        {
            llvm::errs() << "Error: Operandos inválidos en la condición\n";
            return nullptr;
        }

        // Obtener el operador de comparación
        std::string opText = ctx->comparisonOp()->getText();
        llvm::errs() << "Debug: Operador de comparación detectado: " << opText << "\n";

        // Generar la instrucción LLVM correspondiente
        if (lhs->getType()->isIntegerTy())
        {
            llvm::CmpInst::Predicate pred;
            if (opText == "==")
                pred = llvm::CmpInst::ICMP_EQ;
            else if (opText == "!=")
                pred = llvm::CmpInst::ICMP_NE;
            else if (opText == "<")
                pred = llvm::CmpInst::ICMP_SLT;
            else if (opText == ">")
                pred = llvm::CmpInst::ICMP_SGT;
            else if (opText == "<=")
                pred = llvm::CmpInst::ICMP_SLE;
            else if (opText == ">=")
                pred = llvm::CmpInst::ICMP_SGE;
            else
            {
                llvm::errs() << "Error: Operador de comparación desconocido: " << opText << "\n";
                return nullptr;
            }
            return builder->CreateICmp(pred, lhs, rhs, "cmp");
        }
        else if (lhs->getType()->isFloatingPointTy())
        {
            llvm::CmpInst::Predicate pred;
            if (opText == "==")
                pred = llvm::CmpInst::FCMP_OEQ;
            else if (opText == "!=")
                pred = llvm::CmpInst::FCMP_ONE;
            else if (opText == "<")
                pred = llvm::CmpInst::FCMP_OLT;
            else if (opText == ">")
                pred = llvm::CmpInst::FCMP_OGT;
            else if (opText == "<=")
                pred = llvm::CmpInst::FCMP_OLE;
            else if (opText == ">=")
                pred = llvm::CmpInst::FCMP_OGE;
            else
            {
                llvm::errs() << "Error: Operador de comparación desconocido: " << opText << "\n";
                return nullptr;
            }
            return builder->CreateFCmp(pred, lhs, rhs, "cmp");
        }

        llvm::errs() << "Error: Tipo no soportado para comparación\n";
        return nullptr;
    }

    std::any visitComparisonOp(HaploRustParser::ComparisonOpContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitComparisonOp\n";

        // Devuelve el texto del operador de comparación
        std::string opText = ctx->getText();

        llvm::errs() << "Debug: Operador de comparación: " << opText << "\n";

        // Retorna el operador como texto para que otros visitantes puedan usarlo
        return opText;
    }

    std::any visitType(HaploRustParser::TypeContext *ctx) override
    {
        llvm::errs() << "Debug: Entrando a visitType\n";
        return visitChildren(ctx);
    }
};