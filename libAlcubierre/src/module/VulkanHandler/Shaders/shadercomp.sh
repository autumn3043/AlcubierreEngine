slangc ./basic_shader.slang \
    -target spirv \
    -profile spirv_1_4 \
    -emit-spirv-directly \
    -fvk-use-entrypoint-name \
    -entry vertMain \
    -entry fragMain \
    -o ./basic_shader.spv
cp ./basic_shader.spv /home/autumn/development/Alcubierre/AlcubierreEngine/sandbox/basic_shader.spv