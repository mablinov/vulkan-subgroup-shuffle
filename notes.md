## Invocation examples
```bash
./render spv/shader.vert.spv spv/shader.frag.spv render.ppm
./render spv/shader.vert.spv spv/shaderSubgroupGray.frag.spv outputs/ubuntu-lavapipe/shaderSubgroupGray.ppm
./render spv/shader.vert.spv spv/shaderSubgroupShuffleGray.frag.spv outputs/ubuntu-lavapipe/shaderSubgroupShuffleGray.ppm
```

## To download the SDK:
```bash
wget https://sdk.lunarg.com/sdk/download/1.4.321.1/linux/vulkansdk-linux-x86_64-1.4.321.1.tar.xz
```

## To convert `.ppm`:
```bash
convert render.ppm render.png
```
