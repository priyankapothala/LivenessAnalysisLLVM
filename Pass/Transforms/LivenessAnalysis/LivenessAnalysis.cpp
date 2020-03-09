#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/BasicBlock.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <fstream>

using namespace llvm;
using namespace std;
#define DEBUG_TYPE "Liveness"

namespace
{
struct Liveness : public FunctionPass
{
    string func_name = "test";
    static char ID;

    Liveness() : FunctionPass(ID) {}

    void initialize(vector<BasicBlock *> &BBList, vector<set<Value *>> &UEVar, vector<set<Value *>> &VarKill)
    {
        for (int i = 0; i < BBList.size(); ++i)
        {
            for (auto &inst : *BBList[i])
            {
                if (inst.getOpcode() == Instruction::Load)
                {
                    Value *v = inst.getOperand(0);
                    if (VarKill[i].find(v) == VarKill[i].end())
                    {
                        UEVar[i].insert(v);
                    }
                }
                if (inst.getOpcode() == Instruction::Store)
                {
                    VarKill[i].insert(inst.getOperand(1));
                }
            }
        }
    }

    void computeLiveOut(int bbIndex, map<BasicBlock *, int> BBMap, vector<BasicBlock *> BBList, vector<set<Value *>> UEVar, vector<set<Value *>> VarKill, vector<set<Value *>> &LiveOut)
    {
        BasicBlock *bb = BBList[bbIndex];

        for (BasicBlock *Succ : successors(bb))
        {
            // For all successors of the BB compute LiveOut(x) - VarKill(x) U UEVar(x)
            int i = BBMap.at(Succ);
            set<Value *> dest, dest1;
            // Compute LiveOut(x) - VarKill(x) and store in dest
            set_difference(LiveOut[i].begin(), LiveOut[i].end(), VarKill[i].begin(), VarKill[i].end(), inserter(dest, dest.begin()));
            // Compute dest(x) union VarKill(x) and store in dest1
            set_union(dest.begin(), dest.end(), UEVar[i].begin(), UEVar[i].end(), inserter(dest1, dest1.begin()));
            // Add this to the LiveOut using union
            set_union(dest1.begin(), dest1.end(), LiveOut[bbIndex].begin(), LiveOut[bbIndex].end(), inserter(LiveOut[bbIndex], LiveOut[bbIndex].begin()));
            // clear the temporary vectors
            dest.clear();
            dest1.clear();
        }
    }

    bool runOnFunction(Function &F) override
    {
        errs() << "Liveness Analysis: \n";
        errs() << "Function: " << F.getName() << "\n";

        if (F.getName() != func_name)
            return false;

        vector<BasicBlock *> BBList;  //vector to store all the basic blocks
        map<BasicBlock *, int> BBMap; //map to store the basic block and it's index

        int count = 0;
        for (auto &basic_block : F)
        {
            BBList.push_back(&basic_block);
            BBMap[&basic_block] = count;
            count++;
        }

        // initializing the UEVar,VarKill,LiveOut vectors
        vector<set<Value *>> UEVar(count);
        vector<set<Value *>> VarKill(count);
        vector<set<Value *>> LiveOut(count);

        initialize(BBList, UEVar, VarKill);

        // iterative algorithm to compute LiveOut
        bool flag = true;
        while (flag)
        {
            flag = false;
            for (int i = 0; i < BBList.size(); ++i)
            {
                set<Value *> prev = LiveOut[i];
                computeLiveOut(i, BBMap, BBList, UEVar, VarKill, LiveOut);
                // compare the previous LiveOut and current LiveOut
                // if they differ continue
                if (!std::equal(prev.begin(), prev.end(), LiveOut[i].begin(), LiveOut[i].end()))
                    flag = true;
            }
        }

        string filename = F.getParent()->getSourceFileName();
        int lastindex = filename.find_last_of(".");
        string sourcefile = filename.substr(0, lastindex);
        ofstream outfile;
        string outputfile = sourcefile + ".out";
        errs() << outputfile;
        outfile.open(outputfile);
        
        for (int i = 0; i < BBList.size(); ++i)
        {
            StringRef bbName(BBList[i]->getName());
            errs() << "----- ";
            errs() << bbName;
            errs() << " -----";

            outfile << "----- ";
            outfile << bbName.str();
            outfile << " -----";
            
            errs() << "\nUEVAR: ";
            outfile << "\nUEVAR: ";
            for (auto it = UEVar[i].begin(); it != UEVar[i].end(); ++it)
            {
                outfile << (**it).getName().str() << " ";
                errs() << (**it).getName() << " ";
            }
            outfile << "\nVARKILL: ";
            errs() << "\nVARKILL: ";
            for (auto it = VarKill[i].begin(); it != VarKill[i].end(); ++it)
            {
                outfile << (**it).getName().str() << " ";
                errs() << (**it).getName() << " ";
            }
            outfile << "\nLIVEOUT: ";
            errs() << "\nLIVEOUT: ";
            for (auto it = LiveOut[i].begin(); it != LiveOut[i].end(); ++it)
            {
                outfile << (**it).getName().str() << " ";
                errs() << (**it).getName() << " ";
            }
            errs() << "\n";
            outfile << "\n";
        }
        outfile.close();

        return false;
    }
}; // end of struct Liveness
} // end of anonymous namespace

char Liveness::ID = 0;
static RegisterPass<Liveness> X("Liveness", "Liveness Analysis",
                                false /* Only looks at CFG */,
                                false /* Analysis Pass */);