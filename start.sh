cd Pass/build
cmake -DCMAKE_BUILD_TYPE=Release ../Transforms/LivenessAnalysis
make -j4
cd ../..
cd test
opt -load ../Pass/build/libLLVMLivenessAnalysisPass.so  -Liveness < 1.ll > /dev/null

