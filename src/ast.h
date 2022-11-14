#pragma once

#include <iostream>
#include <memory>
#include <string>

// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
    virtual std::string Koopa() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override
    {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }
    std::string Koopa() const override { return func_def->Koopa(); }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override
    {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout << " }";
    }
    std::string Koopa() const override
    {
        return "fun @" + ident + "(): " + func_type->Koopa() + " {\n" +
               block->Koopa() + "}\n";
    }
};

// FuncType 也是 BaseAST
class FuncTypeAST : public BaseAST
{
public:
    std::string type;

    void Dump() const override
    {
        std::cout << "FuncTypeAST { " << type << " }";
    }
    std::string Koopa() const override
    {
        if (type == "int")
            return "i32";
        return type;
    }
};

// Block 也是 BaseAST
class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override
    {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }
    std::string Koopa() const override { return "%entry:\n" + stmt->Koopa(); }
};

// Stmt 也是 BaseAST
class StmtAST : public BaseAST
{
public:
    int number;

    void Dump() const override { std::cout << "StmtAST { " << number << " }"; }
    std::string Koopa() const override
    {
        return "ret " + std::to_string(number) + "\n";
    }
};
