# CapFS Benchmarking

## Environment
Ubuntu 18 EC2 VM. Spec TBD

## Benchmarking Language
Python3.7 
```bash
pip install -r requirements.txt
```

## FS
- EFS: Amazon's NFS 
- CapFS: GDP append-only logging filesystem

## Possible Performance Gap
Presumably, CapFS will have much worse performance because of host environment. 
We need to figure out how to take into account of GDP's poor hosting env.

## MicroBenchmarking
```python
create
read
write
open
close
```

## MacroBenchmarking
```
# sequential read/write
# random read/write
# cp 
# untarring
```

## Metrics
- Latencies
- Read/Write throughput

