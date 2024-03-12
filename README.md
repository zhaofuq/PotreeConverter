# PotreeConverter
Modified PotreeConverter for LOD 3D Gaussian Splatting


## Build
```bash
cd PotreeConverter 
cmake . -B build
cmake --build build --config Release
```
## Usage
```bash
./bin/Release/Converter.exe {source} -o {output}
(optional)
--source/-i
--output/-o
--method/-m random/possion/poisson_average
--flags keep-chunks no-indexing
--chunkMethod LASZIP/LAS_CUSTOM/SKIP
--attributes position/intensity/rgb/f_dc_0/f_dc_1/...
```

