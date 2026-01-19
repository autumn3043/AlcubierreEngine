slangc ./vertex.slang \
    -target spirv \
    -profile spirv_1_4 \
    -emit-spirv-directly \
    -fvk-use-entrypoint-name \
    -entry vertex \
    -o ./vertex.spv
cp ./vertex.spv /home/autumn/development/Alcubierre/AlcubierreEngine/sandbox/vertex.spv
slangc ./fragment.slang \
    -target spirv \
    -profile spirv_1_4 \
    -emit-spirv-directly \
    -fvk-use-entrypoint-name \
    -entry fragment \
    -o ./fragment.spv
cp ./fragment.spv /home/autumn/development/Alcubierre/AlcubierreEngine/sandbox/fragment.spv