#include <fstream>
#include <iostream>
#include <cstring>

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>

int main(int argc, char** argv) {
    using namespace llvm;
    LLVMContext ctx;
    
    // Создание модуля с C++-совместимым data layout для x86-64 Ubuntu
    auto irModule = std::make_unique<Module>("main", ctx);
    irModule->setDataLayout("e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128");
    irModule->setTargetTriple("x86_64-pc-linux-gnu");

    FunctionType* GetcharFT = FunctionType::get(Type::getInt32Ty(ctx), false);
    Function* Getchar = Function::Create(GetcharFT, Function::ExternalLinkage, "getchar");
    irModule->getFunctionList().push_back(Getchar);

    FunctionType* PutcharFT = FunctionType::get(Type::getInt32Ty(ctx), { Type::getInt8Ty(ctx) }, false);
    Function* Putchar = Function::Create(PutcharFT, Function::ExternalLinkage, "putchar");
    irModule->getFunctionList().push_back(Putchar);

    FunctionType* MainFT = FunctionType::get(Type::getInt32Ty(ctx), false);
    Function* Main = Function::Create(MainFT, Function::ExternalLinkage, "main");
    irModule->getFunctionList().push_back(Main);
    BasicBlock* entry = BasicBlock::Create(ctx, "entry", Main);

    IRBuilder<> builder(entry);

    auto* a = builder.CreateCall(GetcharFT, Getchar, {}, "a");

    auto* b = builder.getInt32(1);
    
    auto* c = builder.CreateAdd(a, b, "c");

    auto* truncC = builder.CreateTrunc(c, Type::getInt8Ty(ctx), "truncC");

    builder.CreateCall(PutcharFT, Putchar, truncC); 
    builder.CreateRet(builder.getInt32(0));

    // Проверка корректности модуля
    verifyFunction(*Main);
    
    // Вывод IR в файл
    std::error_code EC;
    if (argc == 2 && std::strcmp(argv[1], "f") == 0)  {
        raw_fd_ostream out("main.ll", EC, sys::fs::OF_None);
        irModule->print(out, nullptr);
    } else {
        raw_os_ostream out(std::cout);
        irModule->print(out, nullptr);
    }

    return 0;
}

// g++  main.cpp $(llvm-config --cxxflags --ldflags --system-libs --libs core) -o llvm_builder