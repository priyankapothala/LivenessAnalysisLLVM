# Liveness Analysis Pass LLVM

## Setup

1. Move to **Pass/build/** directory using *cd* command and execute the following command.
```bash
cmake -DCMAKE_BUILD_TYPE=Release ../Transforms/LivenessAnalysis
```
2. Next execute make and it will generate *.so files under build directory. 
```bash
make -j4
```

## Running the Pass

1. Navigate to the **test** folder and run the pass using the following command

```bash
opt -load ../Pass/build/libLLVMLivenessAnalysisPass.so  -Liveness < 2.ll > /dev/null
```

## Code Explanation 
1. The implemented Pass extends from ``FunctionPass`` class and overrides ``runOnFunction(Function &F)`` function. The Pass implements the iterative algorithm for performing liveness analysis.

2. In ``runOnFunction(Function &F)``, We iterate over the basic blocks and insert them in ``BBList`` and ``BBMap``. ``BBList`` vector is used to store all the basic blocks. ``BBMap`` is a hashmap used to store the basic block and its index. 

    ```c++
	vector<BasicBlock *> BBList;
	map<BasicBlock *, int> BBMap;

	int count = 0;
	for (auto &basic_block : F)
	{
		BBList.push_back(&basic_block);
		BBMap[&basic_block] = count;
		count++;
	}
    ```
3. Next ``UEVar``, ``VarKill`` and ``LiveOut`` vectors are initialized for all the basic blocks and the initialize method is called

    ```c++
    vector<set<Value *>> UEVar(count);
    vector<set<Value *>> VarKill(count);
	vector<set<Value *>> LiveOut(count);

	initialize(BBList, UEVar, VarKill);
    ```

4. In the ``initialize`` function, UEVar and VarKill are computed for each basic block. We iterate over the instructions of each basic block. If the instruction is ``Load``, the variable is added to UEVar. If the instruction is ``Store``, the variable is added to VarKill

    ```c++
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
    ```
5. We iterate over the basic blocks and compute the ``LiveOut`` using ``computeLiveOut`` and stop when they all change

	```c++
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
	```
6. LiveOut of each basic block is computed in ``computeLiveOut`` by iterating over the successors of that basic block
	
   - We iterate over the successors of the basic block using ``successors`` method

   ```c++
   for (BasicBlock *Succ : successors(bb))
   ```

   - ``LiveOut(x) - VarKill(x) U UEVar(x)`` is computed by first calculating ``LiveOut(x) - VarKill(x)`` and stored in dest. Then union of``dest`` and ``VarKill(x)`` is computed and stored in dest1. This procedure is repeated for all the successors of the basic block

   ```c++
    // Compute LiveOut(x) - VarKill(x) and store in dest
	set_difference(LiveOut[i].begin(), LiveOut[i].end(), VarKill[i].begin(), VarKill[i].end(), inserter(dest, dest.begin()));
	// Compute dest(x) union VarKill(x) and store in dest1
	set_union(dest.begin(), dest.end(), UEVar[i].begin(), UEVar[i].end(), inserter(dest1, dest1.begin()));
	// Add this to the LiveOut using union
	set_union(dest1.begin(), dest1.end(), LiveOut[bbIndex].begin(), LiveOut[bbIndex].end(), inserter(LiveOut[bbIndex], LiveOut[bbIndex].begin()));
   ```
   
7. The input file name is obtained using ``F.getParent()->getSourceFileName()`` and by removing the file extension. The filename with .out extension is created and opened

	```c++
	string filename = F.getParent()->getSourceFileName();
    int lastindex = filename.find_last_of("."); 
    string sourcefile = filename.substr(0, lastindex);
	```

	```c++
	string outputfile = sourcefile + ".out";
    outfile.open(outputfile);
	```

8. Then we iterate over all the basic blocks. The basic basic name is obtained using ``getName()``.
    ```c++
    StringRef bbName(BBList[i]->getName());
    ```
9. Varkill, UEVar, LiveOut of each basic block are then written to the outputfile and printed to the console.
	```c++
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
	```


## Test Results

### Output for input file **1.ll**
![image](./1.png)

### Output for input file **2.ll**
![image](./2.png)