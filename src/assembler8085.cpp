#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <stdexcept>

using namespace std;

struct Line {
    string label;
    string opcode;
    vector<string> operands;
    int address = 0;
    int lineNumber = 0;
    string original;
};

class Assembler8085 {
private:
    unordered_map<string, int> symbolTable;
    vector<Line> program;
    vector<unsigned char> machineCode;

    static string trim(string s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == string::npos) return "";

        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static string upper(string s) {
        transform(s.begin(), s.end(), s.begin(),
                  [](unsigned char c) { return toupper(c); });
        return s;
    }

    static string removeComment(string s) {
        size_t pos = s.find(';');
        if (pos != string::npos)
            s = s.substr(0, pos);

        return trim(s);
    }

    static vector<string> splitOperands(string s) {
        vector<string> result;
        string current;

        for (char c : s) {
            if (c == ',') {
                result.push_back(trim(current));
                current.clear();
            } else {
                current += c;
            }
        }

        if (!trim(current).empty())
            result.push_back(trim(current));

        return result;
    }

    static bool isRegister8(const string& s) {
        string x = upper(s);

        return x == "A" || x == "B" || x == "C" ||
               x == "D" || x == "E" || x == "H" ||
               x == "L" || x == "M";
    }

    static int registerCode(const string& s) {
        string x = upper(s);

        if (x == "B") return 0;
        if (x == "C") return 1;
        if (x == "D") return 2;
        if (x == "E") return 3;
        if (x == "H") return 4;
        if (x == "L") return 5;
        if (x == "M") return 6;
        if (x == "A") return 7;

        throw runtime_error("Invalid register: " + s);
    }

    static bool isRegisterPair(const string& s) {
        string x = upper(s);

        return x == "B" || x == "D" ||
               x == "H" || x == "SP" ||
               x == "PSW";
    }

    static int parseNumber(string s) {
        s = trim(upper(s));

        if (s.empty())
            throw runtime_error("Empty numeric value");

        // Hexadecimal formats:
        // 2050H
        // 0x2050
        if (s.back() == 'H') {
            s.pop_back();
            return stoi(s, nullptr, 16);
        }

        if (s.rfind("0X", 0) == 0)
            return stoi(s.substr(2), nullptr, 16);

        // Binary: 10101010B
        if (s.back() == 'B') {
            s.pop_back();
            return stoi(s, nullptr, 2);
        }

        // Decimal
        return stoi(s);
    }

    int resolveValue(const string& operand) {
        string x = upper(trim(operand));

        auto it = symbolTable.find(x);

        if (it != symbolTable.end())
            return it->second;

        return parseNumber(x);
    }

    int instructionSize(const Line& line) {
        string op = upper(line.opcode);

        // One-byte instructions
        static const vector<string> oneByte = {
            "NOP", "HLT", "RLC", "RRC", "RAL", "RAR",
            "DAA", "CMA", "STC", "CMC", "RET", "PCHL",
            "XCHG", "XTHL", "SPHL", "EI", "DI",
            "RNZ", "RZ", "RNC", "RC", "RPO", "RPE",
            "RP", "RM"
        };

        if (find(oneByte.begin(), oneByte.end(), op) != oneByte.end())
            return 1;

        // MOV r,r
        if (op == "MOV")
            return 1;

        // MVI r, data
        if (op == "MVI")
            return 2;

        // INR / DCR r
        if (op == "INR" || op == "DCR")
            return 1;

        // INX / DCX / DAD
        if (op == "INX" || op == "DCX" || op == "DAD")
            return 1;

        // ADD, ADC, SUB, SBB, ANA, XRA, ORA, CMP
        if (op == "ADD" || op == "ADC" ||
            op == "SUB" || op == "SBB" ||
            op == "ANA" || op == "XRA" ||
            op == "ORA" || op == "CMP")
            return 1;

        // LXI rp, data16
        if (op == "LXI")
            return 3;

        // Immediate 8-bit instructions
        if (op == "ADI" || op == "ACI" ||
            op == "SUI" || op == "SBI" ||
            op == "ANI" || op == "XRI" ||
            op == "ORI" || op == "CPI")
            return 2;

        // 16-bit address instructions
        if (op == "JMP" || op == "JC" || op == "JNC" ||
            op == "JZ" || op == "JNZ" ||
            op == "JP" || op == "JM" ||
            op == "JPE" || op == "JPO" ||
            op == "CALL" || op == "CC" ||
            op == "CNC" || op == "CZ" ||
            op == "CNZ" || op == "CP" ||
            op == "CM" || op == "CPE" ||
            op == "CPO" ||
            op == "LDA" || op == "STA" ||
            op == "LHLD" || op == "SHLD")
            return 3;

        // IN/OUT
        if (op == "IN" || op == "OUT")
            return 2;

        // PUSH/POP
        if (op == "PUSH" || op == "POP")
            return 1;

        // RST n
        if (op == "RST")
            return 1;

        // DB
        if (op == "DB")
            return line.operands.size();

        throw runtime_error(
            "Unknown instruction '" + line.opcode + "'"
        );
    }

    void parseSource(istream& input) {
        string raw;
        int lineNumber = 0;

        while (getline(input, raw)) {
            lineNumber++;

            string line = removeComment(raw);

            if (line.empty())
                continue;

            Line current;
            current.lineNumber = lineNumber;
            current.original = raw;

            // Check for label
            size_t colon = line.find(':');

            if (colon != string::npos) {
                current.label = upper(trim(line.substr(0, colon)));
                line = trim(line.substr(colon + 1));
            }

            if (line.empty()) {
                program.push_back(current);
                continue;
            }

            stringstream ss(line);

            ss >> current.opcode;

            current.opcode = upper(current.opcode);

            string remaining;
            getline(ss, remaining);

            remaining = trim(remaining);

            if (!remaining.empty())
                current.operands = splitOperands(remaining);

            program.push_back(current);
        }
    }

    void firstPass() {
        int locationCounter = 0;

        for (auto& line : program) {
            line.address = locationCounter;

            if (!line.label.empty()) {
                if (symbolTable.count(line.label))
                    throw runtime_error(
                        "Duplicate label: " + line.label
                    );

                symbolTable[line.label] = locationCounter;
            }

            if (!line.opcode.empty()) {
                locationCounter += instructionSize(line);
            }
        }
    }

    void emit8(int value) {
        if (value < 0 || value > 255)
            throw runtime_error("8-bit value out of range");

        machineCode.push_back(
            static_cast<unsigned char>(value)
        );
    }

    void emit16(int value) {
        if (value < 0 || value > 65535)
            throw runtime_error("16-bit value out of range");

        // 8085 uses little-endian for 16-bit addresses
        emit8(value & 0xFF);
        emit8((value >> 8) & 0xFF);
    }

    void secondPass() {
        machineCode.clear();

        for (const auto& line : program) {
            if (line.opcode.empty())
                continue;

            string op = upper(line.opcode);

            try {
                // -------------------------
                // DB
                // -------------------------
                if (op == "DB") {
                    for (const string& operand : line.operands) {
                        emit8(resolveValue(operand));
                    }
                }

                // -------------------------
                // NOP / HLT
                // -------------------------
                else if (op == "NOP") emit8(0x00);
                else if (op == "HLT") emit8(0x76);

                // -------------------------
                // MOV
                // -------------------------
                else if (op == "MOV") {
                    if (line.operands.size() != 2)
                        throw runtime_error("MOV requires 2 operands");

                    int dst = registerCode(line.operands[0]);
                    int src = registerCode(line.operands[1]);

                    if (dst == 6 && src == 6)
                        throw runtime_error("MOV M,M is invalid");

                    emit8(0x40 + dst * 8 + src);
                }

                // -------------------------
                // MVI
                // -------------------------
                else if (op == "MVI") {
                    if (line.operands.size() != 2)
                        throw runtime_error("MVI requires 2 operands");

                    int r = registerCode(line.operands[0]);
                    int value = resolveValue(line.operands[1]);

                    emit8(0x06 + r * 8);
                    emit8(value);
                }

                // -------------------------
                // INR
                // -------------------------
                else if (op == "INR") {
                    int r = registerCode(line.operands[0]);
                    emit8(0x04 + r * 8);
                }

                // -------------------------
                // DCR
                // -------------------------
                else if (op == "DCR") {
                    int r = registerCode(line.operands[0]);
                    emit8(0x05 + r * 8);
                }

                // -------------------------
                // INX
                // -------------------------
                else if (op == "INX") {
                    string rp = upper(line.operands[0]);

                    int code;

                    if (rp == "B") code = 0;
                    else if (rp == "D") code = 1;
                    else if (rp == "H") code = 2;
                    else if (rp == "SP") code = 3;
                    else throw runtime_error("Invalid register pair");

                    emit8(0x03 + code * 0x10);
                }

                // -------------------------
                // DCX
                // -------------------------
                else if (op == "DCX") {
                    string rp = upper(line.operands[0]);

                    int code;

                    if (rp == "B") code = 0;
                    else if (rp == "D") code = 1;
                    else if (rp == "H") code = 2;
                    else if (rp == "SP") code = 3;
                    else throw runtime_error("Invalid register pair");

                    emit8(0x0B + code * 0x10);
                }

                // -------------------------
                // LXI
                // -------------------------
                else if (op == "LXI") {
                    string rp = upper(line.operands[0]);
                    int value = resolveValue(line.operands[1]);

                    int code;

                    if (rp == "B") code = 0;
                    else if (rp == "D") code = 1;
                    else if (rp == "H") code = 2;
                    else if (rp == "SP") code = 3;
                    else throw runtime_error("Invalid register pair");

                    emit8(0x01 + code * 0x10);
                    emit16(value);
                }

                // -------------------------
                // DAD
                // -------------------------
                else if (op == "DAD") {
                    string rp = upper(line.operands[0]);

                    int code;

                    if (rp == "B") code = 0;
                    else if (rp == "D") code = 1;
                    else if (rp == "H") code = 2;
                    else if (rp == "SP") code = 3;
                    else throw runtime_error("Invalid register pair");

                    emit8(0x09 + code * 0x10);
                }

                // -------------------------
                // Arithmetic register ops
                // -------------------------
                else if (op == "ADD") {
                    emit8(0x80 + registerCode(line.operands[0]));
                }

                else if (op == "ADC") {
                    emit8(0x88 + registerCode(line.operands[0]));
                }

                else if (op == "SUB") {
                    emit8(0x90 + registerCode(line.operands[0]));
                }

                else if (op == "SBB") {
                    emit8(0x98 + registerCode(line.operands[0]));
                }

                else if (op == "ANA") {
                    emit8(0xA0 + registerCode(line.operands[0]));
                }

                else if (op == "XRA") {
                    emit8(0xA8 + registerCode(line.operands[0]));
                }

                else if (op == "ORA") {
                    emit8(0xB0 + registerCode(line.operands[0]));
                }

                else if (op == "CMP") {
                    emit8(0xB8 + registerCode(line.operands[0]));
                }

                // -------------------------
                // Immediate arithmetic
                // -------------------------
                else if (op == "ADI") {
                    emit8(0xC6);
                    emit8(resolveValue(line.operands[0]));
                }

                else if (op == "ACI") {
                    emit8(0xCE);
                    emit8(resolveValue(line.operands[0]));
                }

                else if (op == "SUI") {
                    emit8(0xD6);
                    emit8(resolveValue(line.operands[0]));
                }

                else if (op == "SBI") {
                    emit8(0xDE);
                    emit8(resolveValue(line.operands[0]));
                }

                else if (op == "ANI") {
                    emit8(0xE6);
                    emit8(resolveValue(line.operands[0]));
                }

                else if (op == "XRI") {
                    emit8(0xEE);
                    emit8(resolveValue(line.operands[0]));
                }

                else if (op == "ORI") {
                    emit8(0xF6);
                    emit8(resolveValue(line.operands[0]));
                }

                else if (op == "CPI") {
                    emit8(0xFE);
                    emit8(resolveValue(line.operands[0]));
                }

                // -------------------------
                // JMP
                // -------------------------
                else if (op == "JMP") {
                    emit8(0xC3);
                    emit16(resolveValue(line.operands[0]));
                }

                // -------------------------
                // Conditional jumps
                // -------------------------
                else if (op == "JNZ") {
                    emit8(0xC2);
                    emit16(resolveValue(line.operands[0]));
                }

                else if (op == "JZ") {
                    emit8(0xCA);
                    emit16(resolveValue(line.operands[0]));
                }

                else if (op == "JNC") {
                    emit8(0xD2);
                    emit16(resolveValue(line.operands[0]));
                }

                else if (op == "JC") {
                    emit8(0xDA);
                    emit16(resolveValue(line.operands[0]));
                }

                else if (op == "JPO") {
                    emit8(0xE2);
                    emit16(resolveValue(line.operands[0]));
                }

                else if (op == "JPE") {
                    emit8(0xEA);
                    emit16(resolveValue(line.operands[0]));
                }

                else if (op == "JP") {
                    emit8(0xF2);
                    emit16(resolveValue(line.operands[0]));
                }

                else if (op == "JM") {
                    emit8(0xFA);
                    emit16(resolveValue(line.operands[0]));
                }

                // -------------------------
                // CALL
                // -------------------------
                else if (op == "CALL") {
                    emit8(0xCD);
                    emit16(resolveValue(line.operands[0]));
                }

                // -------------------------
                // RET
                // -------------------------
                else if (op == "RET") {
                    emit8(0xC9);
                }

                // -------------------------
                // PUSH
                // -------------------------
                else if (op == "PUSH") {
                    string rp = upper(line.operands[0]);

                    if (rp == "B") emit8(0xC5);
                    else if (rp == "D") emit8(0xD5);
                    else if (rp == "H") emit8(0xE5);
                    else if (rp == "PSW") emit8(0xF5);
                    else throw runtime_error("Invalid PUSH pair");
                }

                // -------------------------
                // POP
                // -------------------------
                else if (op == "POP") {
                    string rp = upper(line.operands[0]);

                    if (rp == "B") emit8(0xC1);
                    else if (rp == "D") emit8(0xD1);
                    else if (rp == "H") emit8(0xE1);
                    else if (rp == "PSW") emit8(0xF1);
                    else throw runtime_error("Invalid POP pair");
                }

                // -------------------------
                // LDA
                // -------------------------
                else if (op == "LDA") {
                    emit8(0x3A);
                    emit16(resolveValue(line.operands[0]));
                }

                // -------------------------
                // STA
                // -------------------------
                else if (op == "STA") {
                    emit8(0x32);
                    emit16(resolveValue(line.operands[0]));
                }

                // -------------------------
                // LHLD
                // -------------------------
                else if (op == "LHLD") {
                    emit8(0x2A);
                    emit16(resolveValue(line.operands[0]));
                }

                // -------------------------
                // SHLD
                // -------------------------
                else if (op == "SHLD") {
                    emit8(0x22);
                    emit16(resolveValue(line.operands[0]));
                }

                // -------------------------
                // IN
                // -------------------------
                else if (op == "IN") {
                    emit8(0xDB);
                    emit8(resolveValue(line.operands[0]));
                }

                // -------------------------
                // OUT
                // -------------------------
                else if (op == "OUT") {
                    emit8(0xD3);
                    emit8(resolveValue(line.operands[0]));
                }

                // -------------------------
                // Rotates
                // -------------------------
                else if (op == "RLC") emit8(0x07);
                else if (op == "RRC") emit8(0x0F);
                else if (op == "RAL") emit8(0x17);
                else if (op == "RAR") emit8(0x1F);

                // -------------------------
                // Miscellaneous
                // -------------------------
                else if (op == "DAA") emit8(0x27);
                else if (op == "CMA") emit8(0x2F);
                else if (op == "STC") emit8(0x37);
                else if (op == "CMC") emit8(0x3F);
                else if (op == "PCHL") emit8(0xE9);
                else if (op == "XCHG") emit8(0xEB);
                else if (op == "XTHL") emit8(0xE3);
                else if (op == "SPHL") emit8(0xF9);
                else if (op == "EI") emit8(0xFB);
                else if (op == "DI") emit8(0xF3);

                // -------------------------
                // RST
                // -------------------------
                else if (op == "RST") {
                    int n = resolveValue(line.operands[0]);

                    if (n < 0 || n > 7)
                        throw runtime_error("RST must be 0-7");

                    emit8(0xC7 + n * 8);
                }

                else {
                    throw runtime_error(
                        "Instruction not implemented: " + op
                    );
                }

            } catch (const exception& e) {
                throw runtime_error(
                    "Error on line " +
                    to_string(line.lineNumber) +
                    ": " + string(e.what())
                );
            }
        }
    }

public:

    void assemble(istream& input) {
        parseSource(input);
        firstPass();
        secondPass();
    }

    void printSymbolTable() const {
        cout << "\n===== SYMBOL TABLE =====\n";

        for (const auto& [symbol, address] : symbolTable) {
            cout << left << setw(15)
                 << symbol
                 << "  "
                 << "0x"
                 << uppercase
                 << hex
                 << setw(4)
                 << setfill('0')
                 << address
                 << setfill(' ')
                 << dec
                 << "\n";
        }
    }

    void printMachineCode() const {
        cout << "\n===== MACHINE CODE =====\n";

        for (size_t i = 0; i < machineCode.size(); i++) {

            if (i % 8 == 0) {
                cout << "\n"
                     << uppercase
                     << hex
                     << setw(4)
                     << setfill('0')
                     << i
                     << ": "
                     << setfill(' ');
            }

            cout << uppercase
                 << hex
                 << setw(2)
                 << setfill('0')
                 << static_cast<int>(machineCode[i])
                 << " "
                 << setfill(' ');
        }

        cout << "\n\n";
    }

    void saveBinary(const string& filename) const {
        ofstream file(filename, ios::binary);

        if (!file)
            throw runtime_error("Cannot create output file");

        for (unsigned char byte : machineCode)
            file.write(
                reinterpret_cast<const char*>(&byte),
                1
            );

        file.close();
    }

    void saveHex(const string& filename) const {
        ofstream file(filename);

        if (!file)
            throw runtime_error("Cannot create HEX file");

        for (unsigned char byte : machineCode) {
            file << uppercase
                 << hex
                 << setw(2)
                 << setfill('0')
                 << static_cast<int>(byte)
                 << " ";
        }

        file << "\n";
        file.close();
    }
};


int main(int argc, char* argv[]) {

    try {

        if (argc < 2) {
            cout << "8085 Two-Pass Assembler\n\n";
            cout << "Usage:\n";
            cout << "  assembler8085 program.asm\n\n";
            return 1;
        }

        ifstream input(argv[1]);

        if (!input) {
            cerr << "Error: Cannot open "
                 << argv[1] << "\n";

            return 1;
        }

        Assembler8085 assembler;

        assembler.assemble(input);

        assembler.printSymbolTable();
        assembler.printMachineCode();

        assembler.saveHex("output.hex");
        assembler.saveBinary("output.bin");

        cout << "Assembly successful.\n";
        cout << "Generated files:\n";
        cout << "  output.hex\n";
        cout << "  output.bin\n";

    } catch (const exception& e) {

        cerr << "\nASSEMBLY ERROR:\n";
        cerr << e.what() << "\n";

        return 1;
    }

    return 0;
}
