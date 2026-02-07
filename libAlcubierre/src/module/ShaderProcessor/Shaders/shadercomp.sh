slangc ./vertex.slang \
    -target spirv \
    -profile spirv_1_4 \
    -emit-spirv-directly \
    -fvk-use-entrypoint-name \
    -entry vertex \
    -o ./output/vertex.spv
# cp ./vertex.spv /home/autumn/development/Alcubierre/AlcubierreEngine/sandbox/vertex.spv
xxd -i ./output/vertex.spv | sed 's/^unsigned char/SPV_ALIGN unsigned char/' > ./output/vertex.h
slangc ./fragment.slang \
    -target spirv \
    -profile spirv_1_4 \
    -emit-spirv-directly \
    -fvk-use-entrypoint-name \
    -entry fragment \
    -o ./output/fragment.spv
# cp ./fragment.spv /home/autumn/development/Alcubierre/AlcubierreEngine/sandbox/fragment.spv
xxd -i ./output/fragment.spv | sed 's/^unsigned char/SPV_ALIGN unsigned char/' > ./output/fragment.h