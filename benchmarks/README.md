# CapFS Benchmarking
Details: https://docs.google.com/document/d/1UFXS5OLRtr4HG6aCcXR9TU7djOrmNMDJsQIit0jIE9w/edit?fbclid=IwAR3hotN1X4J0LvWQ0_kne9FbeuT9jeWlHlsp6cwnEP_nym0Ad6ev5whrvTU
Check the Evaluation section.

## Environment
Ubuntu 18 EC2 VM. Spec TBD

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

