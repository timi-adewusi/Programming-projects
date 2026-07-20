#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

// diagnostics

struct Diagnostic {
    int line;
    std::string message;
};

class Diagnostics {
public:
    void error(int line, std::string msg) {items_.push_back({line, std::move(msg)}); }
    
    bool hasErrors() const { return !items_.empty(); }

    void report(std::ostream& os) const {
        for (const auto& d : items_)
            os << "error: line " << d.line << ": " << d.message << "\n";
    }

private:
    std::vector<Diagnostic> items_;
};

//Lexer

enum class Tok {
    Ident,
    Number,
    Register,
    String,
    Char,

    Keyword,

    At,
    Colon,
    LBracket,
    RBracket,
    Dot,
    Comma,

    Newline,
    End
};

struct Token {
    Tok type;
    std::string text;
    int64_t num = 0;
    int line = 0;
    int column = 0;
};

class Lexer {
public:
    Lexer(const std::string& src, Diagnostics& diag) : s_(src), diag_(diag) {}

    std::vector<Token> tokenize() {
        std::vector<Token> out;
        for(;;) {
            Token t = next();
            out.push_back(t);
            if(t.type == Tok::End) break;
        }
        return out;
    }
private:
    const std::string& s_;
    Diagnostics& diag_;

    size_t pos_ = 0;
    int line_ = 1;

    int peek(int off = 0) const {
        size_t p = pos_ + off;
        return p < s_.size() ? (unsigned char)s_[p] : -1;
    }
    int advance() {
        int c = peek();
        pos_++;
        
        if (c == '\n') line_++;
        return c;
    }
    static bool identStart(int c) { return std::isalpha(c) || c == '_' || c == '$'; }
    static bool identChar(int c) { return std::isalnum(c) || c == '_' || c == '$'; }

    Token next() {
        for (;;) {
            int c = peek();
            if (c == -1) return {Tok::End, "", 0, line_};
            if (c == '\n') { advance(); return {Tok::Newline, "", 0, line_ - 1}; }
            if (c == ' ' || c == '\t' || c == '\r') { advance(); continue; }
            if (c == ';') { while (peek() != -1 && peek() != '\n') advance(); continue; }
            break;
        }
        int startLine = line_;
        int c = peek();

        if (c == '[')
        {
            advance();
            return {Tok::LBracket, "[", 0, startLine};
        }

        if (c == ']')
        {
            advance();
            return {Tok::RBracket, "]", 0, startLine};
        }

        if (c == '.')
        {
            advance();
            return {Tok::Dot, ".", 0, startLine};
        }

        if (c == ',')
        {
            advance();
            return {Tok::Comma, ",", 0, startLine};
        }
        if (c == '@') {
            advance();
            std::string name;
            while (identChar(peek())) name += (char)advance();
            return {Tok::At, name, 0, startLine};
        }
        if (c == '\'')
        {
            advance();

            int value = advance();

            if (peek() == '\'')
                advance();
            else
                diag_.error(startLine, "unterminated character literal");

            return {Tok::Char, "", value, startLine};
        }
        if (c == '"') {
            advance();
            std::string val;
            while (peek() != -1 && peek() != '"') {
                int ch = advance();
                if (ch == '\\' && peek() == '"') { val += '"'; advance(); }
                else val += (char)ch;
            }
            if (peek() == '"') advance();
            else diag_.error(startLine, "unterminated string literal");
            return {Tok::String, val, 0, startLine};
        }
        if ((c == '-' && std::isdigit(peek(1))) ||
            std::isdigit(c))
        {
            std::string num;

            if (c == '-')
                num += (char)advance();

            if (peek() == '0' && peek(1) == 'x')
            {
                num += (char)advance();
                num += (char)advance();

                while (std::isxdigit(peek()))
                    num += (char)advance();

                return {Tok::Number, num, std::stoll(num.substr(2), nullptr, 16), startLine};
            }

            while (std::isdigit(peek()))
                num += (char)advance();

            return {Tok::Number, num, std::stoll(num), startLine};
        }
        if (identStart(c)) {
            std::string id;
            while (identChar(peek())) id += (char)advance();
            if (peek() == ':') {
                advance();
                return {Tok::Colon, id, 0, startLine};
            }
            if (id == "SP" ||
                id == "BP" ||
                id == "IP" ||
                id == "FLAGS")
            {
                return {Tok::Register, id, 0, startLine};
            }
            if (id.size() > 1 && id[0] == 'R' &&
                std::all_of(id.begin() + 1, id.end(), [](unsigned char ch) { return std::isdigit(ch); })) {
                int idx = std::stoi(id.substr(1));
                if (idx >= 0 && idx <= 15) return {Tok::Register, id, idx, startLine};
            }
            return {Tok::Ident, id, 0, startLine};
        }
        advance();
        diag_.error(startLine, std::string("unexpected character '") + (char)c + "'");
        return next();
    }
};

// Instruction set

enum Opcode : uint8_t {
    HALT = 0x00,
    MOV = 0x10, MOVR = 0x11,
    ADD = 0x20, SUB = 0x21, MUL = 0x22, DIV = 0x23, MOD = 0x24, NEG = 0x25,
    AND = 0x30, OR = 0x31, XOR = 0x32, NOT = 0x33, SHL = 0x34, SHR = 0x35,
    EQ = 0x40, NEQ = 0x41, LT = 0x42, GT = 0x43, LTE = 0x44, GTE = 0x45,
    LOAD8 = 0x50, LOAD16 = 0x51, LOAD32 = 0x52,
    STORE8 = 0x53, STORE16 = 0x54, STORE32 = 0x55,
    JMP = 0x60, JZ = 0x61, JNZ = 0x62, CALL = 0x63, RET = 0x64,
    ALLOCLOCAL = 0x70, LOADLOCAL = 0x71, STORELOCAL = 0x72,
    PUSH = 0x80, POP = 0x81,
    PRINT = 0x90, EMIT = 0x91, READ = 0x92, PRINTSTR = 0x93,
};

enum class Pattern { NONE, IMM, REG, REG_IMM, REG_REG, REG_REG_REG, REG_ANY, REG_MEM,MEM_REG };

struct InstrInfo { Opcode op; Pattern pattern; };

static const std::unordered_map<std::string, InstrInfo> INSTR_TABLE = {
    {"HALT", {HALT, Pattern::NONE}},
    {"MOV", {MOV, Pattern::REG_IMM}}, {"MOVR", {MOVR, Pattern::REG_REG}},
    {"ADD", {ADD, Pattern::REG_REG_REG}}, {"SUB", {SUB, Pattern::REG_REG_REG}},
    {"MUL", {MUL, Pattern::REG_REG_REG}}, {"DIV", {DIV, Pattern::REG_REG_REG}},
    {"MOD", {MOD, Pattern::REG_REG_REG}}, {"NEG", {NEG, Pattern::REG_REG}},
    {"AND", {AND, Pattern::REG_REG_REG}}, {"OR", {OR, Pattern::REG_REG_REG}},
    {"XOR", {XOR, Pattern::REG_REG_REG}}, {"NOT", {NOT, Pattern::REG_REG}},
    {"SHL", {SHL, Pattern::REG_REG_REG}}, {"SHR", {SHR, Pattern::REG_REG_REG}},
    {"EQ", {EQ, Pattern::REG_REG_REG}}, {"NEQ", {NEQ, Pattern::REG_REG_REG}},
    {"LT", {LT, Pattern::REG_REG_REG}}, {"GT", {GT, Pattern::REG_REG_REG}},
    {"LTE", {LTE, Pattern::REG_REG_REG}}, {"GTE", {GTE, Pattern::REG_REG_REG}},
    {"LOAD8", {LOAD8, Pattern::REG_REG}}, {"LOAD16", {LOAD16, Pattern::REG_REG}},
    {"LOAD32", {LOAD32, Pattern::REG_REG}},
    {"STORE8", {STORE8, Pattern::REG_REG}}, {"STORE16", {STORE16, Pattern::REG_REG}},
    {"STORE32", {STORE32, Pattern::REG_REG}},
    {"JMP", {JMP, Pattern::IMM}}, {"JZ", {JZ, Pattern::REG_IMM}},
    {"JNZ", {JNZ, Pattern::REG_IMM}}, {"CALL", {CALL, Pattern::IMM}},
    {"RET", {RET, Pattern::NONE}},
    {"PUSH", {PUSH, Pattern::REG}}, {"POP", {POP, Pattern::REG}},
    {"PRINT", {PRINT, Pattern::REG}}, {"EMIT", {EMIT, Pattern::REG}},
    {"READ", {READ, Pattern::REG}}, {"PRINTSTR", {PRINTSTR, Pattern::REG}},
};

static int operandBytes(Pattern p) {
    switch (p) {

        case Pattern::NONE:
            return 0;

        // Immediate value
        // Example:
        // JMP 100
        case Pattern::IMM:
            return 4;


        // Single register
        // Example:
        // PUSH R1
        case Pattern::REG:
            return 1;


        // Register + immediate
        // Example:
        // MOV R1 100
        case Pattern::REG_IMM:
            return 5; 
            // R1 (1 byte) + integer (4 bytes)


        // Register + register
        // Example:
        // MOV R1 R2
        case Pattern::REG_REG:
            return 2;


        // Three registers
        // Example:
        // ADD R1 R2 R3
        case Pattern::REG_REG_REG:
            return 3;


        // Variable size
        // Example:
        // MOV R1 10
        // MOV R1 R2
        //
        // Cannot know until operands are parsed.
        case Pattern::REG_ANY:
            return -1;


        // Register + memory address
        // Example:
        // LOAD32 R1 [R2]
        //
        // Register = 1 byte
        // Address = 4 bytes
        case Pattern::REG_MEM:
            return 5;


        // Memory address + register
        // Example:
        // STORE32 [R1] R2
        //
        // Address = 4 bytes
        // Register = 1 byte
        case Pattern::MEM_REG:
            return 5;
    }

    return 0;
}

static int instrSize(Pattern p) { return 1 + operandBytes(p); }

// Unified symbol table

enum class SymKind { Label, Data };
struct Symbol { SymKind kind; int32_t address; std::string ns; };

class SymbolTable {
public:
    bool define(const std::string& name, SymKind kind, int32_t addr, Diagnostics& diag, int line,
                const std::string& ns = "") {
        if (table_.count(name)) {
            diag.error(line, "symbol '" + name + "' redefined");
            return false;
        }
        table_[name] = {kind, addr, ns};
        return true;
    }
    std::optional<Symbol> lookup(const std::string& name) const {
        auto it = table_.find(name);
        if (it == table_.end()) return std::nullopt;
        return it->second;
    }

private:
    std::unordered_map<std::string, Symbol> table_;
};

// Macro processor
using TokLine = std::vector<Token>;

static std::vector<TokLine> splitLines(const std::vector<Token>& toks) {
    std::vector<TokLine> lines;
    TokLine cur;

    for (const auto& t : toks) {
        if (t.type == Tok::Newline || t.type == Tok::End) {
            if (!cur.empty())
                lines.push_back(cur);

            cur.clear();

            if (t.type == Tok::End)
                break;
        }
        else {
            cur.push_back(t);
        }
    }

    return lines;
}

struct MacroDef {
    std::vector<std::string> params;
    std::vector<TokLine> body;
};

class MacroExpander {
public:
    explicit MacroExpander(Diagnostics& diag)
        : diag_(diag) {}

    std::vector<TokLine> expand(std::vector<TokLine> lines) {
        for (int pass = 0; pass < MAX_EXPANSION_PASSES; pass++) {
            bool sawCall = false;

            lines = expandOnce(lines, sawCall);

            if (!sawCall)
                break;
        }

        return lines;
    }
private:

    static constexpr int MAX_EXPANSION_PASSES = 32;

    Diagnostics& diag_;

    std::unordered_map<std::string, MacroDef> registry_;

    uint64_t expansionId_ = 0;

    std::vector<TokLine> expandOnce(
        const std::vector<TokLine>& lines,
        bool& sawCall)
    {
        std::vector<TokLine> out;

        for (size_t i = 0; i < lines.size(); i++) {

            const TokLine& line = lines[i];

            if (line.empty())
                continue;

            if (line[0].type == Tok::Ident &&
                line[0].text == "MACRO")
            {
                if (line.size() < 2) {
                    diag_.error(
                        line[0].line,
                        "MACRO requires a name"
                    );
                    continue;
                }

                MacroDef def;
                std::string name = line[1].text;

                for (size_t k = 2; k < line.size(); k++)
                    def.params.push_back(line[k].text);
                i++;

                while (i < lines.size()) {

                    if (!lines[i].empty() &&
                        lines[i][0].type == Tok::Ident &&
                        lines[i][0].text == "ENDM")
                    {
                        break;
                    }


                    def.body.push_back(lines[i]);
                    i++;
                }
                registry_[name] = std::move(def);

                continue;
            }
            if (line[0].type == Tok::At)
            {
                const std::string& macroName = line[0].text;


                auto it = registry_.find(macroName);


                if (it == registry_.end()) {

                    diag_.error(
                        line[0].line,
                        "undefined macro '" + macroName + "'"
                    );

                    continue;
                }

                const MacroDef& def = it->second;

                size_t argCount = line.size() - 1;

                if (argCount != def.params.size()) {

                    diag_.error(
                        line[0].line,
                        "macro '" + macroName +
                        "' expects " +
                        std::to_string(def.params.size()) +
                        " args, got " +
                        std::to_string(argCount)
                    );

                    continue;
                }

                std::unordered_map<std::string, Token> bind;

                for (size_t k = 0; k < def.params.size(); k++) {

                    bind["$" + def.params[k]]
                        = line[k + 1];
                }

                uint64_t id = expansionId_++;

                for (const TokLine& bodyLine : def.body)
                {
                    TokLine substituted;

                    for (const Token& t : bodyLine)
                    {
                        Token result = t;

                        // Parameter substitution
                        if (t.type == Tok::Ident)
                        {
                            auto bit = bind.find(t.text);

                            if (bit != bind.end())
                            {
                                result = bit->second;
                            }
                        }
                        if (result.type == Tok::Ident &&
                            result.text.size() > 2 &&
                            result.text[0] == '%' &&
                            result.text[1] == '%')
                        {
                            result.text =
                                "__macro_" +
                                std::to_string(id) +
                                "_" +
                                result.text.substr(2);
                        }
                        substituted.push_back(result);
                    }
                    out.push_back(std::move(substituted));
                }
                sawCall = true;

                continue;
            }
            out.push_back(line);
        }
        return out;
    }
};

// 6. PARSER -> IR  (with inline DATA-segment extraction)

struct Operand {
    enum Kind {
        Reg,
        Imm,
        Sym,
        Mem
    } kind = Imm;

    int32_t regIndex = 0;
    int64_t immValue = 0;
    std::string symName;

    int32_t memReg = 0;

    int line = 0;
};

struct IRItem {
    bool isLabel = false;

    std::string label;
    std::string mnemonic;

    Opcode op = HALT;
    Pattern pattern = Pattern::NONE;

    std::vector<Operand> operands;

    int line = 0;
    int32_t address = -1;
};

class Parser {
public:
    Parser(Diagnostics& diag, SymbolTable& syms, std::vector<uint8_t>& dataBlob, int32_t& dataPtr,
           std::string ns)
        : diag_(diag), syms_(syms), blob_(dataBlob), dataPtr_(dataPtr), ns_(std::move(ns)) {}

    std::vector<IRItem> parse(const std::vector<TokLine>& lines) {
        std::vector<IRItem> ir;
        for (const TokLine& line : lines) {
            if (line.empty()) continue;
            const Token& head = line[0];

            if (head.type == Tok::Colon) {
                IRItem item;
                item.isLabel = true;
                item.label = head.text;
                item.line = head.line;
                ir.push_back(std::move(item));
                continue;
            }
            if (head.type == Tok::Ident && head.text == "DATA") {
                parseData(line);
                continue;
            }
            if (head.type != Tok::Ident) {
                diag_.error(head.line, "expected an instruction mnemonic");
                continue;
            }
            auto it = INSTR_TABLE.find(head.text);
            if (it == INSTR_TABLE.end()) {
                diag_.error(head.line, "unknown instruction '" + head.text + "'");
                continue;
            }
            IRItem item;
            item.isLabel = false;
            item.op = it->second.op;
            item.pattern = it->second.pattern;
            item.mnemonic = head.text;
            item.line = head.line;
            if (parseOperands(line, item)) ir.push_back(std::move(item));
        }
        return ir;
    }

private:
    Diagnostics& diag_;
    SymbolTable& syms_;
    std::vector<uint8_t>& blob_;
    int32_t& dataPtr_;
    std::string ns_;

    void parseData(const TokLine& line) {
        if (line.size() < 3 || line[1].type != Tok::Ident || line[2].type != Tok::String) {
            diag_.error(line[0].line, "DATA requires a name and a quoted string");
 return;
        }
        const std::string& name = line[1].text;
        const std::string& text = line[2].text;
        if (!syms_.define(name, SymKind::Data, dataPtr_, diag_, line[0].line, ns_)) return;
        for (char c : text) blob_.push_back((uint8_t)c);
        blob_.push_back(0);
        dataPtr_ += (int32_t)(text.size() + 1);
    }

    bool expectOperand(const TokLine& line, size_t idx, bool wantReg, Operand& out) {
        if (idx >= line.size()) {
            diag_.error(line[0].line, "missing operand");
            return false;
        }
        const Token& t = line[idx];
        out.line = t.line;
        if (wantReg) {
            if (t.type != Tok::Register) { diag_.error(t.line, "expected a register operand"); return false; }
            out.kind = Operand::Reg;
            out.regIndex = (int32_t)t.num;
            return true;
        }
        if (t.type == Tok::Number) { out.kind = Operand::Imm; out.immValue = t.num; return true; }
        if (t.type == Tok::Ident) { out.kind = Operand::Sym; out.symName = t.text; return true; }
        diag_.error(t.line, "expected a number or a symbol name");
        return false;
    }

    bool parseOperands(const TokLine& line, IRItem& item) {
        size_t idx = 1;
        auto need = [&](bool wantReg) -> bool {
            Operand op;
            bool ok = expectOperand(line, idx, wantReg, op);
            idx++;
            item.operands.push_back(op);
            return ok;
        };
        bool ok = true;
        switch (item.pattern) {
            case Pattern::NONE:
                break;

            case Pattern::IMM:
                ok = need(false);
                break;

            case Pattern::REG:
                ok = need(true);
                break;

            case Pattern::REG_IMM:
            {
                bool a = need(true);
                bool b = need(false);
                ok = a && b;
                break;
            }

            case Pattern::REG_REG:
            {
                bool a = need(true);
                bool b = need(true);
                ok = a && b;
                break;
            }

            case Pattern::REG_REG_REG:
            {
                bool a = need(true);
                bool b = need(true);
                bool c = need(true);
                ok = a && b && c;
                break;
            }

            case Pattern::REG_MEM:
            {
                bool a = need(true);
                bool b = need(false);
                ok = a && b;
                break;
            }

            case Pattern::MEM_REG:
            {
                bool a = need(false);
                bool b = need(true);
                ok = a && b;
                break;
            }

            case Pattern::REG_ANY:
            {
                bool a = need(true);
                bool b = need(false);
                ok = a;
                break;
            }
        }
        if (idx < line.size()) diag_.error(line[0].line, "too many operands");
        return ok;
    }
};

// Peephole Optimizer

class PeepholeOptimizer {
public:
    std::vector<IRItem> optimize(std::vector<IRItem> ir) {

        for (int iteration = 0; iteration < 10; iteration++) {

            bool changed = false;

            ir = pass(ir, changed);

            if (!changed)
                break;
        }
        return ir;
    }
private:
    std::vector<IRItem> pass(
        const std::vector<IRItem>& ir,
        bool& changed)
    {
        std::vector<IRItem> out;


        for (size_t i = 0; i < ir.size(); i++) {

            const IRItem& cur = ir[i];

            if (!cur.isLabel &&
                (cur.op == MOVR || cur.op == MOV) &&
                cur.operands.size() == 2 &&
                cur.operands[0].kind == Operand::Reg &&
                cur.operands[1].kind == Operand::Reg &&
                cur.operands[0].regIndex ==
                cur.operands[1].regIndex)
            {
                changed = true;
                continue;
            }

            if (i + 1 < ir.size() &&
                !cur.isLabel &&
                !ir[i + 1].isLabel &&
                cur.op == PUSH &&
                ir[i + 1].op == POP &&
                cur.operands.size() == 1 &&
                ir[i + 1].operands.size() == 1 &&
                cur.operands[0].regIndex ==
                ir[i + 1].operands[0].regIndex)
            {
                changed = true;
                i++;
                continue;
            }

            if (i + 1 < ir.size() &&
                !cur.isLabel &&
                !ir[i + 1].isLabel &&
                cur.op == PUSH &&
                ir[i + 1].op == POP &&
                cur.operands.size() == 1 &&
                ir[i + 1].operands.size() == 1)
            {
                IRItem item;

                item.isLabel = false;
                item.op = MOVR;
                item.line = cur.line;

                Operand dst;
                dst.kind = Operand::Reg;
                dst.regIndex =
                    ir[i + 1].operands[0].regIndex;

                Operand src;
                src.kind = Operand::Reg;
                src.regIndex =
                    cur.operands[0].regIndex;

                item.operands.push_back(dst);
                item.operands.push_back(src);

                out.push_back(std::move(item));

                changed = true;
                i++;

                continue;
            }

            if (!cur.isLabel &&
                cur.op == JMP &&
                cur.operands.size() == 1 &&
                cur.operands[0].kind == Operand::Sym &&
                i + 1 < ir.size() &&
                ir[i + 1].isLabel &&
                ir[i + 1].label ==
                cur.operands[0].symName)
            {
                changed = true;
                continue;
            }

            if (!cur.isLabel &&
                cur.op == JMP &&
                cur.operands.size() == 1 &&
                cur.operands[0].kind == Operand::Sym)
            {
                for (size_t j = i + 1; j < ir.size(); j++) {

                    if (ir[j].isLabel &&
                        ir[j].label ==
                        cur.operands[0].symName)
                    {
                        if (j + 1 < ir.size() &&
                            !ir[j + 1].isLabel &&
                            ir[j + 1].op == JMP &&
                            ir[j + 1].operands.size() == 1 &&
                            ir[j + 1].operands[0].kind ==
                            Operand::Sym)
                        {
                            IRItem item = cur;

                            item.operands[0].symName =
                                ir[j + 1].operands[0].symName;

                            out.push_back(std::move(item));

                            changed = true;

                            goto optimized;
                        }

                        break;
                    }
                }
            }

        optimized:

            out.push_back(cur);
        }

        return out;
    }
};

// Assembler
class Assembler {
public:

    Assembler(
        Diagnostics& diag,
        SymbolTable& syms,
        std::string ns)
        :
        diag_(diag),
        syms_(syms),
        ns_(std::move(ns))
    {}

    void assign(std::vector<IRItem>& ir)
    {
        int32_t addr = 0;

        for(IRItem& item : ir)
        {
            if(item.isLabel)
            {
                syms_.define(
                    item.label,
                    SymKind::Label,
                    addr,
                    diag_,
                    item.line,
                    ns_);

                continue;
            }
            item.address = addr;

            addr += instrSize(item);
        }
        textSize_ = addr;
    }
    int32_t textSize() const
    {
        return textSize_;
    }

private:
    Diagnostics& diag_;
    SymbolTable& syms_;

    std::string ns_;

    int32_t textSize_ = 0;

    int32_t instrSize(const IRItem& item)
    {
        int32_t size = 1; // opcode
        for(const Operand& op : item.operands)
        {
            switch(op.kind)
            {
                case Operand::Reg:
                    size += 1;
                    break;

                case Operand::Imm:
                case Operand::Sym:
                    size += 4;
                    break;

                case Operand::Mem:
                    size += 4;
                    break;
            }
        }
        return size;
    }
};

// Native Code generation
class NativeGen {
public:

    NativeGen(
        const SymbolTable& syms,
        const std::vector<uint8_t>& dataBlob,
        int32_t dataBase,
        const std::string& ns,
        Diagnostics& diag)
        :
        syms_(syms),
        blob_(dataBlob),
        dataBase_(dataBase),
        ns_(ns),
        diag_(diag)
    {}


    void emitData(std::ostream& out) const
    {
        out << "static uint8_t data[] = {";

        if (blob_.empty()) {
            out << "0";
        }
        else {
            for (size_t i = 0; i < blob_.size(); i++)
            {
                if (i % 16 == 0)
                    out << "\n    ";

                out << (int)blob_[i] << ",";
            }
        }

        out << "\n};\n";
    }
    void emitBody(
        std::ostream& out,
        const std::vector<IRItem>& ir) const
    {
        for (const IRItem& item : ir)
        {
            if (item.isLabel)
            {
                out << "L_"
                    << makeLabel(item.label)
                    << ":\n";

                continue;
            }
            emitInstr(out, item);
        }
    }

private:

    const SymbolTable& syms_;
    const std::vector<uint8_t>& blob_;

    int32_t dataBase_;

    const std::string& ns_;

    Diagnostics& diag_;

    std::string makeLabel(const std::string& name) const
    {
        return ns_ + "_" + name;
    }

    std::string regStr(const Operand& op) const
    {
        return "R[" +
            std::to_string(op.regIndex) +
            "]";
    }

    std::string operandValue(
        const Operand& op,
        int line) const
    {
        switch (op.kind)
        {

            case Operand::Imm:
                return std::to_string(op.immValue);

            case Operand::Sym:
            {
                auto s = syms_.lookup(op.symName);

                if (!s)
                {
                    diag_.error(
                        line,
                        "undefined symbol '" +
                        op.symName +
                        "'"
                    );

                    return "0";
                }

                if (s->kind == SymKind::Data)
                {
                    return std::to_string(
                        s->address
                    );
                }

                return "&&L_" +
                    s->ns +
                    "_" +
                    op.symName;
            }

            case Operand::Reg:
                return regStr(op);

            case Operand::Mem:
            {
                return "data[" +
                    std::to_string(op.memReg) +
                    "]";
            }
        }
        return "0";
    }

    std::string memoryAddress(
        const Operand& op) const
    {
        if (op.kind == Operand::Mem)
        {
            return "data[" +
                std::to_string(op.memReg) +
                "]";
        }

        return regStr(op);
    }
    void emitInstr(
        std::ostream& out,
        const IRItem& item) const
    {
        const auto& a = item.operands;

switch (item.op) {
            case HALT: out << "    goto L_end;\n"; break;
            case MOV: out << "    " << regStr(a[0]) << " = " << operandValue(a[1], item.line) << ";\n"; break;
            case MOVR: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << ";\n"; break;
            case ADD: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " + " << regStr(a[2]) << ";\n"; break;
            case SUB: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " - " << regStr(a[2]) << ";\n"; break;
            case MUL: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " * " << regStr(a[2]) << ";\n"; break;
            case DIV: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " / " << regStr(a[2]) << ";\n"; break;
            case MOD: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " % " << regStr(a[2]) << ";\n"; break;
            case NEG: out << "    " << regStr(a[0]) << " = -" << regStr(a[1]) << ";\n"; break;
            case AND: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " & " << regStr(a[2]) << ";\n"; break;
            case OR:  out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " | " << regStr(a[2]) << ";\n"; break;
            case XOR: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " ^ " << regStr(a[2]) << ";\n"; break;
            case NOT: out << "    " << regStr(a[0]) << " = ~" << regStr(a[1]) << ";\n"; break;
            case SHL: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " << " << regStr(a[2]) << ";\n"; break;
            case SHR: out << "    " << regStr(a[0]) << " = " << regStr(a[1]) << " >> " << regStr(a[2]) << ";\n"; break;
            case EQ:  out << "    " << regStr(a[0]) << " = (" << regStr(a[1]) << " == " << regStr(a[2]) << ");\n"; break;
            case NEQ: out << "    " << regStr(a[0]) << " = (" << regStr(a[1]) << " != " << regStr(a[2]) << ");\n"; break;
            case LT:  out << "    " << regStr(a[0]) << " = (" << regStr(a[1]) << " < " << regStr(a[2]) << ");\n"; break;
            case GT:  out << "    " << regStr(a[0]) << " = (" << regStr(a[1]) << " > " << regStr(a[2]) << ");\n"; break;
            case LTE: out << "    " << regStr(a[0]) << " = (" << regStr(a[1]) << " <= " << regStr(a[2]) << ");\n"; break;
            case GTE: out << "    " << regStr(a[0]) << " = (" << regStr(a[1]) << " >= " << regStr(a[2]) << ");\n"; break;
            case LOAD8:  out << "    " << regStr(a[0]) << " = (int8_t)data[" << regStr(a[1]) << "];\n"; break;
            case LOAD16: out << "    " << regStr(a[0]) << " = (int16_t)*(int16_t*)(data + " << regStr(a[1]) << ");\n"; break;
            case LOAD32: out << "    " << regStr(a[0]) << " = (int32_t)*(int32_t*)(data + " << regStr(a[1]) << ");\n"; break;
            case STORE8:  out << "    data[" << regStr(a[0]) << "] = (uint8_t)" << regStr(a[1]) << ";\n"; break;
            case STORE16: out << "    *(int16_t*)(data + " << regStr(a[0]) << ") = (int16_t)" << regStr(a[1]) << ";\n"; break;
            case STORE32: out << "    *(int32_t*)(data + " << regStr(a[0]) << ") = (int32_t)" << regStr(a[1]) << ";\n"; break;
            case JMP: out << "    goto L_" << resolveLabel(a[0].symName, item.line) << ";\n"; break;
            case JZ:  out << "    if (" << regStr(a[0]) << " == 0) goto L_" << resolveLabel(a[1].symName, item.line) << ";\n"; break;
            case JNZ: out << "    if (" << regStr(a[0]) << " != 0) goto L_" << resolveLabel(a[1].symName, item.line) << ";\n"; break;
            case CALL: {
                std::string ret = ns_ + "_ret_" + std::to_string(item.address);
                out << "    stack[sp++] = (int64_t)&&L_" << ret << "; goto L_" << resolveLabel(a[0].symName, item.line) << ";\n";
                out << "  L_" << ret << ": ;\n";
                break;
            }
            case RET: {
                if (a.empty() || a[0].kind == Operand::Imm) {
                    out << "    { int64_t _r = " << (a.empty() ? "0" : operandValue(a[1], item.line)) << "; sp--; goto *(void*)stack[sp]; }\n";
                } else {
                    out << "    sp--; goto *(void*)stack[sp];\n";
                }
                break;
            }
            case PUSH: out << "    stack[sp++] = " << regStr(a[0]) << ";\n"; break;
            case POP:  out << "    " << regStr(a[0]) << " = stack[--sp];\n"; break;
            case PRINT:    out << "    printf(\"%lld\\n\", (long long)" << regStr(a[0]) << ");\n"; break;
            case EMIT:     out << "    putchar((int)" << regStr(a[0]) << ");\n"; break;
            case READ:     out << "    " << regStr(a[0]) << " = readnum();\n"; break;
            case PRINTSTR: out << "    printf(\"%s\\n\", (char*)(data + " << regStr(a[0]) << "));\n"; break;
        }
    }

    std::string resolveLabel(
        const std::string& name,
        int line) const
    {
        auto s = syms_.lookup(name);

        if (s)
        {
            return s->ns + "_" + name;
        }

        diag_.error(
            line,
            "undefined symbol '" + name + "'"
        );
        return ns_ + "_" + name;
    }
};

// Pipeline glue and entry point

struct CompiledUnit {
    std::string ns;
    std::vector<uint8_t> dataBlob;
    std::vector<IRItem> ir;
};

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static CompiledUnit compileFile(const std::string& path, const std::string& ns,
                                SymbolTable& globalSyms, Diagnostics& globalDiag,
                                std::vector<uint8_t>& globalData, int32_t& globalDataPtr) {
    std::string src = readFile(path);

    Lexer lex(src, globalDiag);
    auto tokens = lex.tokenize();
    auto lines = splitLines(tokens);

    MacroExpander macros(globalDiag);
    lines = macros.expand(std::move(lines));

    Parser parser(globalDiag, globalSyms, globalData, globalDataPtr, ns);
    auto ir = parser.parse(lines);

    PeepholeOptimizer peep;
    ir = peep.optimize(std::move(ir));

    Assembler asm_(globalDiag, globalSyms, ns);
    asm_.assign(ir);

    if (globalDiag.hasErrors()) {
        std::ostringstream oss;
        globalDiag.report(oss);
        throw std::runtime_error("compilation failed for " + path + ":\n" + oss.str());
    }

    return {ns, {}, std::move(ir)};
}

int main(int argc, char** argv) {
    std::vector<std::string> files;
    std::string outExe = "lehout";
    bool keepC = false;
    bool verbose = false;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-o") { outExe = argv[++i]; }
        else if (a == "-k") { keepC = true; }
        else if (a == "-v") { verbose = true; }
        else if (a == "--help" || a == "-h") {
            std::cout << "usage: lehin [-o exe] [-k] [-v] file.leh [file.leh ...]\n"
                         "  -o exe  output executable name (default: lehout)\n"
                         "  -k      keep intermediate .c file\n"
                         "  -v      verbose (print generated C)\n";
            return 0;
        } else {
            files.push_back(a);
        }
    }

    if (files.empty()) {
        std::cerr << "no input files. usage: lehin file.leh [...]\n";
        return 1;
    }

    try {
        // Shared global symbol table so cross-file CALL/JMP resolve.
        SymbolTable globalSyms;
        Diagnostics globalDiag;
        std::vector<uint8_t> globalData;
        int32_t globalDataPtr = 0;

        std::vector<CompiledUnit> units;
        for (size_t i = 0; i < files.size(); i++) {
            std::string ns = "ns" + std::to_string(i);
            units.push_back(compileFile(files[i], ns, globalSyms, globalDiag, globalData, globalDataPtr));
        }
        if (globalDiag.hasErrors()) {
            std::ostringstream oss;
            globalDiag.report(oss);
            throw std::runtime_error("compilation failed:\n" + oss.str());
        }

        std::ostringstream combined;
        combined << "// Combined LEHIN native output\n"
                    "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n"
                    "// Enable computed goto (GCC extension).\n"
                    "static int64_t R[16];\n"
                    "static int64_t stack[1 << 20];\n"
                    "static int64_t sp = 0;\n"
                    "static int64_t locals[1 << 20];\n"
                    "static int64_t lcnt = 0;\n"
                    "static int64_t readnum(void) {\n"
                    "    long long v = 0; if (scanf(\"%lld\", &v) != 1) v = 0; return (int64_t)v;\n"
                    "}\n\n";

        // Emit the single global data array.
        if (!globalData.empty()) {
            combined << "static uint8_t data[] = {";
            for (size_t i = 0; i < globalData.size(); i++) {
                if (i % 16 == 0) combined << "\n    ";
                combined << (int)globalData[i] << ",";
            }
            combined << "\n};\n\n";
        } else {
            combined << "static uint8_t data[1];\n\n";
        }

        // Single combined run function: all units' code with namespaced labels.
        combined << "static void run(void) {\n";
        // Jump to the entry point of the first file (its 'start' label).
        combined << "  goto L_" << units[0].ns << "_start;\n";
        for (const auto& u : units) {
            combined << "  // === " << u.ns << " ===\n";
            NativeGen gen(globalSyms, globalData, 0, u.ns, globalDiag);
            gen.emitBody(combined, u.ir);
        }
        combined << "  L_end: ;\n"
                    "}\n\n"
                    "int main(void) { run(); return 0; }\n";

        // Report any undefined-symbol errors discovered during code generation.
        if (globalDiag.hasErrors()) {
            std::ostringstream oss;
            globalDiag.report(oss);
            throw std::runtime_error("compilation failed:\n" + oss.str());
        }

        std::string cCode = combined.str();
        if (verbose) std::cout << cCode;

        std::string cFile = outExe + ".c";
        {
            std::ofstream f(cFile);
            f << cCode;
        }

        std::string cmd = "gcc -O2 -std=gnu11 -w -o " + outExe + " " + cFile;
        if (verbose) std::cout << "$ " << cmd << "\n";
        int rc = std::system(cmd.c_str());
        if (rc != 0) throw std::runtime_error("gcc failed (exit " + std::to_string(rc) + ")");

        if (!keepC) std::remove(cFile.c_str());
        std::cout << "compiled " << files.size() << " file(s) -> " << outExe << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "lehin: " << e.what() << "\n";
        return 1;
    }
}
