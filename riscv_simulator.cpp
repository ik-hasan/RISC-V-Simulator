#include <bits/stdc++.h>
#include <string>
using namespace std;

struct Decoded {
    string op;                 // mnemonic
    int rd = -1, rs1 = -1, rs2 = -1;
    int imm = 0;               // for I/S/B types
    string label;              // for B-type target (before patch)
    int labelTarget = -1;     // resolved instruction index
    string raw;                // original line for pretty logs
};
// === Utilities ===
static inline bool isRegister(const string& t) {//t is register like x7
    return t.size() >= 2 && (t[0] == 'x' || t[0] == 'X');
}

static inline int regIndex(const string& t) {//t is register like X10
    // expects like "x5"
    if (!isRegister(t)) return -1;
    int v = stoi(t.substr(1)); // x7 ko 7 bana diya
    if (v < 0 || v > 31) return -1;
    return v;
}

static inline string trim(string s) {
    // remove comments
    auto p = s.find('#');
    if (p != string::npos) s = s.substr(0, p);
    // trim spaces
    auto l = s.find_first_not_of(" \t\r\n");
    if (l == string::npos) return "";
    auto r = s.find_last_not_of(" \t\r\n");
    return s.substr(l, r - l + 1);
}

static inline vector<string> splitCSV(const string& args) {
    // split by commas, then trim each
    vector<string> out;
    string cur; 
    for (char c : args) {
        if (c == ',') {
            string t = trim(cur);
            if (!t.empty()) out.push_back(t);
            cur.clear();
        } else cur.push_back(c);
    }
    string t = trim(cur);
    if (!t.empty()) out.push_back(t);
    return out;
}

// Parse offset(rs) like "8(x2)"  or "-4(x10)"
static bool parseOffsetBase(const string& tok, int& offset, int& baseReg) {
    auto l = tok.find('(');
    auto r = tok.find(')');
    if (l == string::npos || r == string::npos || r < l) return false;
    int off = stoi(tok.substr(0, l));
    string base = tok.substr(l + 1, r - l - 1);
    int rb = regIndex(trim(base));
    if (rb < 0) return false;
    offset = off; baseReg = rb;
    return true;
}

// === Parser: first pass builds labels, second pass decodes ===
vector<string> loadLines(const string& filename) {//load each line of of .s file to vector of string
    ifstream fin(filename);
    if (!fin) {
        cerr << "Error: cannot open " << filename << "\n";
        exit(1);
    }
    vector<string> lines;
    string s;
    while (getline(fin, s)) {//jabtak line milta jaai .s me, tabtak push karte jaa
        lines.push_back(s);
    }
    return lines;
}

struct Program {
    vector<Decoded> code;
    unordered_map<string, int> labelToIndex;
};

Program parseProgram(const string& filename) {
    auto lines = loadLines(filename);
    Program P;

    // First pass: collect labels & keep instruction lines
    vector<string> onlyInstr;
    for (auto& raw : lines) {
        string s = trim(raw);
        if (s.empty()) continue;

        // label line?
        if (s.back() == ':') {
            string label = trim(s.substr(0, s.size() - 1));
            if (P.labelToIndex.count(label)) {
                cerr << "Duplicate label: " << label << "\n";
                exit(1);
            }
            P.labelToIndex[label] = (int)onlyInstr.size();
        } else {
            onlyInstr.push_back(s);
        }
    }

    // Second pass: decode instructions
    for (auto& line : onlyInstr) {
        Decoded d; d.raw = line;

        // op and rest
        string s = line;
        // get opcode (till first space)
        string op;
        {
            stringstream ss(s);
            ss >> op;
        }
        // uppercase op for uniformity
        for (auto &c : op) c = toupper(c);
        d.op = op;

        // get argument string (after opcode)
        auto pos = s.find(' ');
        string argstr = (pos == string::npos) ? "" : trim(s.substr(pos + 1));
        auto args = splitCSV(argstr);

        if (op == "ADD" || op == "SUB") {
            // ADD rd, rs1, rs2
            if (args.size() != 3) { cerr << "Bad R-type: " << line << "\n"; exit(1); }
            d.rd  = regIndex(args[0]);
            d.rs1 = regIndex(args[1]);
            d.rs2 = regIndex(args[2]);
            if (d.rd < 0 || d.rs1 < 0 || d.rs2 < 0) { cerr << "Bad regs in: " << line << "\n"; exit(1); }
        } else if (op == "ADDI") {
            // ADDI rd, rs1, imm
            if (args.size() != 3) { cerr << "Bad ADDI: " << line << "\n"; exit(1); }
            d.rd  = regIndex(args[0]);
            d.rs1 = regIndex(args[1]);
            d.imm = stoi(args[2]);
            if (d.rd < 0 || d.rs1 < 0) { cerr << "Bad regs in: " << line << "\n"; exit(1); }
        } else if (op == "LW") {
            // LW rd, offset(rs1)
            if (args.size() != 2) { cerr << "Bad LW: " << line << "\n"; exit(1); }
            d.rd = regIndex(args[0]);
            int off, base; 
            if (!parseOffsetBase(args[1], off, base)) { cerr << "Bad LW addr: " << line << "\n"; exit(1); }
            d.rs1 = base; d.imm = off;
            if (d.rd < 0 || d.rs1 < 0) { cerr << "Bad regs in: " << line << "\n"; exit(1); }
        } else if (op == "SW") {
            // SW rs2, offset(rs1)
            if (args.size() != 2) { cerr << "Bad SW: " << line << "\n"; exit(1); }
            int off, base; 
            if (!parseOffsetBase(args[1], off, base)) { cerr << "Bad SW addr: " << line << "\n"; exit(1); }
            d.rs2 = regIndex(args[0]); d.rs1 = base; d.imm = off;
            if (d.rs2 < 0 || d.rs1 < 0) { cerr << "Bad regs in: " << line << "\n"; exit(1); }
        } else if (op == "BEQ") {
            // BEQ rs1, rs2, label
            if (args.size() != 3) { cerr << "Bad BEQ: " << line << "\n"; exit(1); }
            d.rs1 = regIndex(args[0]);
            d.rs2 = regIndex(args[1]);
            d.label = args[2]; // resolve later
            if (d.rs1 < 0 || d.rs2 < 0) { cerr << "Bad regs in: " << line << "\n"; exit(1); }
        } else {
            cerr << "Unsupported opcode: " << op << " in line: " << line << "\n";
            exit(1);
        }

        P.code.push_back(d);
    }

    // Resolve labels for BEQ
    for (auto& d : P.code) {
        if (d.op == "BEQ") {
            if (!P.labelToIndex.count(d.label)) {
                cerr << "Undefined label: " << d.label << " used in: " << d.raw << "\n";
                exit(1);
            }
            d.labelTarget = P.labelToIndex[d.label];
        }
    }
    return P;
}

// === CPU core ===
struct CPU {
    static const int MEM_SIZE = 1024; // words

    int32_t R[32]{};                  // register file
    vector<Decoded> code;             // instruction memory
    int PC = 0;                       // instruction index
    int64_t cycle = 0;                // cycle counter
    int32_t MEM[MEM_SIZE]{};          // simple data memory

    // Temporary latches for multi-cycle (non-pipelined) execution:
    Decoded cur;          // current instruction (latched in IF/ID)
    int32_t aluOut = 0;   // EX result
    int32_t memOut = 0;   // MEM read data
    bool takeBranch = false;

    // Helpers
    void printRegs() {//print all register (that are changed)
        cout << "Registers:\n";
        for (int i = 0; i < 32; ++i) {
            if(R[i] != 0 ) { // maine add kiya taki emply registers na print ho.
                cout << "x" << i << "=" << R[i] << ((i%8==7) ? "\n" : "\t");
            }
        }
    }
    void printMem(int from=0, int to=7) {
        cout << "Memory[words " << from << ".." << to << "]:\n";
        for (int i = from; i <= to; ++i) {
            cout << "[" << i << "]=" << MEM[i] << ((i%8==7) ? "\n" : "\t");
        }
    }

    // Stage 1: IF
    bool IF_stage() {
        cycle++; 
        if (PC < 0 || PC >= (int)code.size()) {
            cout << "Cycle " << cycle << ": IF  - PC out of range -> HALT\n";
            return false; // no instruction to fetch
        }
        cur = code[PC];
        cout << "Cycle " << cycle << ": IF  - Fetched \"" << cur.raw << "\" (PC=" << PC << ")\n";
        return true;
    }

    // Stage 2: ID
    void ID_stage() {
        cycle++;
        cout << "Cycle " << cycle << ": ID  - Decoding -> op=" << cur.op;
        if (cur.rd!=-1)  cout << ", rd=x" << cur.rd;
        if (cur.rs1!=-1) cout << ", rs1=x" << cur.rs1;
        if (cur.rs2!=-1) cout << ", rs2=x" << cur.rs2;
        if (cur.op=="ADDI" || cur.op=="LW" || cur.op=="SW" || cur.op=="BEQ") cout << ", imm=" << cur.imm;
        if (cur.op=="BEQ") cout << ", target=" << cur.label << "(" << cur.labelTarget << ")";
        cout << "\n";
    }

    // Stage 3: EX
    void EX_stage() {
        cycle++;
        takeBranch = false;
        if (cur.op == "ADD") {
            aluOut = R[cur.rs1] + R[cur.rs2];
            cout << "Cycle " << cycle << ": EX  - ADD -> " << R[cur.rs1] << " + " << R[cur.rs2] << " = " << aluOut << "\n";
        } else if (cur.op == "SUB") {
            aluOut = R[cur.rs1] - R[cur.rs2];
            cout << "Cycle " << cycle << ": EX  - SUB -> " << R[cur.rs1] << " - " << R[cur.rs2] << " = " << aluOut << "\n";
        } else if (cur.op == "ADDI") {
            aluOut = R[cur.rs1] + cur.imm;
            cout << "Cycle " << cycle << ": EX  - ADDI -> " << R[cur.rs1] << " + " << cur.imm << " = " << aluOut << "\n";
        } else if (cur.op == "LW" || cur.op == "SW") {
            // address = rs1 + imm ; use word index = addr/4
            int addr = R[cur.rs1] + cur.imm;
            int idx = addr / 4;
            aluOut = idx; // pass index to MEM stage
            cout << "Cycle " << cycle << ": EX  - Addr calc -> base=" << R[cur.rs1] << " + off=" << cur.imm
                 << " => byteAddr=" << addr << ", wordIdx=" << idx << "\n";
        } else if (cur.op == "BEQ") {
            takeBranch = (R[cur.rs1] == R[cur.rs2]);
            cout << "Cycle " << cycle << ": EX  - BEQ compare: x" << cur.rs1 << "=" << R[cur.rs1]
                 << " ?= x" << cur.rs2 << "=" << R[cur.rs2]
                 << " -> " << (takeBranch ? "TAKE" : "NOT TAKE") << "\n";
        } else {
            cout << "Cycle " << cycle << ": EX  - (no-op)\n";
        }
    }

    // Stage 4: MEM
    void MEM_stage() {
        cycle++;
        if (cur.op == "LW") {
            if (aluOut < 0 || aluOut >= MEM_SIZE) {
                cout << "Cycle " << cycle << ": MEM - LW OOB index=" << aluOut << " -> 0\n";
                memOut = 0;
            } else {
                memOut = MEM[aluOut];
                cout << "Cycle " << cycle << ": MEM - LW read MEM[" << aluOut << "]=" << memOut << "\n";
            }
        } else if (cur.op == "SW") {
            if (aluOut < 0 || aluOut >= MEM_SIZE) {
                cout << "Cycle " << cycle << ": MEM - SW OOB index=" << aluOut << " (ignored)\n";
            } else {
                MEM[aluOut] = R[cur.rs2];
                cout << "Cycle " << cycle << ": MEM - SW write MEM[" << aluOut << "]=" << R[cur.rs2] << "\n";
            }
        } else {
            cout << "Cycle " << cycle << ": MEM - (no memory op)\n";
        }
    }

    // Stage 5: WB
    void WB_stage() {
        cycle++;
        if (cur.op == "ADD" || cur.op == "SUB" || cur.op == "ADDI") {
            if (cur.rd != 0) R[cur.rd] = aluOut;  // x0 stays 0
            cout << "Cycle " << cycle << ": WB  - x" << cur.rd << " = " << (cur.rd==0?0:aluOut) << "\n";
        } else if (cur.op == "LW") {
            if (cur.rd != 0) R[cur.rd] = memOut;
            cout << "Cycle " << cycle << ": WB  - x" << cur.rd << " = " << (cur.rd==0?0:memOut) << "\n";
        } else if (cur.op == "BEQ") {
            // update PC on branch here (architecturally it's control, we model it after the WB for clarity)
            if (takeBranch) {
                cout << "Cycle " << cycle << ": WB  - Branch taken: PC <- " << cur.labelTarget << "\n";
                PC = cur.labelTarget; // jump to label
                return;
            } else {
                cout << "Cycle " << cycle << ": WB  - Branch not taken\n";
            }
        } else {
            cout << "Cycle " << cycle << ": WB  - (no writeback)\n";
        }

        // Normal PC advance (for non-branch or not-taken branch)
        if (cur.op != "BEQ" || !takeBranch) PC += 1;

        // Enforce x0 = 0
        R[0] = 0;
    }

    // Run until PC out of range or maxSteps hit
    void run(size_t maxSteps = 1e6) {
        size_t steps = 0;
        while (PC >= 0 && PC < (int)code.size() && steps < maxSteps) {
            // one instruction across 5 stages
            if (!IF_stage()) break;
            ID_stage();
            EX_stage();
            MEM_stage();
            WB_stage();
            steps++;
        }
        cout << "\n=== HALT ===\n";
        cout << "Total cycles: " << cycle << ", instructions executed: " << steps << "\n";
    }
};

int main(int argc, char** argv) {
    // ios::sync_with_stdio(false);
    // cin.tie(nullptr);  //for fast i/o setup(optional hai)

    string file = (argc >= 2 ? argv[1] : "simple.s");  // if run directly without specify ./a.exe simple.s, then it will run this file by default
    Program P = parseProgram(file);

    CPU cpu;
    cpu.code = P.code;

    cout << "Loaded " << cpu.code.size() << " instructions.\n\n";
    cpu.run();


    {
        ofstream fout("reg_values.dat");
        if (!fout) {
            cerr << "Error: could not open reg_values.dat for writing\n";
        } else {
            // header (optional)
            fout << "# reg value\n";
            for (int i = 0; i < 8; ++i) {
                fout << "x" << i << " " << cpu.R[i] << "\n";
            }
            fout.close();
            cout << "\nWrote reg_values.dat for plotting.\n";
        }
    }
    cout << "\n--- Final State ---\n";
    cpu.printRegs();
    cout << "\n";
    cpu.printMem(0, 7); // show first few words
    return 0;
}


//for running this code:
//Method-1 direct run ctrl+alt+n
//Method-2 using terminal:
//1. g++ riscv_simulator.cpp -o anyword (to compile the code)
//2. ./anyword mini.s (it'lll give the output of the mini.s file)
//3. gnuplot -p plot.gnu (to plot the graph using gnuplot)